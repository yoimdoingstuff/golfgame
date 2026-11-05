/* SPDX-License-Identifier: GPL-3.0-or-later
   Golfgame compatibility adapter over the reusable MultiNet library. */
#include "tg_network.h"
#include "multinet.h"

#include <stdio.h>
#include <string.h>

static mn_instance *g_network;
static tg_net_player g_players[TG_NET_MAX_PLAYERS];
static tg_net_info g_empty_info;
static tg_net_message g_message_queue[TG_NET_MESSAGE_QUEUE_CAPACITY];
static size_t g_message_head;
static size_t g_message_count;

static void clear_message_queue(void) {
    memset(g_message_queue, 0, sizeof(g_message_queue));
    g_message_head = 0u;
    g_message_count = 0u;
}

static void enqueue_message(const mn_event *event) {
    size_t index;
    tg_net_message *message;
    if (!event || event->type != MN_EVENT_MESSAGE ||
        event->data_size > TG_NET_MAX_MESSAGE_BYTES) return;
    if (g_message_count == TG_NET_MESSAGE_QUEUE_CAPACITY) {
        g_message_head = (g_message_head + 1u) % TG_NET_MESSAGE_QUEUE_CAPACITY;
        --g_message_count;
    }
    index = (g_message_head + g_message_count) % TG_NET_MESSAGE_QUEUE_CAPACITY;
    message = &g_message_queue[index];
    memset(message, 0, sizeof(*message));
    message->sender_id = event->player_id;
    message->channel = event->channel;
    message->data_size = event->data_size;
    if (event->data_size > 0u) memcpy(message->data, event->data, event->data_size);
    ++g_message_count;
}

static void copy_text(char *destination, size_t capacity, const char *source) {
    size_t length = 0u;
    if (!destination || capacity == 0u) return;
    if (!source) source = "";
    while (length + 1u < capacity && source[length] != '\0') ++length;
    if (length > 0u) memcpy(destination, source, length);
    destination[length] = '\0';
}

static tg_net_status map_status(mn_status status) {
    switch (status) {
        case MN_STATUS_HOSTING: return TG_NET_STATUS_HOSTING;
        case MN_STATUS_SEARCHING: return TG_NET_STATUS_SEARCHING;
        case MN_STATUS_CONNECTING: return TG_NET_STATUS_CONNECTING;
        case MN_STATUS_CONNECTED: return TG_NET_STATUS_CONNECTED;
        case MN_STATUS_OFFLINE:
        default: return TG_NET_STATUS_OFFLINE;
    }
}

static tg_net_role map_role(mn_role role) {
    switch (role) {
        case MN_ROLE_HOST: return TG_NET_ROLE_HOST;
        case MN_ROLE_CLIENT: return TG_NET_ROLE_CLIENT;
        case MN_ROLE_NONE:
        default: return TG_NET_ROLE_OFFLINE;
    }
}

static void map_player(const mn_player *source, tg_net_player *destination) {
    int32_t score;
    if (!source || !destination) return;
    memset(destination, 0, sizeof(*destination));
    destination->id = source->id;
    copy_text(destination->name, sizeof(destination->name), source->name);
    destination->ball_x = source->position.x;
    destination->ball_y = source->position.y;
    destination->ball_z = source->position.z;
    destination->ball_vx = source->velocity.x;
    destination->ball_vy = source->velocity.y;
    destination->ball_vz = source->velocity.z;
    destination->ball_qx = source->rotation.x;
    destination->ball_qy = source->rotation.y;
    destination->ball_qz = source->rotation.z;
    destination->ball_qw = source->rotation.w;
    destination->ball_colour_rgba = source->colour_rgba;
    destination->hat_id = source->appearance[0];
    destination->trail_id = source->appearance[1];
    destination->hole_index = source->appearance[2];
    score = source->score;
    if (score < 0) score = 0;
    if (score > 65535) score = 65535;
    destination->strokes = (uint16_t)score;
    destination->flags = (uint8_t)(source->flags & 0xFFu);
}

