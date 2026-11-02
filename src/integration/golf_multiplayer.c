#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

#include "common/debug_console.h"
#include "common/graphics.h"
#include "golf/game.h"
#include "golf/golf.h"
#include "golf/ui.h"
#include "integration/golf_multiplayer.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int multiplayer_ready;
static int match_start_handled;
static char mp_player_name[TG_NET_PLAYER_NAME] = "Player";
static char mp_room_name[TG_NET_ROOM_NAME] = "Golfgame Room";
static char mp_direct_address[48] = "";

typedef struct golf_mp_remote_slot {
    int active;
    int seen;
    uint32_t id;
    float x, y, z;
    float missing_time;
    tg_net_player player;
} golf_mp_remote_slot;

static golf_mp_remote_slot mp_remote_slots[TG_NET_MAX_PLAYERS];
static tg_net_player mp_remote_players[TG_NET_MAX_PLAYERS];
static size_t mp_remote_count;

static const uint32_t mp_player_colors[TG_NET_MAX_PLAYERS] = {
    0xFFFFFFFFu, 0xFF5C5CFFu, 0x4D9DE0FFu, 0xF2C14EFFu,
    0x7DCE82FFu, 0xB388EBFFu, 0xFF8C42FFu, 0x50C5B7FFu,
    0xF06AB8FFu, 0xA7C957FFu, 0xEE964BFFu, 0x6C91C2FFu,
    0xD66BA0FFu, 0x8ECAE6FFu, 0x9BC53DFFu, 0xE55934FFu
};

static const char *status_name(tg_net_status status) {
    switch (status) {
        case TG_NET_STATUS_HOSTING: return "Hosting";
        case TG_NET_STATUS_SEARCHING: return "Searching";
        case TG_NET_STATUS_CONNECTING: return "Connecting";
        case TG_NET_STATUS_CONNECTED: return "Connected";
        case TG_NET_STATUS_OFFLINE:
        default: return "Offline";
    }
}

static void clear_remote_players(void) {
    memset(mp_remote_slots, 0, sizeof(mp_remote_slots));
    memset(mp_remote_players, 0, sizeof(mp_remote_players));
    mp_remote_count = 0;
}

static golf_mp_remote_slot *find_remote_slot(uint32_t id) {
    size_t i;
    golf_mp_remote_slot *free_slot = NULL;

    for (i = 0; i < TG_NET_MAX_PLAYERS; ++i) {
        golf_mp_remote_slot *slot = &mp_remote_slots[i];
        if (slot->active && slot->id == id) return slot;
        if (!slot->active && !free_slot) free_slot = slot;
    }
    return free_slot;
}

static void update_remote_players(float dt, const tg_net_info *info) {
    size_t player_count = 0;
    size_t i;
    const tg_net_player *players;

    for (i = 0; i < TG_NET_MAX_PLAYERS; ++i) {
        mp_remote_slots[i].seen = 0;
    }

    players = tg_net_get_players(&player_count);
    if (player_count > TG_NET_MAX_PLAYERS) player_count = TG_NET_MAX_PLAYERS;

    if (info->role == TG_NET_ROLE_OFFLINE ||
        (info->role == TG_NET_ROLE_CLIENT && info->status != TG_NET_STATUS_CONNECTED)) {
        clear_remote_players();
        return;
    }

    for (i = 0; i < player_count; ++i) {
        const tg_net_player *incoming = &players[i];
        golf_mp_remote_slot *slot;
        float blend;

        if (incoming->id == 0 || incoming->id == info->local_player_id) continue;
        /* Bit 2 means the sender has loaded a course and published a real ball
           position. Do not draw peers at the default world origin in the lobby. */
        if ((incoming->flags & 4u) == 0) continue;

        slot = find_remote_slot(incoming->id);
        if (!slot) continue;

        if (!slot->active || slot->id != incoming->id) {
            memset(slot, 0, sizeof(*slot));
            slot->active = 1;
            slot->id = incoming->id;
            slot->x = incoming->ball_x;
            slot->y = incoming->ball_y;
            slot->z = incoming->ball_z;
        }

        /* Smooth the 10 Hz network snapshots without delaying local input. */
        blend = 1.0f - expf(-fmaxf(dt, 0.0f) * 18.0f);
        slot->x += (incoming->ball_x - slot->x) * blend;
        slot->y += (incoming->ball_y - slot->y) * blend;
        slot->z += (incoming->ball_z - slot->z) * blend;
        slot->player = *incoming;
        slot->player.ball_x = slot->x;
        slot->player.ball_y = slot->y;
        slot->player.ball_z = slot->z;
        slot->missing_time = 0.0f;
        slot->seen = 1;
    }

    mp_remote_count = 0;
    for (i = 0; i < TG_NET_MAX_PLAYERS; ++i) {
        golf_mp_remote_slot *slot = &mp_remote_slots[i];
        if (!slot->active) continue;

        if (!slot->seen) {
            slot->missing_time += fmaxf(dt, 0.0f);
            if (slot->missing_time > 1.5f) {
                memset(slot, 0, sizeof(*slot));
                continue;
            }
        }

        if (mp_remote_count < TG_NET_MAX_PLAYERS) {
            mp_remote_players[mp_remote_count++] = slot->player;
        }
    }
}

