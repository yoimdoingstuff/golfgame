#include "golf_scorecard.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    golf_scorecard card;
    uint16_t strokes = 0u;
    unsigned hole;

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

    assert(!golf_scorecard_record(&card, 0u, 0u, 3u));
    assert(!golf_scorecard_record(&card, 17u, 0u, 3u));
    assert(!golf_scorecard_record(&card, 1u, 18u, 3u));
    assert(!golf_scorecard_record(&card, 1u, 4u, 0u));
    assert(!golf_scorecard_record(&card, 1u, 4u, 1000u));
    assert(!golf_scorecard_record(NULL, 1u, 0u, 3u));
    assert(!golf_scorecard_get(&card, 1u, 0u, NULL));

    /* A complete 18-hole round stays in range and totals correctly. */
    golf_scorecard_reset(&card);
    for (hole = 0u; hole < GOLF_SCORECARD_HOLES; ++hole) {
        assert(golf_scorecard_record(&card, 16u, hole, hole + 1u));
    }
    assert(golf_scorecard_holes_completed(&card, 16u) == 18u);
    assert(golf_scorecard_total(&card, 16u) == 171u);
    assert(!golf_scorecard_record(&card, 16u, 17u, 2u));

    golf_scorecard_reset(&card);
    assert(golf_scorecard_total(&card, 16u) == 0u);
    puts("PASS: scorecard validates IDs/holes, records once, and totals all 18 holes");
    return 0;
}
