#include "golf_spawn_layout.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    golf_spawn_layout_offset offsets[GOLF_SPAWN_LAYOUT_MAX_PLAYERS];
    const float radius = 0.12f;
    const float gap = 0.025f;
    const float required_spacing = radius * 2.0f + gap;
    float sum_right = 0.0f;
    float sum_forward = 0.0f;
    unsigned i, j;

    for (i = 0; i < GOLF_SPAWN_LAYOUT_MAX_PLAYERS; ++i) {
        assert(golf_spawn_layout_get_offset(i, radius, gap, &offsets[i]));
        sum_right += offsets[i].right;
        sum_forward += offsets[i].forward;
    }

    assert(fabsf(sum_right) < 0.0001f);
    assert(fabsf(sum_forward) < 0.0001f);

    for (i = 0; i < GOLF_SPAWN_LAYOUT_MAX_PLAYERS; ++i) {
        for (j = i + 1u; j < GOLF_SPAWN_LAYOUT_MAX_PLAYERS; ++j) {
            float dx = offsets[i].right - offsets[j].right;
            float dz = offsets[i].forward - offsets[j].forward;
            assert(sqrtf(dx * dx + dz * dz) + 0.0001f >= required_spacing);
        }
    }

    assert(offsets[0].right < 0.0f && offsets[15].right > 0.0f);
    assert(offsets[0].forward < 0.0f && offsets[15].forward > 0.0f);
    assert(!golf_spawn_layout_get_offset(16u, radius, gap, &offsets[0]));
    assert(!golf_spawn_layout_get_offset(0u, 0.0f, gap, &offsets[0]));
    assert(!golf_spawn_layout_get_offset(0u, radius, -1.0f, &offsets[0]));
    assert(!golf_spawn_layout_get_offset(0u, radius, gap, NULL));

    puts("PASS: 16 deterministic tee slots remain separated with valid input");
    return 0;
}