static void map_local_player(const tg_net_player *source, mn_player *destination) {
    if (!source || !destination) return;
    memset(destination, 0, sizeof(*destination));
    destination->id = source->id;
    copy_text(destination->name, sizeof(destination->name), source->name);
    destination->position.x = source->ball_x;
    destination->position.y = source->ball_y;
    destination->position.z = source->ball_z;
    destination->velocity.x = source->ball_vx;
    destination->velocity.y = source->ball_vy;
    destination->velocity.z = source->ball_vz;
    destination->rotation.x = source->ball_qx;
    destination->rotation.y = source->ball_qy;
    destination->rotation.z = source->ball_qz;
    destination->rotation.w = source->ball_qw;
    destination->colour_rgba = source->ball_colour_rgba ?
        source->ball_colour_rgba : TG_NET_DEFAULT_BALL_COLOUR_RGBA;
    destination->appearance[0] = source->hat_id;
    destination->appearance[1] = source->trail_id;
    destination->appearance[2] = source->hole_index;
    destination->score = (int32_t)source->strokes;
    destination->flags = source->flags;
}

int tg_net_init(void) {
    mn_config config;
    if (g_network) return 1;
    memset(g_players, 0, sizeof(g_players));
    clear_message_queue();
    memset(&g_empty_info, 0, sizeof(g_empty_info));
    g_empty_info.role = TG_NET_ROLE_OFFLINE;
    g_empty_info.status = TG_NET_STATUS_OFFLINE;
    g_empty_info.max_players = TG_NET_MAX_PLAYERS;
    mn_config_init(&config);
    config.port = TG_NET_DEFAULT_PORT;
    g_network = mn_create(&config);
    if (!g_network) {
        copy_text(g_empty_info.last_error, sizeof(g_empty_info.last_error),
                  "MultiNet could not create its UDP socket");
        return 0;
    }
    return 1;
}

void tg_net_shutdown(void) {
    if (g_network) {
        mn_destroy(g_network);
        g_network = NULL;
    }
    clear_message_queue();
    memset(g_players, 0, sizeof(g_players));
    memset(&g_empty_info, 0, sizeof(g_empty_info));
    g_empty_info.role = TG_NET_ROLE_OFFLINE;
    g_empty_info.status = TG_NET_STATUS_OFFLINE;
    g_empty_info.max_players = TG_NET_MAX_PLAYERS;
}

int tg_net_host(const char *player_name, const char *room_name,
                uint8_t max_players) {
    if (!g_network && !tg_net_init()) return 0;
    return mn_host(g_network, player_name, room_name, max_players);
}

int tg_net_discover(void) {
    if (!g_network && !tg_net_init()) return 0;
    /* Try global/subnet broadcast and mDNS together. Manual join remains
       available in the existing Golfgame UI when discovery is filtered. */
    return mn_discover_auto(g_network);
}

size_t tg_net_get_rooms(tg_net_room *out_rooms, size_t capacity) {
    mn_room rooms[MN_MAX_ROOMS];
    size_t total, count, i;
    if (!g_network || !out_rooms) return 0u;
    total = mn_get_rooms(g_network, rooms, MN_MAX_ROOMS);
    count = total;
    if (count > capacity) count = capacity;
    if (count > TG_NET_MAX_ROOMS) count = TG_NET_MAX_ROOMS;
    for (i = 0; i < count; ++i) {
        memset(&out_rooms[i], 0, sizeof(out_rooms[i]));
        copy_text(out_rooms[i].name, sizeof(out_rooms[i].name), rooms[i].name);
        copy_text(out_rooms[i].host_name, sizeof(out_rooms[i].host_name),
                  rooms[i].host_name);
        copy_text(out_rooms[i].address, sizeof(out_rooms[i].address),
                  rooms[i].address);
        out_rooms[i].port = rooms[i].port;
        out_rooms[i].player_count = rooms[i].player_count;
        out_rooms[i].max_players = rooms[i].max_players;
        out_rooms[i].room_id = rooms[i].room_id;
    }
    return count;
}

