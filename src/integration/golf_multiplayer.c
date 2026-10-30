#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

#include "common/debug_console.h"
#include "integration/golf_multiplayer.h"

#include <stdio.h>

static int multiplayer_ready;

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
    return multiplayer_ready ? tg_net_host(player_name, room_name, TG_NET_MAX_PLAYERS) : 0;
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
    if (multiplayer_ready) tg_net_update(dt);
}

void golf_mp_shutdown(void) {
    if (!multiplayer_ready) return;
    tg_net_shutdown();
    multiplayer_ready = 0;
}