static void publish_local_ball(const tg_net_info *info) {
    golf_t *golf = golf_get();
    golf_game_t *game = golf_game_get();
    tg_net_player player;

    if (info->role != TG_NET_ROLE_HOST &&
        !(info->role == TG_NET_ROLE_CLIENT && info->status == TG_NET_STATUS_CONNECTED)) {
        return;
    }
    if (golf->state != GOLF_STATE_IN_GAME || game->state == GOLF_GAME_STATE_MAIN_MENU) {
        return;
    }

    memset(&player, 0, sizeof(player));
    player.ball_x = game->ball.pos.x;
    player.ball_y = game->ball.pos.y;
    player.ball_z = game->ball.pos.z;
    player.ball_qx = game->ball.orientation.x;
    player.ball_qy = game->ball.orientation.y;
    player.ball_qz = game->ball.orientation.z;
    player.ball_qw = game->ball.orientation.w;
    player.ball_colour_rgba = TG_NET_DEFAULT_BALL_COLOUR_RGBA;
    if (info->local_player_id > 0 && info->local_player_id <= TG_NET_MAX_PLAYERS) {
        player.ball_colour_rgba = mp_player_colors[info->local_player_id - 1];
    }
    player.strokes = (uint16_t)(game->stroke_count < 0 ? 0 :
        (game->stroke_count > 65535 ? 65535 : game->stroke_count));
    player.flags |= 4u; /* Valid in-game ball position. */
    if (game->ball.is_moving) player.flags |= 1u;
    if (game->ball.is_in_hole || game->state == GOLF_GAME_STATE_CELEBRATION ||
        game->state == GOLF_GAME_STATE_FINISHED) {
        player.flags |= 2u;
    }
    tg_net_set_local_player(&player);
}

int golf_mp_host(const char *player_name, const char *room_name) {
    int ok = multiplayer_ready ? tg_net_host(player_name, room_name, TG_NET_MAX_PLAYERS) : 0;
    fprintf(stderr, "[golfgame] Host LAN room '%s': %s\n",
        room_name ? room_name : "", ok ? "hosting" : "failed");
    fflush(stderr);
    return ok;
}

int golf_mp_discover(void) {
    return multiplayer_ready ? tg_net_discover() : 0;
}

int golf_mp_join(const char *ipv4_or_hostname, const char *player_name) {
    return multiplayer_ready ? tg_net_join(ipv4_or_hostname, player_name) : 0;
}

int golf_mp_start_match(void) {
    return multiplayer_ready ? tg_net_start_match() : 0;
}

void golf_mp_leave(void) {
    if (multiplayer_ready) {
        tg_net_leave();
        clear_remote_players();
        match_start_handled = 0;
    }
}

void golf_mp_get_info(tg_net_info *out_info) {
    if (out_info) tg_net_get_info(out_info);
}

const tg_net_player *golf_mp_get_players(size_t *out_count) {
    return tg_net_get_players(out_count);
}

const tg_net_player *golf_mp_get_remote_players(size_t *out_count) {
    if (out_count) *out_count = mp_remote_count;
    return mp_remote_players;
}

