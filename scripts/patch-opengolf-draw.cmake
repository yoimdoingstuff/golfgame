if(NOT DEFINED OPENGOLF_DIR)
    message(FATAL_ERROR "OPENGOLF_DIR is required.")
endif()

set(_draw_c "${OPENGOLF_DIR}/src/golf/draw.c")
if(NOT EXISTS "${_draw_c}")
    message(FATAL_ERROR "Open-Golf draw.c was not found: ${_draw_c}")
endif()

file(READ "${_draw_c}" _contents)
string(REPLACE "\r\n" "\n" _contents "${_contents}")
string(FIND "${_contents}" "golf_mp_draw_remote_balls();" _hook_pos)
if(NOT _hook_pos EQUAL -1)
    message(STATUS "Open-Golf remote-ball draw hook is already applied.")
    return()
endif()

set(_include_anchor "#include \"golf/ui.h\"")
string(FIND "${_contents}" "${_include_anchor}" _include_pos)
if(_include_pos EQUAL -1)
    message(FATAL_ERROR "Could not find Open-Golf draw.c include anchor.")
endif()
string(REPLACE "${_include_anchor}" "${_include_anchor}\n#include \"integration/golf_multiplayer.h\"" _contents "${_contents}")

set(_draw_anchor [=[
            sg_draw(0, model->positions.length, 1);
        }

        // Draw the ball hidden behind objects
]=])
set(_draw_replacement [=[
            sg_draw(0, model->positions.length, 1);
        }

        /* Draw networked player balls in the same depth-tested course pass. */
        golf_mp_draw_remote_balls();

        // Draw the ball hidden behind objects
]=])
string(FIND "${_contents}" "${_draw_anchor}" _draw_pos)
if(_draw_pos EQUAL -1)
    message(FATAL_ERROR "Could not find the local ball draw anchor in Open-Golf draw.c.")
endif()
string(REPLACE "${_draw_anchor}" "${_draw_replacement}" _contents "${_contents}")

file(WRITE "${_draw_c}" "${_contents}")
message(STATUS "Applied Open-Golf remote-ball draw hook.")
