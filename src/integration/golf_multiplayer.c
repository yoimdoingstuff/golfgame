#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

#include "common/debug_console.h"
#include "common/graphics.h"
#include "golf/game.h"
#include "golf/golf.h"
#include "golf/ui.h"
#include "integration/golf_multiplayer.h"
#include "integration/golf_scorecard.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int multiplayer_ready;
static int match_start_handled;
static float mp_ping_timer;
static int mp_local_ready;
static int mp_next_hole_level = -1;
static int mp_advance_sent;
static int mp_final_score_sent;
static golf_scorecard mp_scorecard;
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

#define GOLF_MP_CONTROL_CHANNEL 0x4747u
#define GOLF_MP_CONTROL_HEADER_BYTES 8u
#define GOLF_MP_HOLE_RECORD_BYTES 6u
#define GOLF_MP_CONTROL_MAX_BYTES (GOLF_MP_CONTROL_HEADER_BYTES + TG_NET_MAX_PLAYERS * GOLF_MP_HOLE_RECORD_BYTES)
#define GOLF_MP_FLAG_READY_NEXT 8u
#define GOLF_MP_HOST_PLAYER_ID 1u
#define GOLF_MP_CONTROL_ADVANCE_HOLE 1u
#define GOLF_MP_CONTROL_SCORE_HOLE 2u

static const uint8_t mp_hole_par[GOLF_SCORECARD_HOLES] = {
    2, 3, 3, 4, 3, 4, 3, 4, 3, 4, 4, 3, 4, 4, 4, 4, 4, 5
};

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

static void clear_match_scorecard(void) {
    golf_scorecard_reset(&mp_scorecard);
}

static unsigned match_par_through_hole(int hole_index) {
    unsigned par = 0u;
    int i;
    if (hole_index < 0) return 0u;
    if (hole_index >= (int)GOLF_SCORECARD_HOLES)
        hole_index = (int)GOLF_SCORECARD_HOLES - 1;
    for (i = 0; i <= hole_index; ++i) par += mp_hole_par[i];
    return par;
}

static uint32_t player_match_total(uint32_t player_id, int hole_index,
                                  unsigned current_strokes, int on_current_hole) {
    uint32_t total = golf_scorecard_total(&mp_scorecard, player_id);
    uint16_t recorded_strokes;
    if (on_current_hole && hole_index >= 0 &&
        hole_index < (int)GOLF_SCORECARD_HOLES &&
        !golf_scorecard_get(&mp_scorecard, player_id,
                            (unsigned)hole_index, &recorded_strokes)) {
        total += current_strokes;
    }
    return total;
}