static void multiplayer_debug_tab(void) {
    tg_net_info info;
    size_t count = 0;
    const tg_net_player *players;

    tg_net_get_info(&info);
    players = tg_net_get_players(&count);

    igText("Golfgame LAN multiplayer");
    igText("Transport: UDP / port %u", (unsigned)TG_NET_DEFAULT_PORT);
    igText("Status: %s", status_name(info.status));
    igText("Room: %s", info.room_name[0] ? info.room_name : "(none)");
    igText("Players: %u / %u", (unsigned)info.player_count, (unsigned)info.max_players);

    if (igButton("Host LAN test room", (ImVec2){0, 0})) {
        golf_mp_host("Player", "Golfgame LAN Match");
    }
    igSameLine(0, 8);
    if (igButton("Find LAN rooms", (ImVec2){0, 0})) {
        golf_mp_discover();
    }

    if (igButton("Join localhost", (ImVec2){0, 0})) {
        golf_mp_join("127.0.0.1", "Player");
    }
    igSameLine(0, 8);
    if (igButton("Start match", (ImVec2){0, 0})) {
        golf_mp_start_match();
    }
    igSameLine(0, 8);
    if (igButton("Leave room", (ImVec2){0, 0})) {
        golf_mp_leave();
    }

    for (size_t i = 0; i < count && i < TG_NET_MAX_PLAYERS; ++i) {
        igText("%s  |  %u strokes  |  ball (%0.2f, %0.2f, %0.2f)",
            players[i].name,
            (unsigned)players[i].strokes,
            players[i].ball_x, players[i].ball_y, players[i].ball_z);
    }

    if (info.last_error[0]) {
        igText("Last network message: %s", info.last_error);
    }
}

void golf_mp_init(void) {
    if (multiplayer_ready) return;
    if (!tg_net_init()) {
        fprintf(stderr, "Golfgame multiplayer initialisation failed.\n");
        return;
    }
    clear_remote_players();
    multiplayer_ready = 1;
    golf_debug_console_add_tab("Multiplayer", multiplayer_debug_tab);
}

void golf_mp_update(float dt) {
    tg_net_info info;
    if (!multiplayer_ready) return;

    tg_net_update(dt);
    tg_net_get_info(&info);
    publish_local_ball(&info);
    update_remote_players(dt, &info);

    if (!info.match_started) {
        match_start_handled = 0;
    } else if (!match_start_handled) {
        golf_state_t state = golf_get()->state;
        if (state == GOLF_STATE_MAIN_MENU) {
            /* A host PK_START or replicated snapshot starts the same course
               for every client. Remote ball positions are displayed as separate balls. */
            match_start_handled = 1;
            golf_start_level(0);
        } else if (state == GOLF_STATE_LOADING_LEVEL || state == GOLF_STATE_IN_GAME) {
            /* Do not restart a level if the start packet arrives late. */
            match_start_handled = 1;
        }
    }
}

static void golf_mp_push_theme(void) {
    igPushStyleColor_Vec4(ImGuiCol_WindowBg, (ImVec4){0.018f, 0.055f, 0.058f, 0.97f});
    igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.12f, 0.31f, 0.29f, 0.85f});
    igPushStyleColor_Vec4(ImGuiCol_TitleBg, (ImVec4){0.018f, 0.075f, 0.072f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_TitleBgActive, (ImVec4){0.035f, 0.16f, 0.14f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_FrameBg, (ImVec4){0.055f, 0.14f, 0.14f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_FrameBgHovered, (ImVec4){0.075f, 0.22f, 0.19f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_FrameBgActive, (ImVec4){0.08f, 0.28f, 0.22f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.055f, 0.32f, 0.25f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.08f, 0.44f, 0.33f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.035f, 0.25f, 0.19f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_Header, (ImVec4){0.045f, 0.25f, 0.20f, 0.9f});
    igPushStyleColor_Vec4(ImGuiCol_HeaderHovered, (ImVec4){0.08f, 0.39f, 0.29f, 0.95f});
    igPushStyleColor_Vec4(ImGuiCol_HeaderActive, (ImVec4){0.04f, 0.30f, 0.23f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_Separator, (ImVec4){0.12f, 0.29f, 0.27f, 0.8f});
    igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 8.0f);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 5.0f);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 1.0f);
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){18.0f, 16.0f});
    igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){11.0f, 8.0f});
    igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){10.0f, 9.0f});
}

static void golf_mp_pop_theme(void) {
    igPopStyleVar(6);
    igPopStyleColor(14);
}

