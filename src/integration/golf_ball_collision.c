#include "integration/golf_ball_collision.h"

#include <math.h>

#define GOLF_BALL_COLLISION_EPSILON 0.000001f
#define GOLF_BALL_COLLISION_SLOP 0.001f

static int finite_body(const golf_ball_collision_body *body) {
    int i;
    if (!body || !isfinite(body->radius) || body->radius <= 0.0f ||
        !isfinite(body->inverse_mass) || body->inverse_mass < 0.0f) {
        return 0;
    }
    for (i = 0; i < 3; ++i) {
        if (!isfinite(body->position[i]) || !isfinite(body->velocity[i])) {
            return 0;
        }
    }
    return 1;
}

golf_ball_collision_result golf_ball_collision_resolve(
    golf_ball_collision_body *a,
    golf_ball_collision_body *b,
    float restitution,
    float correction_percent) {
    golf_ball_collision_result result = {0};
    float delta[3];
    float relative_velocity[3];
    float distance_squared = 0.0f;
    float distance;
    float radius_sum;
    float inverse_mass_sum;
    float normal_speed;
    float correction;
    int i;

    if (!finite_body(a) || !finite_body(b) ||
        !isfinite(restitution) || restitution < 0.0f || restitution > 1.0f ||
        !isfinite(correction_percent) ||
        correction_percent < 0.0f || correction_percent > 1.0f) {
        return result;
    }

    inverse_mass_sum = a->inverse_mass + b->inverse_mass;
    if (!isfinite(inverse_mass_sum) ||
        inverse_mass_sum <= GOLF_BALL_COLLISION_EPSILON) {
        return result;
    }

    for (i = 0; i < 3; ++i) {
        delta[i] = a->position[i] - b->position[i];
        relative_velocity[i] = a->velocity[i] - b->velocity[i];
        distance_squared += delta[i] * delta[i];
    }
    if (!isfinite(distance_squared)) return result;

    radius_sum = a->radius + b->radius;
    if (!isfinite(radius_sum) || distance_squared >= radius_sum * radius_sum) {
        return result;
    }

    distance = sqrtf(distance_squared);
    if (distance > GOLF_BALL_COLLISION_EPSILON) {
        float inverse_distance = 1.0f / distance;
        for (i = 0; i < 3; ++i) {
            result.normal[i] = delta[i] * inverse_distance;
        }
    }
    else {
        float relative_speed_squared = 0.0f;
        for (i = 0; i < 3; ++i) {
            relative_speed_squared += relative_velocity[i] * relative_velocity[i];
        }
        if (relative_speed_squared > GOLF_BALL_COLLISION_EPSILON) {
            float inverse_relative_speed = 1.0f / sqrtf(relative_speed_squared);
            for (i = 0; i < 3; ++i) {
                result.normal[i] = -relative_velocity[i] * inverse_relative_speed;
            }
        }
        else {
            result.normal[0] = 1.0f;
            result.normal[1] = 0.0f;
            result.normal[2] = 0.0f;
        }
    }

    result.collided = 1;
    result.penetration = radius_sum - distance;

    correction = fmaxf(result.penetration - GOLF_BALL_COLLISION_SLOP, 0.0f) *
        correction_percent / inverse_mass_sum;
    for (i = 0; i < 3; ++i) {
        a->position[i] += result.normal[i] * correction * a->inverse_mass;
        b->position[i] -= result.normal[i] * correction * b->inverse_mass;
    }

    normal_speed = relative_velocity[0] * result.normal[0] +
        relative_velocity[1] * result.normal[1] +
        relative_velocity[2] * result.normal[2];
    if (normal_speed < -GOLF_BALL_COLLISION_EPSILON) {
        result.impulse = -(1.0f + restitution) * normal_speed / inverse_mass_sum;
        for (i = 0; i < 3; ++i) {
            a->velocity[i] += result.normal[i] * result.impulse * a->inverse_mass;
            b->velocity[i] -= result.normal[i] * result.impulse * b->inverse_mass;
        }
        result.impulse_applied = 1;
    }

    return result;
}
