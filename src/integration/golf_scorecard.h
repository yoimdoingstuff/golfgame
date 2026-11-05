#ifndef GOLFGAME_SCORECARD_H
#define GOLFGAME_SCORECARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GOLF_SCORECARD_MAX_PLAYERS 16u
#define GOLF_SCORECARD_HOLES 18u
#define GOLF_SCORECARD_MAX_STROKES 999u

typedef struct golf_scorecard {
    uint16_t strokes[GOLF_SCORECARD_MAX_PLAYERS][GOLF_SCORECARD_HOLES];
    uint32_t recorded_holes[GOLF_SCORECARD_MAX_PLAYERS];
} golf_scorecard;

/* Player IDs are 1-based; hole indexes are 0-based. Each hole records once. */
void golf_scorecard_reset(golf_scorecard *scorecard);
int golf_scorecard_record(golf_scorecard *scorecard, uint32_t player_id,
                          unsigned hole_index, unsigned strokes);
int golf_scorecard_get(const golf_scorecard *scorecard, uint32_t player_id,
                       unsigned hole_index, uint16_t *out_strokes);
unsigned golf_scorecard_holes_completed(const golf_scorecard *scorecard,
                                        uint32_t player_id);
uint32_t golf_scorecard_total(const golf_scorecard *scorecard,
                              uint32_t player_id);

#ifdef __cplusplus
}
#endif

#endif
