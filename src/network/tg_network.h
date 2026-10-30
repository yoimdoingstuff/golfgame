#ifndef TG_NETWORK_H
#define TG_NETWORK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TG_NET_MAX_PLAYERS 16
#define TG_NET_MAX_ROOMS 16
#define TG_NET_PLAYER_NAME 24
#define TG_NET_ROOM_NAME 32
#define TG_NET_DEFAULT_PORT 27890
#define TG_NET_DEFAULT_BALL_COLOUR_RGBA 0xFFFFFFFFu

typedef enum tg_net_role {
    TG_NET_ROLE_OFFLINE = 0,
    TG_NET_ROLE_HOST = 1,
    TG_NET_ROLE_CLIENT = 2
} tg_net_role;

typedef enum tg_net_status {
    TG_NET_STATUS_OFFLINE = 0,
    TG_NET_STATUS_HOSTING = 1,
    TG_NET_STATUS_SEARCHING = 2,
    TG_NET_STATUS_CONNECTING = 3,
    TG_NET_STATUS_CONNECTED = 4
} tg_net_status;

/* One network player is one golf ball. No character/avatar state exists. */
typedef struct tg_net_player {
    uint32_t id;
    char name[TG_NET_PLAYER_NAME];
    float ball_x, ball_y, ball_z;
    float ball_qx, ball_qy, ball_qz, ball_qw;
    uint32_t ball_colour_rgba; /* Packed 0xRRGGBBAA; visual only. */
    uint16_t hat_id;           /* 0 means no hat/accessory. */
    uint16_t trail_id;         /* 0 means no trail. */
    uint16_t strokes;
    uint8_t flags;             /* Bit 0: ball moving, bit 1: finished hole. */
} tg_net_player;

typedef struct tg_net_room {
    char name[TG_NET_ROOM_NAME];
    char host_name[TG_NET_PLAYER_NAME];
    char address[48];
    uint16_t port;
    uint8_t player_count;
    uint8_t max_players;
    uint32_t room_id;
} tg_net_room;

typedef struct tg_net_info {
    tg_net_status status;
    tg_net_role role;
    uint32_t local_player_id;
    uint32_t room_id;
    uint8_t player_count;
    uint8_t max_players;
    uint8_t match_started;
    char room_name[TG_NET_ROOM_NAME];
    char local_name[TG_NET_PLAYER_NAME];
    char last_error[160];
} tg_net_info;

int tg_net_init(void);
void tg_net_shutdown(void);
int tg_net_host(const char *player_name, const char *room_name, uint8_t max_players);
int tg_net_discover(void);
size_t tg_net_get_rooms(tg_net_room *out_rooms, size_t capacity);
int tg_net_join(const char *ipv4_or_hostname, const char *player_name);
int tg_net_start_match(void);
void tg_net_leave(void);
void tg_net_update(float dt);
void tg_net_set_local_player(const tg_net_player *player);
void tg_net_get_info(tg_net_info *out_info);
const tg_net_player *tg_net_get_players(size_t *out_count);

#ifdef __cplusplus
}
#endif
#endif
