#ifndef GOLFGAME_SPAWN_LAYOUT_H
#define GOLFGAME_SPAWN_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#define GOLF_SPAWN_LAYOUT_MAX_PLAYERS 16u

typedef struct golf_spawn_layout_offset {
    float right;
    float forward;
} golf_spawn_layout_offset;

/* Deterministic compact 4x4 tee grid. Slot order comes from the shared room
   roster, not the monotonically increasing network player ID. */
int golf_spawn_layout_get_offset(unsigned slot_index, float ball_radius,
    float separation_gap, golf_spawn_layout_offset *out_offset);

#ifdef __cplusplus
}
#endif

#endif
