#ifndef GOLFGAME_MULTIPLAYER_H
#define GOLFGAME_MULTIPLAYER_H

#include "tg_network.h"

void golf_mp_init(void);
void golf_mp_update(float dt);
void golf_mp_shutdown(void);

int golf_mp_host(const char *player_name, const char *room_name);
int golf_mp_discover(void);
int golf_mp_join(const char *ipv4_or_hostname, const char *player_name);
int golf_mp_start_match(void);
void golf_mp_leave(void);

void golf_mp_get_info(tg_net_info *out_info);
const tg_net_player *golf_mp_get_players(size_t *out_count);

#endif