int tg_net_join(const char *ipv4_or_hostname, const char *player_name) {
    if (!g_network && !tg_net_init()) return 0;
    return mn_join(g_network, ipv4_or_hostname, TG_NET_DEFAULT_PORT, player_name);
}

int tg_net_start_match(void) {
    return g_network ? mn_start_match(g_network) : 0;
}

void tg_net_leave(void) {
    if (g_network) mn_leave(g_network);
    clear_message_queue();
    memset(g_players, 0, sizeof(g_players));
}

void tg_net_update(float dt) {
    mn_event event;
    if (!g_network) return;
    mn_update(g_network, dt);
    /* Preserve application messages for the game while consuming lifecycle,
       pairing and delivery events that this compatibility API does not expose. */
    while (mn_poll_event(g_network, &event)) {
        if (event.type == MN_EVENT_MESSAGE) enqueue_message(&event);
    }
}

void tg_net_set_local_player(const tg_net_player *player) {
    mn_player converted;
    if (!g_network || !player) return;
    map_local_player(player, &converted);
    (void)mn_set_local_player(g_network, &converted);
}

int tg_net_ping(uint32_t target_player_id) {
    return g_network ? mn_ping(g_network, target_player_id) : 0;
}

int tg_net_send_reliable_message(uint32_t target_player_id, uint16_t channel,
                                 const void *data, size_t size) {
    if (!g_network || !data || size == 0u || size > TG_NET_MAX_MESSAGE_BYTES) return 0;
    return mn_send_reliable_message(g_network, target_player_id, channel, data, size);
}

int tg_net_poll_message(tg_net_message *out_message) {
    if (!out_message || g_message_count == 0u) return 0;
    *out_message = g_message_queue[g_message_head];
    memset(&g_message_queue[g_message_head], 0, sizeof(g_message_queue[g_message_head]));
    g_message_head = (g_message_head + 1u) % TG_NET_MESSAGE_QUEUE_CAPACITY;
    --g_message_count;
    return 1;
}

void tg_net_get_info(tg_net_info *out_info) {
    mn_info info;
    if (!out_info) return;
    if (!g_network) {
        *out_info = g_empty_info;
        return;
    }
    memset(out_info, 0, sizeof(*out_info));
    mn_get_info(g_network, &info);
    out_info->status = map_status(info.status);
    out_info->role = map_role(info.role);
    out_info->local_player_id = info.local_player_id;
    out_info->room_id = info.room_id;
    out_info->player_count = info.player_count;
    out_info->max_players = info.max_players;
    out_info->match_started = info.match_started;
    out_info->ping_ms = info.ping_ms;
    out_info->ping_available = info.ping_available;
    out_info->ping_pending = info.ping_pending;
    out_info->mdns_available = info.mdns_available;
    copy_text(out_info->room_name, sizeof(out_info->room_name), info.room_name);
    copy_text(out_info->local_name, sizeof(out_info->local_name), info.local_name);
    copy_text(out_info->last_error, sizeof(out_info->last_error), info.last_error);
}

const tg_net_player *tg_net_get_players(size_t *out_count) {
    mn_player players[MN_MAX_PLAYERS];
    size_t total = 0u;
    size_t count, i;
    memset(g_players, 0, sizeof(g_players));
    if (g_network) {
        total = mn_get_players(g_network, players, MN_MAX_PLAYERS);
        count = total < TG_NET_MAX_PLAYERS ? total : TG_NET_MAX_PLAYERS;
        for (i = 0; i < count; ++i)
            map_player(&players[i], &g_players[i]);
    } else {
        count = 0u;
    }
    if (out_count) *out_count = count;
    return g_players;
}