void golf_mp_ui_update(void) {
    tg_net_info info;
    tg_net_room rooms[TG_NET_MAX_ROOMS];
    size_t room_count = 0, i;
    const tg_net_player *players;
    size_t player_count;
    golf_t *golf = golf_get();
    golf_game_t *game = golf_game_get();
    golf_graphics_t *graphics = golf_graphics_get();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    static const int hole_par[18] = {
        2, 3, 3, 4, 3, 4, 3, 4, 3, 4, 4, 3, 4, 4, 4, 4, 4, 5
    };

    if (!multiplayer_ready) return;
    tg_net_get_info(&info);
    players = tg_net_get_players(&player_count);
    golf_mp_push_theme();

    if (golf->state == GOLF_STATE_IN_GAME) {
        int hole = golf->level_num + 1;
        int par = (hole >= 1 && hole <= 18) ? hole_par[hole - 1] : 3;
        igSetNextWindowPos((ImVec2){18.0f, 18.0f}, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
        igSetNextWindowSize((ImVec2){292.0f, 150.0f}, ImGuiCond_Always);
        igSetNextWindowBgAlpha(0.90f);
        if (igBegin("Workshop##roundhud", NULL, flags)) {
            igText("HOLE %02d  /  18", hole);
            igSeparator();
            igText("PAR  %d", par);
            igSameLine(105.0f, 12.0f);
            igText("STROKES  %d", game->stroke_count);
            if (game->state == GOLF_GAME_STATE_AIMING) {
                float power = game->aim_line.power * 100.0f;
                if (power < 0.0f) power = 0.0f;
                if (power > 100.0f) power = 100.0f;
                igText("POWER  %.0f%%", power);
            } else if (game->state == GOLF_GAME_STATE_FINISHED ||
                       game->state == GOLF_GAME_STATE_CELEBRATION) {
                igText("HOLED OUT");
            } else if (game->ball.is_moving) {
                igText("BALL IN MOTION");
            } else {
                igText("HOLD LEFT MOUSE TO AIM");
            }
            igText("%u PLAYERS  /  %s", (unsigned)info.player_count,
                info.status == TG_NET_STATUS_OFFLINE ? "PRACTICE" : status_name(info.status));
        }
        igEnd();

        igSetNextWindowPos((ImVec2){graphics->viewport_size.x - 298.0f, 18.0f},
            ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
        igSetNextWindowSize((ImVec2){280.0f, 250.0f}, ImGuiCond_Always);
        igSetNextWindowBgAlpha(0.90f);
        if (igBegin("Players##round", NULL, flags)) {
            igText("SCOREBOARD");
            igSeparator();
            if (player_count == 0) igText("Solo practice");
            for (i = 0; i < player_count && i < TG_NET_MAX_PLAYERS; ++i) {
                const char *state = (players[i].flags & 2u) ? "HOLED" :
                    ((players[i].flags & 1u) ? "PUTTING" : "READY");
                igBulletText("%s   %u   %s", players[i].name,
                    (unsigned)players[i].strokes, state);
            }
            if (info.last_error[0]) {
                igSeparator();
                igTextWrapped("%s", info.last_error);
            }
        }
        igEnd();
        golf_mp_pop_theme();
        return;
    }

    if (golf->state != GOLF_STATE_MAIN_MENU) {
        golf_mp_pop_theme();
        return;
    }

    room_count = tg_net_get_rooms(rooms, TG_NET_MAX_ROOMS);
    igSetNextWindowPos((ImVec2){graphics->viewport_size.x * 0.5f, 405.0f},
        ImGuiCond_Always, (ImVec2){0.5f, 0.0f});
    igSetNextWindowSize((ImVec2){450.0f, 190.0f}, ImGuiCond_Always);
    igSetNextWindowBgAlpha(0.93f);
    if (igBegin("Workshop entry##main", NULL, flags | ImGuiWindowFlags_NoTitleBar)) {
        igText("THE WORKSHOP");
        igSameLine(0.0f, 12.0f);
        igText("18 HOLES");
        igSeparator();
        igTextWrapped("An original minigolf course pack with ramps, bumpers, corners and moving obstacles.");
        igTextWrapped("Everyone putts at the same time. No turns, no golfer avatars.");
        if (info.role == TG_NET_ROLE_CLIENT) {
            igSeparator();
            igText("WAITING FOR THE HOST TO START THE MATCH");
        } else if (info.role == TG_NET_ROLE_HOST && info.match_started) {
            igSeparator();
            igText("STARTING WORKSHOP MATCH...");
        } else {
            const char *action = info.role == TG_NET_ROLE_HOST ?
                "START WORKSHOP MATCH" : "PLAY WORKSHOP";
            if (igButton(action, (ImVec2){414.0f, 54.0f})) {
                if (info.role == TG_NET_ROLE_HOST) {
                    (void)golf_mp_start_match();
                } else {
                    golf_start_level(0);
                }
            }
        }
        igText("2-16 PLAYERS  /  THIRD-PERSON BALL CONTROL");
    }
    igEnd();

    igSetNextWindowPos((ImVec2){graphics->viewport_size.x - 350.0f, 18.0f},
        ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    igSetNextWindowSize((ImVec2){330.0f, 560.0f}, ImGuiCond_Always);
    igSetNextWindowBgAlpha(0.94f);
    if (igBegin("Play Together", NULL, flags)) {
        igText("NEARBY MULTIPLAYER");
        igText("UP TO %u PLAYERS", (unsigned)TG_NET_MAX_PLAYERS);
        igSeparator();

        if (info.status == TG_NET_STATUS_OFFLINE ||
            info.status == TG_NET_STATUS_SEARCHING) {
            igText("PLAYER NAME");
            igInputText("##mp_player_name", mp_player_name, sizeof(mp_player_name),
                (ImGuiInputTextFlags)0, NULL, NULL);
            igText("ROOM NAME");
            igInputText("##mp_room_name", mp_room_name, sizeof(mp_room_name),
                (ImGuiInputTextFlags)0, NULL, NULL);
            if (igButton("HOST ROOM", (ImVec2){142.0f, 36.0f}))
                (void)golf_mp_host(mp_player_name, mp_room_name);
            igSameLine(0.0f, 8.0f);
            if (igButton("FIND ROOMS", (ImVec2){142.0f, 36.0f}))
                (void)golf_mp_discover();

            igSeparator();
            igText("JOIN BY ADDRESS");
            igInputTextWithHint("##mp_direct_address", "192.168.1.10",
                mp_direct_address, sizeof(mp_direct_address),
                (ImGuiInputTextFlags)0, NULL, NULL);
            if (igButton("JOIN ADDRESS", (ImVec2){292.0f, 34.0f}))
                (void)golf_mp_join(mp_direct_address, mp_player_name);

            igSeparator();
            igText("NEARBY ROOMS");
            if (room_count == 0) {
                igTextWrapped(info.status == TG_NET_STATUS_SEARCHING ?
                    "Searching for Golfgame rooms..." :
                    "No rooms found. Host a room or search again.");
            }
            for (i = 0; i < room_count; ++i) {
                char label[160], button[40];
                snprintf(label, sizeof(label), "%.31s  (%u/%u)\nHost: %.23s",
                    rooms[i].name, (unsigned)rooms[i].player_count,
                    (unsigned)rooms[i].max_players, rooms[i].host_name);
                igTextWrapped("%s", label);
                snprintf(button, sizeof(button), "JOIN##room%u", (unsigned)rooms[i].room_id);
                if (igButton(button, (ImVec2){292.0f, 30.0f}))
                    (void)golf_mp_join(rooms[i].address, mp_player_name);
            }
        } else {
            igText("STATUS  %s", status_name(info.status));
            igTextWrapped("ROOM  %s", info.room_name);
            igText("PLAYERS  %u / %u", (unsigned)info.player_count, (unsigned)info.max_players);
            igSeparator();
            for (i = 0; i < player_count && i < TG_NET_MAX_PLAYERS; ++i)
                igBulletText("%s  /  %u strokes", players[i].name, (unsigned)players[i].strokes);
            if (info.role == TG_NET_ROLE_HOST && !info.match_started) {
                igTextWrapped("Room ready. Start the match from the Workshop panel.");
                igText("Everyone can putt simultaneously.");
            } else if (info.match_started) {
                igTextWrapped("Match started. Loading hole 1...");
            } else if (info.role == TG_NET_ROLE_CLIENT) {
                igTextWrapped("Waiting for the host to start...");
            }
            if (igButton("LEAVE ROOM", (ImVec2){292.0f, 34.0f}))
                golf_mp_leave();
        }
        if (info.last_error[0]) {
            igSeparator();
            igTextWrapped("%s", info.last_error);
        }
    }
    igEnd();
    golf_mp_pop_theme();
}
void golf_mp_shutdown(void) {
    if (!multiplayer_ready) return;
    tg_net_shutdown();
    clear_remote_players();
    multiplayer_ready = 0;
}