static void record_finished_player_scores(const tg_net_info *info) {
    golf_t *golf = golf_get();
    golf_game_t *game = golf_game_get();
    const tg_net_player *players;
    size_t count = 0u, i;
    int hole_index;

    if (!info || !info->match_started ||
        info->role == TG_NET_ROLE_OFFLINE ||
        golf->state != GOLF_STATE_IN_GAME) return;

    hole_index = golf->level_num;
    if (hole_index < 0 || hole_index >= (int)GOLF_SCORECARD_HOLES) return;

    players = tg_net_get_players(&count);
    if (count > TG_NET_MAX_PLAYERS) count = TG_NET_MAX_PLAYERS;
    for (i = 0; i < count; ++i) {
        const tg_net_player *player = &players[i];
        if (player->id == 0u || player->id == info->local_player_id) continue;
        if (player->hole_index != (uint16_t)hole_index ||
            (player->flags & 4u) == 0u || (player->flags & 2u) == 0u) continue;
        (void)golf_scorecard_record(&mp_scorecard, player->id,
                                    (unsigned)hole_index, player->strokes);
    }

    if (info->local_player_id > 0u &&
        (game->state == GOLF_GAME_STATE_FINISHED ||
         game->state == GOLF_GAME_STATE_CELEBRATION || game->ball.is_in_hole)) {
        unsigned local_strokes = game->stroke_count < 1 ? 1u :
            (game->stroke_count > (int)GOLF_SCORECARD_MAX_STROKES ?
             GOLF_SCORECARD_MAX_STROKES : (unsigned)game->stroke_count);
        (void)golf_scorecard_record(&mp_scorecard, info->local_player_id,
                                    (unsigned)hole_index, local_strokes);
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
        float target_x = incoming->ball_x;
        float target_y = incoming->ball_y;
        float target_z = incoming->ball_z;

        if (incoming->id == 0 || incoming->id == info->local_player_id) continue;
        /* Bit 2 means the sender has loaded a course and published a real ball
           position. Do not draw peers at the default world origin in the lobby. */
        if ((incoming->flags & 4u) == 0) continue;

        slot = find_remote_slot(incoming->id);
        if (!slot) continue;

        /* Predict only a few milliseconds ahead from the replicated velocity.
           Cap the offset so a malformed/stale velocity cannot move a ball far
           from its last authoritative snapshot. */
        if ((incoming->flags & 1u) != 0u &&
            isfinite(incoming->ball_vx) && isfinite(incoming->ball_vy) &&
            isfinite(incoming->ball_vz)) {
            float speed = sqrtf(incoming->ball_vx * incoming->ball_vx +
                incoming->ball_vy * incoming->ball_vy +
                incoming->ball_vz * incoming->ball_vz);
            if (isfinite(speed) && speed > 0.01f && speed <= 60.0f) {
                float prediction_dt = fminf(0.04f, 0.18f / speed);
                target_x += incoming->ball_vx * prediction_dt;
                target_y += incoming->ball_vy * prediction_dt;
                target_z += incoming->ball_vz * prediction_dt;
            }
        }

        if (!slot->active || slot->id != incoming->id) {
            memset(slot, 0, sizeof(*slot));
            slot->active = 1;
            slot->id = incoming->id;
            slot->x = target_x;
            slot->y = target_y;
            slot->z = target_z;
        }

        /* Smooth the 10 Hz snapshots toward a bounded velocity-predicted target. */
        blend = 1.0f - expf(-fmaxf(dt, 0.0f) * 18.0f);
        slot->x += (target_x - slot->x) * blend;
        slot->y += (target_y - slot->y) * blend;
        slot->z += (target_z - slot->z) * blend;
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
    if (game->ball.is_moving) {
        player.ball_vx = game->ball.vel.x;
        player.ball_vy = game->ball.vel.y;
        player.ball_vz = game->ball.vel.z;
    }
    player.hole_index = (uint16_t)golf->level_num;
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
    if (mp_local_ready && game->state == GOLF_GAME_STATE_FINISHED)
        player.flags |= GOLF_MP_FLAG_READY_NEXT;
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
    if (!multiplayer_ready) return 0;
    mp_local_ready = 0;
    mp_next_hole_level = -1;
    mp_advance_sent = 0;
    mp_final_score_sent = 0;
    clear_match_scorecard();
    return tg_net_start_match();
}

int golf_mp_request_next_hole(int next_level) {
    tg_net_info info;
    golf_t *golf = golf_get();
    golf_game_t *game = golf_game_get();

    if (!multiplayer_ready) return 0;
    tg_net_get_info(&info);
    if (info.role == TG_NET_ROLE_OFFLINE ||
        (info.role == TG_NET_ROLE_CLIENT && info.status != TG_NET_STATUS_CONNECTED) ||
        (info.role == TG_NET_ROLE_HOST && info.status != TG_NET_STATUS_HOSTING)) {
        return 0;
    }
    if (golf->state != GOLF_STATE_IN_GAME ||
        game->state != GOLF_GAME_STATE_FINISHED) return 1;
    if (next_level < 0 || next_level >= 18) return 1;

    if (!mp_local_ready) {
        mp_local_ready = 1;
        mp_next_hole_level = next_level;
        mp_advance_sent = 0;
    }
    return 1;
}

int golf_mp_is_waiting_for_next_hole(void) {
    return mp_local_ready;
}

void golf_mp_leave(void) {
    if (multiplayer_ready) {
        tg_net_leave();
        clear_remote_players();
        match_start_handled = 0;
        mp_ping_timer = 0.0f;
        mp_local_ready = 0;
        mp_next_hole_level = -1;
        mp_advance_sent = 0;
        mp_final_score_sent = 0;
        clear_match_scorecard();
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

static void start_next_hole_local(int next_level) {
    mp_local_ready = 0;
    mp_next_hole_level = -1;
    mp_advance_sent = 0;
    mp_final_score_sent = 0;
    golf_start_level(next_level);
}

static void write_u16_le(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8) & 0xFFu);
}

static void write_u32_le(uint8_t *out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8) & 0xFFu);
    out[2] = (uint8_t)((value >> 16) & 0xFFu);
    out[3] = (uint8_t)((value >> 24) & 0xFFu);
}

