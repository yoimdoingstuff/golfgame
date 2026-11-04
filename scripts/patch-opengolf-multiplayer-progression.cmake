if(NOT DEFINED OPENGOLF_DIR)
    message(FATAL_ERROR "OPENGOLF_DIR is required.")
endif()
set(_ui_c "${OPENGOLF_DIR}/src/golf/ui.c")
if(NOT EXISTS "${_ui_c}")
    message(FATAL_ERROR "Open-Golf UI source was not found: ${_ui_c}")
endif()

file(READ "${_ui_c}" _ui)
string(REPLACE "\r\n" "\n" _ui "${_ui}")

set(_include_anchor "#include \"golf/golf.h\"")
set(_include_patch "${_include_anchor}\n#include \"integration/golf_multiplayer.h\"")
string(FIND "${_ui}" "#include \"integration/golf_multiplayer.h\"" _include_done)
if(_include_done EQUAL -1)
    string(FIND "${_ui}" "${_include_anchor}" _include_pos)
    if(_include_pos EQUAL -1)
        message(FATAL_ERROR "Could not find the Open-Golf UI include anchor.")
    endif()
    string(REPLACE "${_include_anchor}" "${_include_patch}" _ui "${_ui}")
endif()

set(_button_anchor [=[
        if (_golf_ui_button_name(layout, "finished_menu_next_button")) {
            _golf_ui_start_fade_out(false, true, golf->level_num + 1, false);
        }
]=])
set(_button_patch [=[
        if (_golf_ui_button_name(layout, "finished_menu_next_button")) {
            /* Networked matches wait until every player is ready. */
            if (!golf_mp_request_next_hole(golf->level_num + 1)) {
                _golf_ui_start_fade_out(false, true, golf->level_num + 1, false);
            }
        }
]=])
string(FIND "${_ui}" "${_button_patch}" _already)
if(_already EQUAL -1)
    string(FIND "${_ui}" "${_button_anchor}" _button_pos)
    if(_button_pos EQUAL -1)
        message(FATAL_ERROR "Could not find the finished-screen Next Hole button anchor.")
    endif()
    string(REPLACE "${_button_anchor}" "${_button_patch}" _ui "${_ui}")
endif()

file(WRITE "${_ui_c}" "${_ui}")
message(STATUS "Applied Golfgame synchronized next-hole ready-up hook.")
