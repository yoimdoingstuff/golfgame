#include "golf_scorecard.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

int main(void) {
    golf_scorecard card;
    uint16_t strokes = 0u;
    unsigned hole;
    unsigned player;

    golf_scorecard_reset(&card);
    assert(golf_scorecard_total(&card, 1u) == 0u);
    assert(golf_scorecard_holes_completed(&card, 1u) == 0u);

    assert(golf_scorecard_record(&card, 1u, 0u, 3u));
    assert(golf_scorecard_record(&card, 1u, 1u, 4u));
    assert(golf_scorecard_record(&card, 1u, 2u, 2u));
    assert(golf_scorecard_record(&card, 2u, 0u, 5u));

    assert(golf_scorecard_total(&card, 1u) == 9u);
    assert(golf_scorecard_total(&card, 2u) == 5u);
    assert(golf_scorecard_holes_completed(&card, 1u) == 3u);
    assert(golf_scorecard_get(&card, 1u, 2u, &strokes));
    assert(strokes == 2u);
    assert(!golf_scorecard_get(&card, 1u, 3u, &strokes));

    /* Duplicate/replayed hole results must not double-count or overwrite. */
    assert(!golf_scorecard_record(&card, 1u, 1u, 99u));
    assert(golf_scorecard_total(&card, 1u) == 9u);
    assert(golf_scorecard_get(&card, 1u, 1u, &strokes));
    assert(strokes == 4u);

    /* A host-relayed result may correct a provisional peer-snapshot value. */
    assert(golf_scorecard_set(&card, 1u, 1u, 6u));
    assert(golf_scorecard_get(&card, 1u, 1u, &strokes));
    assert(strokes == 6u);
    assert(golf_scorecard_total(&card, 1u) == 11u);
    assert(golf_scorecard_holes_completed(&card, 1u) == 3u);
    assert(!golf_scorecard_set(&card, 0u, 1u, 3u));
    assert(!golf_scorecard_set(&card, 1u, 18u, 3u));
    assert(!golf_scorecard_set(&card, 1u, 3u, 0u));
    assert(!golf_scorecard_set(&card, 1u, 3u, 1000u));
    assert(!golf_scorecard_set(NULL, 1u, 1u, 3u));
    assert(golf_scorecard_get(&card, 1u, 1u, &strokes));
    assert(strokes == 6u);

    assert(!golf_scorecard_record(&card, 0u, 0u, 3u));
    assert(!golf_scorecard_record(&card, 1u, 18u, 3u));
    assert(!golf_scorecard_record(&card, 1u, 4u, 0u));
    assert(!golf_scorecard_record(&card, 1u, 4u, 1000u));
    assert(!golf_scorecard_record(NULL, 1u, 0u, 3u));
    assert(!golf_scorecard_get(&card, 1u, 0u, NULL));

    /* Player IDs can exceed 16 after disconnects and re-joins. */
    assert(golf_scorecard_record(&card, 17u, 0u, 4u));
    assert(golf_scorecard_record(&card, UINT32_C(0xABCDEF01), 0u, 6u));
    assert(golf_scorecard_total(&card, 17u) == 4u);
    assert(golf_scorecard_total(&card, UINT32_C(0xABCDEF01)) == 6u);

    /* A complete 18-hole round remains in range and totals correctly. */
    golf_scorecard_reset(&card);
    for (hole = 0u; hole < GOLF_SCORECARD_HOLES; ++hole) {
        assert(golf_scorecard_record(&card, UINT32_C(0x10001), hole, hole + 1u));
    }
    assert(golf_scorecard_holes_completed(&card, UINT32_C(0x10001)) == 18u);
    assert(golf_scorecard_total(&card, UINT32_C(0x10001)) == 171u);
    assert(!golf_scorecard_record(&card, UINT32_C(0x10001), 17u, 2u));

    /* Don't silently evict a participant's score if the roster is exhausted. */
    for (player = 1u; player < GOLF_SCORECARD_MAX_PLAYERS; ++player) {
        assert(golf_scorecard_record(&card, 0x20000u + player, 0u, 2u));
    }
    assert(!golf_scorecard_record(&card, 0x30000u, 0u, 2u));

    golf_scorecard_reset(&card);
    assert(golf_scorecard_total(&card, UINT32_C(0x10001)) == 0u);
    puts("PASS: scorecard validates input, records provisional rows once, applies host corrections and totals 18 holes");
    return 0;
}