static uint16_t read_u16_le(const uint8_t *in) {
    return (uint16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8));
}

static uint32_t read_u32_le(const uint8_t *in) {
    return (uint32_t)in[0] |
        ((uint32_t)in[1] << 8) |
        ((uint32_t)in[2] << 16) |
        ((uint32_t)in[3] << 24);
}

static unsigned local_hole_strokes(const golf_game_t *game) {
    if (!game || game->stroke_count < 1) return 1u;
    if (game->stroke_count > (int)GOLF_SCORECARD_MAX_STROKES)
        return GOLF_SCORECARD_MAX_STROKES;
    return (unsigned)game->stroke_count;
}

/* Host result envelopes use a fixed-endian header and one record per active
   player: 32-bit ID plus 16-bit strokes. */
static int build_hole_result_message(const tg_net_info *info, int hole_index,
        uint8_t command, uint8_t payload[GOLF_MP_CONTROL_MAX_BYTES],
        size_t *out_size) {
    const tg_net_player *players;
    size_t count = 0u, i, record_count = 0u;
    golf_game_t *game = golf_game_get();

    if (!info || !payload || !out_size || hole_index < 0 ||
        hole_index >= (int)GOLF_SCORECARD_HOLES) return 0;

    players = tg_net_get_players(&count);
    if (count == 0u || count > TG_NET_MAX_PLAYERS ||
        count != info->player_count) return 0;

    memset(payload, 0, GOLF_MP_CONTROL_MAX_BYTES);
    memcpy(payload, "GGH1", 4u);
    payload[4] = 1u;
    payload[5] = command;
    payload[6] = (uint8_t)hole_index;

    for (i = 0; i < count; ++i) {
        const tg_net_player *player = &players[i];
        unsigned strokes;
        uint8_t *record;

        if (player->id == 0u || player->hole_index != (uint16_t)hole_index ||
            (player->flags & 4u) == 0u || (player->flags & 2u) == 0u) {
            return 0;
        }
        if (command == GOLF_MP_CONTROL_ADVANCE_HOLE &&
            (player->flags & GOLF_MP_FLAG_READY_NEXT) == 0u) {
            return 0;
        }

        strokes = player->id == info->local_player_id ?
            local_hole_strokes(game) : (unsigned)player->strokes;
        if (strokes == 0u || strokes > GOLF_SCORECARD_MAX_STROKES) return 0;

        record = payload + GOLF_MP_CONTROL_HEADER_BYTES +
            record_count * GOLF_MP_HOLE_RECORD_BYTES;
        write_u32_le(record, player->id);
        write_u16_le(record + 4u, (uint16_t)strokes);
        ++record_count;
    }

    if (record_count == 0u || record_count > TG_NET_MAX_PLAYERS) return 0;
    payload[7] = (uint8_t)record_count;
    *out_size = GOLF_MP_CONTROL_HEADER_BYTES +
        record_count * GOLF_MP_HOLE_RECORD_BYTES;
    return 1;
}

