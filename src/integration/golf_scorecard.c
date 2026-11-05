#include "golf_scorecard.h"

#include <stddef.h>
#include <string.h>

static int valid_player(uint32_t player_id) {
    return player_id >= 1u && player_id <= GOLF_SCORECARD_MAX_PLAYERS;
}

void golf_scorecard_reset(golf_scorecard *scorecard) {
    if (!scorecard) return;
    memset(scorecard, 0, sizeof(*scorecard));
}

int golf_scorecard_record(golf_scorecard *scorecard, uint32_t player_id,
                          unsigned hole_index, unsigned strokes) {
    uint32_t bit;
    size_t slot;

    if (!scorecard || !valid_player(player_id) ||
        hole_index >= GOLF_SCORECARD_HOLES ||
        strokes == 0u || strokes > GOLF_SCORECARD_MAX_STROKES) {
        return 0;
    }

    slot = (size_t)(player_id - 1u);
    bit = UINT32_C(1) << hole_index;
    if ((scorecard->recorded_holes[slot] & bit) != 0u) {
        return 0;
    }

    scorecard->strokes[slot][hole_index] = (uint16_t)strokes;
    scorecard->recorded_holes[slot] |= bit;
    return 1;
}

int golf_scorecard_get(const golf_scorecard *scorecard, uint32_t player_id,
                       unsigned hole_index, uint16_t *out_strokes) {
    size_t slot;
    uint32_t bit;

    if (!scorecard || !out_strokes || !valid_player(player_id) ||
        hole_index >= GOLF_SCORECARD_HOLES) {
        return 0;
    }

    slot = (size_t)(player_id - 1u);
    bit = UINT32_C(1) << hole_index;
    if ((scorecard->recorded_holes[slot] & bit) == 0u) return 0;

    *out_strokes = scorecard->strokes[slot][hole_index];
    return 1;
}

unsigned golf_scorecard_holes_completed(const golf_scorecard *scorecard,
                                        uint32_t player_id) {
    uint32_t bits;
    unsigned count = 0u;
    if (!scorecard || !valid_player(player_id)) return 0u;

    bits = scorecard->recorded_holes[player_id - 1u];
    while (bits != 0u) {
        count += bits & 1u;
        bits >>= 1u;
    }
    return count;
}

uint32_t golf_scorecard_total(const golf_scorecard *scorecard,
                              uint32_t player_id) {
    const uint16_t *scores;
    uint32_t total = 0u;
    unsigned hole;
    if (!scorecard || !valid_player(player_id)) return 0u;

    scores = scorecard->strokes[player_id - 1u];
    for (hole = 0u; hole < GOLF_SCORECARD_HOLES; ++hole) {
        if ((scorecard->recorded_holes[player_id - 1u] &
             (UINT32_C(1) << hole)) != 0u) {
            total += scores[hole];
        }
    }
    return total;
}
