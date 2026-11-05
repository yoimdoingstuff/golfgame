#include "golf_scorecard.h"

#include <stddef.h>
#include <string.h>

static int find_player_slot(const golf_scorecard *scorecard,
                            uint32_t player_id, int create_slot) {
    size_t i;
    size_t first_free = GOLF_SCORECARD_MAX_PLAYERS;

    if (!scorecard || player_id == 0u) return -1;
    for (i = 0u; i < GOLF_SCORECARD_MAX_PLAYERS; ++i) {
        if (scorecard->player_ids[i] == player_id) return (int)i;
        if (scorecard->player_ids[i] == 0u &&
            first_free == GOLF_SCORECARD_MAX_PLAYERS) {
            first_free = i;
        }
    }
    if (!create_slot || first_free == GOLF_SCORECARD_MAX_PLAYERS) return -1;
    ((golf_scorecard *)scorecard)->player_ids[first_free] = player_id;
    return (int)first_free;
}

void golf_scorecard_reset(golf_scorecard *scorecard) {
    if (!scorecard) return;
    memset(scorecard, 0, sizeof(*scorecard));
}

int golf_scorecard_record(golf_scorecard *scorecard, uint32_t player_id,
                          unsigned hole_index, unsigned strokes) {
    uint32_t bit;
    int slot;

    if (!scorecard || player_id == 0u ||
        hole_index >= GOLF_SCORECARD_HOLES ||
        strokes == 0u || strokes > GOLF_SCORECARD_MAX_STROKES) {
        return 0;
    }

    slot = find_player_slot(scorecard, player_id, 1);
    if (slot < 0) return 0;
    bit = UINT32_C(1) << hole_index;
    if ((scorecard->recorded_holes[slot] & bit) != 0u) return 0;

    scorecard->strokes[slot][hole_index] = (uint16_t)strokes;
    scorecard->recorded_holes[slot] |= bit;
    return 1;
}

int golf_scorecard_set(golf_scorecard *scorecard, uint32_t player_id,
                       unsigned hole_index, unsigned strokes) {
    uint32_t bit;
    int slot;

    if (!scorecard || player_id == 0u ||
        hole_index >= GOLF_SCORECARD_HOLES ||
        strokes == 0u || strokes > GOLF_SCORECARD_MAX_STROKES) {
        return 0;
    }

    slot = find_player_slot(scorecard, player_id, 1);
    if (slot < 0) return 0;
    bit = UINT32_C(1) << hole_index;

    /* The host-relayed result replaces a snapshot-derived provisional value. */
    scorecard->strokes[slot][hole_index] = (uint16_t)strokes;
    scorecard->recorded_holes[slot] |= bit;
    return 1;
}

int golf_scorecard_get(const golf_scorecard *scorecard, uint32_t player_id,
                       unsigned hole_index, uint16_t *out_strokes) {
    uint32_t bit;
    int slot;

    if (!scorecard || !out_strokes || player_id == 0u ||
        hole_index >= GOLF_SCORECARD_HOLES) {
        return 0;
    }

    slot = find_player_slot(scorecard, player_id, 0);
    if (slot < 0) return 0;
    bit = UINT32_C(1) << hole_index;
    if ((scorecard->recorded_holes[slot] & bit) == 0u) return 0;

    *out_strokes = scorecard->strokes[slot][hole_index];
    return 1;
}

unsigned golf_scorecard_holes_completed(const golf_scorecard *scorecard,
                                        uint32_t player_id) {
    uint32_t bits;
    unsigned count = 0u;
    int slot = find_player_slot(scorecard, player_id, 0);
    if (slot < 0) return 0u;

    bits = scorecard->recorded_holes[slot];
    while (bits != 0u) {
        count += bits & 1u;
        bits >>= 1u;
    }
    return count;
}

uint32_t golf_scorecard_total(const golf_scorecard *scorecard,
                              uint32_t player_id) {
    uint32_t total = 0u;
    unsigned hole;
    int slot = find_player_slot(scorecard, player_id, 0);
    if (slot < 0) return 0u;

    for (hole = 0u; hole < GOLF_SCORECARD_HOLES; ++hole) {
        if ((scorecard->recorded_holes[slot] &
             (UINT32_C(1) << hole)) != 0u) {
            total += scorecard->strokes[slot][hole];
        }
    }
    return total;
}