static int apply_hole_result_message(const uint8_t *payload, size_t payload_size) {
    unsigned count, i;
    unsigned hole_index;

    if (!payload || payload_size < GOLF_MP_CONTROL_HEADER_BYTES ||
        memcmp(payload, "GGH1", 4u) != 0 || payload[4] != 1u) return 0;

    hole_index = payload[6];
    count = payload[7];
    if (hole_index >= GOLF_SCORECARD_HOLES || count == 0u ||
        count > TG_NET_MAX_PLAYERS ||
        payload_size != GOLF_MP_CONTROL_HEADER_BYTES +
            (size_t)count * GOLF_MP_HOLE_RECORD_BYTES) return 0;

    for (i = 0; i < count; ++i) {
        const uint8_t *record = payload + GOLF_MP_CONTROL_HEADER_BYTES +
            (size_t)i * GOLF_MP_HOLE_RECORD_BYTES;
        uint32_t player_id = read_u32_le(record);
        uint16_t strokes = read_u16_le(record + 4u);
        unsigned previous;
        if (player_id == 0u || strokes == 0u ||
            strokes > GOLF_SCORECARD_MAX_STROKES) return 0;
        for (previous = 0; previous < i; ++previous) {
            const uint8_t *other = payload + GOLF_MP_CONTROL_HEADER_BYTES +
                (size_t)previous * GOLF_MP_HOLE_RECORD_BYTES;
            if (read_u32_le(other) == player_id) return 0;
        }
    }

    for (i = 0; i < count; ++i) {
        const uint8_t *record = payload + GOLF_MP_CONTROL_HEADER_BYTES +
            (size_t)i * GOLF_MP_HOLE_RECORD_BYTES;
        (void)golf_scorecard_record(&mp_scorecard, read_u32_le(record),
            hole_index, read_u16_le(record + 4u));
    }
    return 1;
}

static void process_match_control_messages(const tg_net_info *info) {
    tg_net_message message;
    golf_t *golf = golf_get();
    golf_game_t *game = golf_game_get();

    while (tg_net_poll_message(&message)) {
        uint8_t command, hole_index;
        int local_finished;
        if (message.channel != GOLF_MP_CONTROL_CHANNEL ||
            message.data_size < GOLF_MP_CONTROL_HEADER_BYTES ||
            memcmp(message.data, "GGH1", 4u) != 0 ||
            message.data[4] != 1u) continue;

        /* Only host-originated envelopes may update shared scores or advance. */
        if (info->role != TG_NET_ROLE_CLIENT ||
            message.sender_id != GOLF_MP_HOST_PLAYER_ID) continue;

        command = message.data[5];
        hole_index = message.data[6];
        if (hole_index != (uint8_t)golf->level_num ||
            hole_index >= GOLF_SCORECARD_HOLES) continue;

        if (command == GOLF_MP_CONTROL_SCORE_HOLE) {
            local_finished = game->ball.is_in_hole ||
                game->state == GOLF_GAME_STATE_CELEBRATION ||
                game->state == GOLF_GAME_STATE_FINISHED;
            if (golf->state != GOLF_STATE_IN_GAME || !local_finished) continue;
            (void)apply_hole_result_message(message.data, message.data_size);
            continue;
        }

        if (command != GOLF_MP_CONTROL_ADVANCE_HOLE ||
            golf->state != GOLF_STATE_IN_GAME ||
            game->state != GOLF_GAME_STATE_FINISHED ||
            !mp_local_ready ||
            (int)hole_index + 1 != golf->level_num + 1 ||
            (int)hole_index + 1 >= (int)GOLF_SCORECARD_HOLES) {
            continue;
        }
        if (!apply_hole_result_message(message.data, message.data_size)) continue;
        start_next_hole_local((int)hole_index + 1);
    }
}

