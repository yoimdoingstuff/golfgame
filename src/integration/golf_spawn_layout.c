#include "golf_spawn_layout.h"

#include <math.h>

int golf_spawn_layout_get_offset(unsigned slot_index, float ball_radius,
    float separation_gap, golf_spawn_layout_offset *out_offset) {
    const float grid_width = 4.0f;
    const float spacing = ball_radius * 2.0f + separation_gap;
    const unsigned column = slot_index % 4u;
    const unsigned row = slot_index / 4u;

    if (!out_offset || slot_index >= GOLF_SPAWN_LAYOUT_MAX_PLAYERS ||
        !isfinite(ball_radius) || ball_radius <= 0.0f ||
        !isfinite(separation_gap) || separation_gap < 0.0f ||
        !isfinite(spacing) || spacing <= 0.0f) {
        return 0;
    }

    out_offset->right = ((float)column - (grid_width - 1.0f) * 0.5f) * spacing;
    out_offset->forward = ((float)row - (grid_width - 1.0f) * 0.5f) * spacing;
    return 1;
}
