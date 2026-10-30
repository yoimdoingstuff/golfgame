#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

#include "common/debug_console.h"
#include "golf/golf.h"
#include "integration/golf_multiplayer.h"

#include <stdio.h>

static int multiplayer_ready;
static int match_start_handled;
static char mp_player_name[TG_NET_PLAYER_NAME] = "Player";
static char mp_room_name[TG_NET_ROOM_NAME] = "Golfgame Room";
static char mp_direct_address[48] = "";

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

int golf_mp_host(const char *player_name, const char *room_name) {
    int ok = multiplayer_ready ? tg_net_host(player_name, room_name, TG_NET_MAX_PLAYERS) : 0;
    fprintf(stderr, "[golfgame] Host LAN room '%s': %s\\n",
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
    if (multiplayer_ready) tg_net_leave();
}

void golf_mp_get_info(tg_net_info *out_info) {
    if (out_info) tg_net_get_info(out_info);
}

const tg_net_player *golf_mp_get_players(size_t *out_count) {
    return tg_net_get_players(out_count);
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
        igText("%s  |  strokes %u  |  ball (%0.2f, %0.2f, %0.2f)",
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
    multiplayer_ready = 1;
    golf_debug_console_add_tab("Multiplayer", multiplayer_debug_tab);
}

void golf_mp_update(float dt) {
    tg_net_info info;
    if (!multiplayer_ready) return;

    tg_net_update(dt);
    tg_net_get_info(&info);

    if (!info.match_started) {
        match_start_handled = 0;
    } else if (!match_start_handled) {
        golf_state_t state = golf_get()->state;
        if (state == GOLF_STATE_MAIN_MENU) {
            /* A host PK_START or replicated snapshot starts the same course
               for every client. Remote balls are not rendered yet. */
            match_start_handled = 1;
            golf_start_level(0);
        } else if (state == GOLF_STATE_LOADING_LEVEL || state == GOLF_STATE_IN_GAME) {
            /* Do not restart a level if the start packet arrives late. */
            match_start_handled = 1;
        }
    }
}

void golf_mp_ui_update(void) {
    tg_net_info info;
    tg_net_room rooms[TG_NET_MAX_ROOMS];
    size_t room_count, i;
    const tg_net_player *players;
    size_t player_count;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    if (!multiplayer_ready || golf_get()->state != GOLF_STATE_MAIN_MENU) {
        return;
    }

    tg_net_get_info(&info);
    room_count = tg_net_get_rooms(rooms, TG_NET_MAX_ROOMS);
    players = tg_net_get_players(&player_count);

    igSetNextWindowPos((ImVec2){930.0f, 18.0f}, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    igSetNextWindowSize((ImVec2){330.0f, 660.0f}, ImGuiCond_Always);
    igSetNextWindowBgAlpha(0.94f);
    if (igBegin("Multiplayer", NULL, flags)) {
        igText("LOCAL MULTIPLAYER");
        igText("UDP LAN  |  PORT %u  |  UP TO %u PLAYERS",
            (unsigned)TG_NET_DEFAULT_PORT, (unsigned)TG_NET_MAX_PLAYERS);
        igSeparator();

        if (info.status == TG_NET_STATUS_OFFLINE ||
            info.status == TG_NET_STATUS_SEARCHING) {
            igText("PLAYER NAME");
            igInputText("##mp_player_name", mp_player_name, sizeof(mp_player_name),
                (ImGuiInputTextFlags)0, NULL, NULL);
            igText("ROOM NAME");
            igInputText("##mp_room_name", mp_room_name, sizeof(mp_room_name),
                (ImGuiInputTextFlags)0, NULL, NULL);

            if (igButton("Host LAN room", (ImVec2){145.0f, 0.0f})) {
                (void)golf_mp_host(mp_player_name, mp_room_name);
            }
            igSameLine(0.0f, 8.0f);
            if (igButton("Find LAN rooms", (ImVec2){145.0f, 0.0f})) {
                (void)golf_mp_discover();
            }

            igSeparator();
            igText("JOIN BY ADDRESS");
            igInputTextWithHint("##mp_direct_address", "192.168.1.10",
                mp_direct_address, sizeof(mp_direct_address),
                (ImGuiInputTextFlags)0, NULL, NULL);
            if (igButton("Join address", (ImVec2){298.0f, 0.0f})) {
                (void)golf_mp_join(mp_direct_address, mp_player_name);
            }

            igSeparator();
            igText("ROOMS ON YOUR LAN");
            if (room_count == 0) {
                if (info.status == TG_NET_STATUS_SEARCHING) {
                    igTextWrapped("Looking for rooms...");
                } else {
                    igTextWrapped("No rooms found yet. Search again or enter an address.");
                }
            }
            for (i = 0; i < room_count; ++i) {
                char label[160];
                char button[40];
                snprintf(label, sizeof(label), "%s  (%u/%u)\nHost: %s  |  %s",
                    rooms[i].name,
                    (unsigned)rooms[i].player_count,
                    (unsigned)rooms[i].max_players,
                    rooms[i].host_name, rooms[i].address);
                igTextWrapped("%s", label);
                snprintf(button, sizeof(button), "Join##room%u", (unsigned)rooms[i].room_id);
                if (igButton(button, (ImVec2){298.0f, 0.0f})) {
                    (void)golf_mp_join(rooms[i].address, mp_player_name);
                }
            }
        } else {
            igText("Status: %s", status_name(info.status));
            igTextWrapped("Room: %s", info.room_name);
            igText("Players: %u / %u", (unsigned)info.player_count, (unsigned)info.max_players);
            igSeparator();
            for (i = 0; i < player_count && i < TG_NET_MAX_PLAYERS; ++i) {
                igBulletText("%s  |  %u strokes%s",
                    players[i].name,
                    (unsigned)players[i].strokes,
                    (players[i].flags & 2u) ? "  |  finished" : "");
            }

            if (info.role == TG_NET_ROLE_HOST && !info.match_started) {
                if (igButton("START MATCH", (ImVec2){298.0f, 42.0f})) {
                    (void)golf_mp_start_match();
                }
                igTextWrapped("All connected players load the same course together. Remote ball rendering and shared shot simulation are still being integrated.");
            } else if (info.match_started) {
                igTextWrapped("Match started. Loading the course...");
            } else if (info.role == TG_NET_ROLE_CLIENT) {
                igTextWrapped("Waiting for the host to start the match...");
            }

            if (igButton("Leave room", (ImVec2){298.0f, 0.0f})) {
                golf_mp_leave();
            }
        }

        if (info.last_error[0]) {
            igSeparator();
            igTextWrapped("Network message: %s", info.last_error);
        }
    }
    igEnd();
}

void golf_mp_shutdown(void) {
    if (!multiplayer_ready) return;
    tg_net_shutdown();
    multiplayer_ready = 0;
}