static void host_try_advance_hole(const tg_net_info *info) {
    uint8_t payload[GOLF_MP_CONTROL_MAX_BYTES];
    size_t payload_size = 0u;
    int current_hole;

    if (!info || info->role != TG_NET_ROLE_HOST ||
        !mp_local_ready || mp_advance_sent ||
        mp_next_hole_level < 1 ||
        mp_next_hole_level >= (int)GOLF_SCORECARD_HOLES) return;

    current_hole = mp_next_hole_level - 1;
    if (!build_hole_result_message(info, current_hole,
            GOLF_MP_CONTROL_ADVANCE_HOLE, payload, &payload_size)) return;

    if (info->player_count > 1u && !tg_net_send_reliable_message(0,
            GOLF_MP_CONTROL_CHANNEL, payload, payload_size)) return;

    /* The reliable advance includes exact hole scores before any ball resets. */
    mp_advance_sent = 1;
    start_next_hole_local(mp_next_hole_level);
}

static void host_try_publish_final_hole(const tg_net_info *info) {
    uint8_t payload[GOLF_MP_CONTROL_MAX_BYTES];
    size_t payload_size = 0u;
    golf_t *golf = golf_get();
    golf_game_t *game = golf_game_get();

    if (!info || info->role != TG_NET_ROLE_HOST || !info->match_started ||
        mp_final_score_sent || golf->state != GOLF_STATE_IN_GAME ||
        golf->level_num != (int)GOLF_SCORECARD_HOLES - 1 ||
        !(game->ball.is_in_hole ||
          game->state == GOLF_GAME_STATE_CELEBRATION ||
          game->state == GOLF_GAME_STATE_FINISHED)) return;

    if (!build_hole_result_message(info, golf->level_num,
            GOLF_MP_CONTROL_SCORE_HOLE, payload, &payload_size)) return;

    if (info->player_count > 1u && !tg_net_send_reliable_message(0,
            GOLF_MP_CONTROL_CHANNEL, payload, payload_size)) return;
    mp_final_score_sent = 1;
}

