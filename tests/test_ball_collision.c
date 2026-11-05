#include "integration/golf_ball_collision.h"

#include <math.h>
#include <stdio.h>

static int failures;

#define CHECK(condition, name) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s (line %d)\n", (name), __LINE__); \
        ++failures; \
    } \
} while (0)

static golf_ball_collision_body body(float x, float y, float z,
        float vx, float vy, float vz, float inverse_mass) {
    golf_ball_collision_body value = {
        {x, y, z}, {vx, vy, vz}, 0.12f, inverse_mass
    };
    return value;
}

static int near_float(float a, float b, float tolerance) {
    return fabsf(a - b) <= tolerance;
}

static void test_head_on_equal_mass_collision(void) {
    golf_ball_collision_body a = body(-0.10f, 0, 0, 2.0f, 0, 0, 1.0f);
    golf_ball_collision_body b = body( 0.10f, 0, 0,-1.0f, 0, 0, 1.0f);
    golf_ball_collision_result result =
        golf_ball_collision_resolve(&a, &b, 0.5f, 0.8f);

    CHECK(result.collided, "overlapping spheres collide");
    CHECK(result.impulse_applied, "approaching spheres receive an impulse");
    CHECK(near_float(a.velocity[0], -0.25f, 0.0001f),
        "equal-mass head-on collision changes local velocity");
    CHECK(near_float(b.velocity[0], 1.25f, 0.0001f),
        "equal-mass head-on collision changes peer velocity symmetrically");
    CHECK(a.position[0] < -0.10f && b.position[0] > 0.10f,
        "penetration correction separates both spheres");
    CHECK(near_float(a.velocity[0] + b.velocity[0], 1.0f, 0.0001f),
        "collision conserves linear momentum for equal masses");
}

static void test_separating_overlap_does_not_apply_impulse(void) {
    golf_ball_collision_body a = body(0.0f, 0, 0, -1.0f, 0, 0, 1.0f);
    golf_ball_collision_body b = body(0.20f, 0, 0,  1.0f, 0, 0, 1.0f);
    golf_ball_collision_result result =
        golf_ball_collision_resolve(&a, &b, 0.6f, 0.8f);

    CHECK(result.collided, "overlap is detected while separating");
    CHECK(!result.impulse_applied, "separating spheres receive no bounce impulse");
    CHECK(a.velocity[0] == -1.0f && b.velocity[0] == 1.0f,
        "separating velocities are unchanged");
    CHECK(a.position[0] < 0.0f && b.position[0] > 0.20f,
        "separating overlap is still corrected");
}

static void test_non_overlapping_spheres_are_unchanged(void) {
    golf_ball_collision_body a = body(0.0f, 0, 0, 2.0f, 0, 0, 1.0f);
    golf_ball_collision_body b = body(1.0f, 0, 0, 0.0f, 0, 0, 1.0f);
    golf_ball_collision_result result =
        golf_ball_collision_resolve(&a, &b, 0.5f, 0.8f);

    CHECK(!result.collided, "separated spheres do not collide");
    CHECK(a.position[0] == 0.0f && a.velocity[0] == 2.0f,
        "non-contact leaves body A unchanged");
    CHECK(b.position[0] == 1.0f && b.velocity[0] == 0.0f,
        "non-contact leaves body B unchanged");
}

static void test_immovable_peer_receives_full_response(void) {
    golf_ball_collision_body a = body(-0.20f, 0, 0, 2.0f, 0, 0, 1.0f);
    golf_ball_collision_body b = body( 0.00f, 0, 0, 0.0f, 0, 0, 0.0f);
    golf_ball_collision_result result =
        golf_ball_collision_resolve(&a, &b, 0.5f, 0.8f);

    CHECK(result.collided && result.impulse_applied,
        "dynamic body hits an immovable body");
    CHECK(near_float(a.velocity[0], -1.0f, 0.0001f),
        "dynamic body receives the complete impulse");
    CHECK(b.velocity[0] == 0.0f && b.position[0] == 0.0f,
        "zero-inverse-mass body remains fixed");
}

static void test_coincident_centres_use_stable_fallback(void) {
    golf_ball_collision_body a = body(0, 0, 0, 1.0f, 0, 0, 1.0f);
    golf_ball_collision_body b = body(0, 0, 0,-1.0f, 0, 0, 1.0f);
    golf_ball_collision_result result =
        golf_ball_collision_resolve(&a, &b, 0.5f, 0.8f);

    CHECK(result.collided && result.impulse_applied,
        "coincident centres still resolve");
    CHECK(isfinite(a.position[0]) && isfinite(b.position[0]),
        "coincident fallback keeps positions finite");
    CHECK(near_float(a.velocity[0], -0.5f, 0.0001f) &&
          near_float(b.velocity[0], 0.5f, 0.0001f),
        "coincident fallback produces a separating impulse");
}

static void test_invalid_input_fails_closed(void) {
    golf_ball_collision_body a = body(0, 0, 0, 1, 0, 0, 1);
    golf_ball_collision_body b = body(0.1f, 0, 0, 0, 0, 0, 1);
    golf_ball_collision_result result;

    a.position[0] = NAN;
    result = golf_ball_collision_resolve(&a, &b, 0.5f, 0.8f);
    CHECK(!result.collided && !result.impulse_applied,
        "non-finite positions are rejected");
    CHECK(isnan(a.position[0]), "invalid input is not partially mutated");

    a = body(0, 0, 0, 1, 0, 0, 1);
    result = golf_ball_collision_resolve(&a, &b, 1.1f, 0.8f);
    CHECK(!result.collided, "invalid restitution is rejected");
}

int main(void) {
    test_head_on_equal_mass_collision();
    test_separating_overlap_does_not_apply_impulse();
    test_non_overlapping_spheres_are_unchanged();
    test_immovable_peer_receives_full_response();
    test_coincident_centres_use_stable_fallback();
    test_invalid_input_fails_closed();

    if (failures != 0) {
        fprintf(stderr, "%d ball-collision test(s) failed.\n", failures);
        return 1;
    }
    puts("PASS: ball collision impulse, separation, mass, fallback and validation");
    return 0;
}
