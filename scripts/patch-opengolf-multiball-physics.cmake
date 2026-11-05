if(NOT DEFINED OPENGOLF_DIR)
    message(FATAL_ERROR "OPENGOLF_DIR is required.")
endif()

set(_game_h "${OPENGOLF_DIR}/src/golf/game.h")
set(_game_c "${OPENGOLF_DIR}/src/golf/game.c")
if(NOT EXISTS "${_game_h}" OR NOT EXISTS "${_game_c}")
    message(FATAL_ERROR "Open-Golf game.h/game.c was not found.")
endif()

file(READ "${_game_h}" _header)
file(READ "${_game_c}" _source)
string(REPLACE "\r\n" "\n" _header "${_header}")
string(REPLACE "\r\n" "\n" _source "${_source}")

if(_header MATCHES "golf_game_simulate_ball")
    message(STATUS "Open-Golf per-ball physics API is already applied.")
    return()
endif()

set(_ball_struct [=[
    struct {
        vec3 start_pos, pos, vel, draw_pos, rot_vec;
        quat orientation;
        float time_since_water_ripple, time_going_slow, time_out_of_bounds, radius, rot_vel,
              time_out_of_water, time_since_impact_sound;
        bool is_moving, is_in_hole, is_in_water, is_out_of_bounds;
    } ball;
]=])
string(FIND "${_header}" "${_ball_struct}" _ball_struct_pos)
if(_ball_struct_pos EQUAL -1)
    message(FATAL_ERROR "Could not find the pinned Open-Golf ball-state layout.")
endif()

set(_ball_typedef [=[
typedef struct golf_ball_state {
    vec3 start_pos, pos, vel, draw_pos, rot_vec;
    quat orientation;
    float time_since_water_ripple, time_going_slow, time_out_of_bounds, radius, rot_vel,
          time_out_of_water, time_since_impact_sound;
    bool is_moving, is_in_hole, is_in_water, is_out_of_bounds;
} golf_ball_state_t;

]=])
string(REPLACE "typedef struct golf_game {" "${_ball_typedef}typedef struct golf_game {" _header "${_header}")
string(REPLACE "${_ball_struct}" "    golf_ball_state_t ball;" _header "${_header}")
string(REPLACE
    "golf_game_t *golf_game_get(void);"
    "golf_game_t *golf_game_get(void);\nvoid golf_game_simulate_ball(golf_ball_state_t *ball, float dt);"
    _header "${_header}")

set(_physics_start "static void _physics_tick(float dt) {")
set(_physics_end "\nvoid golf_game_update(float dt) {")
string(FIND "${_source}" "${_physics_start}" _start)
string(FIND "${_source}" "${_physics_end}" _end)
if(_start EQUAL -1 OR _end EQUAL -1 OR _end LESS _start)
    message(FATAL_ERROR "Could not find the pinned Open-Golf physics tick.")
endif()
math(EXPR _body_length "${_end} - ${_start}")
string(SUBSTRING "${_source}" ${_start} ${_body_length} _body_original)
set(_body "${_body_original}")

string(REPLACE
    "static void _physics_tick(float dt) {"
    "static void _physics_tick_for_ball(float dt, golf_ball_state_t *ball, int effects_enabled, int record_history, int update_dynamic_bvh) {\n    if (!ball) return;"
    _body "${_body}")
string(REPLACE "game.ball." "ball->" _body "${_body}")

string(REPLACE
    "    // Create the BVH for entities that move"
    "    if (update_dynamic_bvh) {\n    // Create the BVH for entities that move"
    _body "${_body}")
string(REPLACE
    "    int num_contacts = 0;"
    "    }\n\n    int num_contacts = 0;"
    _body "${_body}")

string(REPLACE
    "if (ball->is_moving && num_contacts > 0) {"
    "if (record_history && ball->is_moving && num_contacts > 0) {"
    _body "${_body}")
string(REPLACE
    "if (contact->impulse_mag > 1 && contact->cull_dot < -0.15f) {"
    "if (effects_enabled && contact->impulse_mag > 1 && contact->cull_dot < -0.15f) {"
    _body "${_body}")
string(REPLACE
    "if (ball->time_since_water_ripple > CFG_NUM(game_cfg, \"water_ripple_frequency\")) {"
    "if (effects_enabled && ball->time_since_water_ripple > CFG_NUM(game_cfg, \"water_ripple_frequency\")) {"
    _body "${_body}")

set(_water_audio [=[
    if (ball->time_out_of_water < 0.1f) {
        golf_audio_start_sound("ball_in_water", "data/audio/in_water.ogg", 0.1f, true, false);
    }
    else {
        golf_audio_stop_sound("ball_in_water", 0.2f);
    }
]=])
string(FIND "${_body}" "${_water_audio}" _water_audio_pos)
if(_water_audio_pos EQUAL -1)
    message(FATAL_ERROR "Could not find the pinned Open-Golf water-audio block.")
endif()
set(_water_audio_replacement [=[
    if (effects_enabled) {
        if (ball->time_out_of_water < 0.1f) {
            golf_audio_start_sound("ball_in_water", "data/audio/in_water.ogg", 0.1f, true, false);
        }
        else {
            golf_audio_stop_sound("ball_in_water", 0.2f);
        }
    }
]=])
string(REPLACE "${_water_audio}" "${_water_audio_replacement}" _body "${_body}")

string(FIND "${_body}" "game.ball." _global_ball_reference_pos)
string(FIND "${_body}" "if (record_history && ball->is_moving && num_contacts > 0)" _history_guard_pos)
string(FIND "${_body}" "if (effects_enabled && contact->impulse_mag > 1 && contact->cull_dot < -0.15f)" _impact_guard_pos)
string(FIND "${_body}" "if (update_dynamic_bvh) {" _dynamic_bvh_guard_pos)
if(NOT _global_ball_reference_pos EQUAL -1)
    message(FATAL_ERROR "The extracted physics tick still contains a global ball reference.")
endif()
if(_history_guard_pos EQUAL -1 OR
   _impact_guard_pos EQUAL -1 OR
   _dynamic_bvh_guard_pos EQUAL -1)
    message(FATAL_ERROR "The per-ball physics side-effect guards were not applied.")
endif()

set(_physics_wrapper [=[
static void _physics_tick(float dt) {
    _physics_tick_for_ball(dt, &game.ball, 1, 1, 1);
}

void golf_game_simulate_ball(golf_ball_state_t *ball, float dt) {
    if (!ball || !golf || !golf->level || !isfinite(dt) ||
        dt <= 0.0f || dt > 0.1f) {
        return;
    }
    if (ball->is_in_hole || ball->is_out_of_bounds) return;

    _physics_tick_for_ball(dt, ball, 0, 0, 0);
    if (ball->is_in_hole || ball->is_out_of_bounds) {
        ball->vel = V3(0, 0, 0);
        ball->is_moving = false;
    }
}

]=])

string(REPLACE "${_body_original}" "${_body}\n\n${_physics_wrapper}" _source "${_source}")
if(NOT _source MATCHES "void golf_game_simulate_ball")
    message(FATAL_ERROR "Could not add the public per-ball physics wrapper.")
endif()

file(WRITE "${_game_h}" "${_header}")
file(WRITE "${_game_c}" "${_source}")
message(STATUS "Applied shared course-collision physics API for independent remote balls.")