static int golf_mp_request_ping(void) {
    tg_net_info info;
    const tg_net_player *players;
    size_t count = 0, i;

    if (!multiplayer_ready) return 0;
    tg_net_get_info(&info);
    if (info.role == TG_NET_ROLE_CLIENT && info.status == TG_NET_STATUS_CONNECTED) {
        if (tg_net_ping(0)) {
            mp_ping_timer = 0.0f;
            return 1;
        }
        return 0;
    }
    if (info.role == TG_NET_ROLE_HOST && info.status == TG_NET_STATUS_HOSTING) {
        players = tg_net_get_players(&count);
        for (i = 0; i < count && i < TG_NET_MAX_PLAYERS; ++i) {
            if (players[i].id != 0 && players[i].id != info.local_player_id) {
                if (tg_net_ping(players[i].id)) {
                    mp_ping_timer = 0.0f;
                    return 1;
                }
                return 0;
            }
        }
    }
    return 0;
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
    if (info.ping_available) igText("Round-trip latency: %.1f ms", info.ping_ms);
    else if (info.ping_pending) igText("Round-trip latency: measuring...");
    else igText("Round-trip latency: not measured");
    igText("mDNS discovery: %s", info.mdns_available ? "available" : "unavailable");
    if (igButton("Measure ping", (ImVec2){0, 0})) {
        if (!golf_mp_request_ping())
            igText("Connect to a peer before measuring ping.");
    }

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
    clear_match_scorecard();
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
    record_finished_player_scores(&info);
    process_match_control_messages(&info);
    host_try_publish_final_hole(&info);
    host_try_advance_hole(&info);

    mp_ping_timer += fmaxf(dt, 0.0f);
    if (!info.ping_pending && mp_ping_timer >= 3.0f &&
        (info.role == TG_NET_ROLE_HOST ||
         (info.role == TG_NET_ROLE_CLIENT && info.status == TG_NET_STATUS_CONNECTED))) {
        if (!golf_mp_request_ping()) mp_ping_timer = 2.0f;
    }

    if (!info.match_started) {
        match_start_handled = 0;
    } else if (!match_start_handled) {
        golf_state_t state = golf_get()->state;
        if (state == GOLF_STATE_MAIN_MENU) {
            /* A host PK_START or replicated snapshot starts the same course
               for every client. Remote ball positions are displayed as separate balls. */
            match_start_handled = 1;
            clear_match_scorecard();
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


static uint32_t mp_imgui_color(uint32_t rgba) {
    uint32_t r = (rgba >> 24) & 0xFFu;
    uint32_t g = (rgba >> 16) & 0xFFu;
    uint32_t b = (rgba >> 8) & 0xFFu;
    uint32_t a = rgba & 0xFFu;
    return r | (g << 8) | (b << 16) | (a << 24);
}

static void draw_ball_nameplate(ImDrawList *draw_list,
        const char *name, vec3 ball_pos, uint32_t ball_colour,
        const golf_graphics_t *graphics, float ball_radius) {
    char safe_name[TG_NET_PLAYER_NAME];
    size_t name_length;
    vec3 label_world = ball_pos;
    vec2 screen;
    float width, left, top;
    ImVec2 p_min, p_max, dot_pos, text_pos;

    if (!draw_list || !name || !graphics) return;

    snprintf(safe_name, sizeof(safe_name), "%.*s",
        (int)sizeof(safe_name) - 1, name);
    name_length = strlen(safe_name);
    if (name_length == 0) return;

    label_world.y += fmaxf(ball_radius * 2.5f, 0.24f);
    if (vec3_dot(vec3_sub(label_world, graphics->cam_pos),
            graphics->cam_dir) <= 0.10f) {
        return;
    }

    screen = golf_graphics_world_to_screen(label_world);
    if (!isfinite(screen.x) || !isfinite(screen.y) ||
        screen.x < graphics->viewport_pos.x - 32.0f ||
        screen.y < graphics->viewport_pos.y - 28.0f ||
        screen.x > graphics->viewport_pos.x + graphics->viewport_size.x + 32.0f ||
        screen.y > graphics->viewport_pos.y + graphics->viewport_size.y + 28.0f) {
        return;
    }

    width = (float)name_length * 7.0f + 24.0f;
    left = screen.x - width * 0.5f;
    top = screen.y - 23.0f;
    p_min = (ImVec2){left, top};
    p_max = (ImVec2){left + width, top + 20.0f};
    dot_pos = (ImVec2){left + 9.0f, top + 10.0f};
    text_pos = (ImVec2){left + 17.0f, top + 3.0f};

    if (ball_colour == 0) ball_colour = TG_NET_DEFAULT_BALL_COLOUR_RGBA;
    ImDrawList_AddRectFilled(draw_list, p_min, p_max, 0xD911171Eu, 5.0f, 0);
    ImDrawList_AddCircleFilled(draw_list, dot_pos, 3.5f,
        mp_imgui_color(ball_colour), 12);
    ImDrawList_AddText_Vec2(draw_list, text_pos, 0xFFFFFFFFu,
        safe_name, safe_name + name_length);
}

static void draw_live_ball_nameplates(const tg_net_info *info,
        const golf_game_t *game, const golf_graphics_t *graphics) {
    size_t remote_count = 0;
    size_t i;
    const tg_net_player *remote_players;
    uint32_t local_colour = TG_NET_DEFAULT_BALL_COLOUR_RGBA;
    ImDrawList *draw_list;

    if (!info || !game || !graphics || info->role == TG_NET_ROLE_OFFLINE) return;

    draw_list = igGetForegroundDrawList_Nil();
    if (!draw_list) return;

    if (info->local_player_id > 0 &&
        info->local_player_id <= TG_NET_MAX_PLAYERS) {
        local_colour = mp_player_colors[info->local_player_id - 1];
    }
    draw_ball_nameplate(draw_list, mp_player_name, game->ball.draw_pos,
        local_colour, graphics, game->ball.radius);

    remote_players = golf_mp_get_remote_players(&remote_count);
    for (i = 0; i < remote_count && i < TG_NET_MAX_PLAYERS; ++i) {
        const tg_net_player *player = &remote_players[i];
        draw_ball_nameplate(draw_list, player->name,
            V3(player->ball_x, player->ball_y, player->ball_z),
            player->ball_colour_rgba, graphics, game->ball.radius);
    }
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
        igSetNextWindowSize((ImVec2){292.0f, 190.0f}, ImGuiCond_Always);
        igSetNextWindowBgAlpha(0.90f);
        if (igBegin("Workshop##roundhud", NULL, flags)) {
            igText("HOLE %02d  /  18", hole);
            igSeparator();
            igText("PAR  %d", par);
            igSameLine(105.0f, 12.0f);
            igText("STROKES  %d", game->stroke_count);
            if (mp_local_ready) {
                igText("WAITING FOR THE OTHER PLAYERS");
            } else if (game->state == GOLF_GAME_STATE_AIMING) {
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
            if (info.match_started && info.local_player_id > 0u) {
                unsigned total = player_match_total(info.local_player_id,
                    hole - 1, (unsigned)(game->stroke_count > 0 ? game->stroke_count : 0), 1);
                igText("TOTAL  %u  /  PAR  %u", total, match_par_through_hole(hole - 1));
            }
            if (info.ping_available) igText("PING  %.1f ms", info.ping_ms);
            else if (info.ping_pending) igText("PING  MEASURING");
            else if (info.status == TG_NET_STATUS_CONNECTED ||
                     info.status == TG_NET_STATUS_HOSTING) igText("PING  NOT MEASURED");
        }
        igEnd();

        igSetNextWindowPos((ImVec2){graphics->viewport_size.x - 370.0f, 18.0f},
            ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
        igSetNextWindowSize((ImVec2){352.0f, 500.0f}, ImGuiCond_Always);
        igSetNextWindowBgAlpha(0.90f);
        if (igBegin("Players##round", NULL, flags)) {
            igText("18-HOLE SCORECARD");
            if (info.match_started) {
                igText("PLAYER  /  HOLE  /  TOTAL (+/- PAR)");
            }
            igSeparator();
            if (player_count == 0) igText("Solo practice");
            for (i = 0; i < player_count && i < TG_NET_MAX_PLAYERS; ++i) {
                const tg_net_player *player = &players[i];
                const char *state = (player->flags & GOLF_MP_FLAG_READY_NEXT) ? "NEXT READY" :
                    ((player->flags & 2u) ? "HOLED" :
                    ((player->flags & 1u) ? "PUTTING" : "READY"));
                int on_current_hole = player->hole_index == (uint16_t)golf->level_num;
                int par_hole = on_current_hole ? golf->level_num : golf->level_num - 1;
                uint32_t total = info.match_started ? player_match_total(
                    player->id, golf->level_num, player->strokes, on_current_hole) : player->strokes;
                int32_t relative_par = (int32_t)total -
                    (int32_t)match_par_through_hole(par_hole);
                if (on_current_hole) {
                    igBulletText("%s  /  %u  /  %u (%+d)  %s", player->name,
                        (unsigned)player->strokes, (unsigned)total,
                        (int)relative_par, state);
                } else {
                    igBulletText("%s  /  --  /  %u (%+d)  LOADING", player->name,
                        (unsigned)total, (int)relative_par);
                }
            }
            if (info.last_error[0]) {
                igSeparator();
                igTextWrapped("%s", info.last_error);
            }
        }
        igEnd();
        draw_live_ball_nameplates(&info, game, graphics);
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
            if (info.ping_available) igText("ROUND-TRIP  %.1f ms", info.ping_ms);
            else if (info.ping_pending) igText("ROUND-TRIP  measuring...");
            else igText("ROUND-TRIP  not measured");
            igText("mDNS discovery  %s", info.mdns_available ? "available" : "unavailable");
            if (igButton("TEST CONNECTION", (ImVec2){292.0f, 30.0f})) {
                if (!golf_mp_request_ping())
                    fprintf(stderr, "[golfgame] Ping requires a connected peer.\\n");
            }
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
