#ifndef GOLFGAME_BALL_COLLISION_H
#define GOLFGAME_BALL_COLLISION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct golf_ball_collision_body {
    float position[3];
    float velocity[3];
    float radius;
    float inverse_mass;
} golf_ball_collision_body;

typedef struct golf_ball_collision_result {
    int collided;
    int impulse_applied;
    float penetration;
    float impulse;
    float normal[3];
} golf_ball_collision_result;

/*
 * Resolve one sphere contact. The normal points from body B toward body A.
 * Position correction and velocity impulses are distributed by inverse mass.
 * Returns an all-zero result for invalid input or non-overlapping spheres.
 */
golf_ball_collision_result golf_ball_collision_resolve(
    golf_ball_collision_body *a,
    golf_ball_collision_body *b,
    float restitution,
    float correction_percent);

#ifdef __cplusplus
}
#endif

#endif
