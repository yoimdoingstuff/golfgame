#ifndef GOLFGAME_SCORECARD_H
#define GOLFGAME_SCORECARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A match is capped at 16 concurrent players, but IDs are monotonic and may
   exceed 16 when participants leave and new players join mid-match. */
#define GOLF_SCORECARD_MAX_PLAYERS 64u
#define GOLF_SCORECARD_HOLES 18u
#define GOLF_SCORECARD_MAX_STROKES 999u

typedef struct golf_scorecard {
    uint32_t player_ids[GOLF_SCORECARD_MAX_PLAYERS];
    uint16_t strokes[GOLF_SCORECARD_MAX_PLAYERS][GOLF_SCORECARD_HOLES];
    uint32_t recorded_holes[GOLF_SCORECARD_MAX_PLAYERS];
} golf_scorecard;

/* Player IDs are non-zero and mapped into a scorecard slot; hole indexes are
   0-based. Each player/hole pair records once. Up to 64 distinct IDs per match. */
void golf_scorecard_reset(golf_scorecard *scorecard);
int golf_scorecard_record(golf_scorecard *scorecard, uint32_t player_id,
                          unsigned hole_index, unsigned strokes);
/* Store a validated host-relayed result, replacing any provisional snapshot row. */
int golf_scorecard_set(golf_scorecard *scorecard, uint32_t player_id,
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
