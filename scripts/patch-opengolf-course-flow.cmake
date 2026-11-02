if(NOT DEFINED OPENGOLF_DIR)
    message(FATAL_ERROR "OPENGOLF_DIR is required.")
endif()
set(_ui_c "${OPENGOLF_DIR}/src/golf/ui.c")
set(_golf_c "${OPENGOLF_DIR}/src/golf/golf.c")
if(NOT EXISTS "${_ui_c}" OR NOT EXISTS "${_golf_c}")
    message(FATAL_ERROR "Open-Golf course-flow source files were not found.")
endif()

file(READ "${_ui_c}" _ui)
set(_selector_anchor [=[
    int num_levels = (int)CFG_NUM(game_cfg, "num_levels");
    int num_rows = (int)ceilf((float)num_levels / buttons_per_row);
]=])
set(_selector_patch [=[
    /* A single Golfgame course pack contains all 18 sequential holes. */
    int num_levels = 1;
    int num_rows = 1;
]=])
string(FIND "${_ui}" "${_selector_patch}" _already)
if(_already EQUAL -1)
    string(FIND "${_ui}" "${_selector_anchor}" _pos)
    if(_pos EQUAL -1)
        message(FATAL_ERROR "Could not find the original course-selector anchor.")
    endif()
    string(REPLACE "${_selector_anchor}" "${_selector_patch}" _ui "${_ui}")
endif()

set(_y_anchor "button_pos.y = button_pos.y - 0.5f * bg_size.y + row * button_size.y + 0.5f * button_size.y + row * button_padding;")
set(_y_patch "${_y_anchor}\n            if (num_levels == 1) button_pos.y = bg_pos.y;")
string(FIND "${_ui}" "${_y_patch}" _y_already)
if(_y_already EQUAL -1)
    string(FIND "${_ui}" "${_y_anchor}" _y_pos)
    if(_y_pos EQUAL -1)
        message(FATAL_ERROR "Could not find the course-card position anchor.")
    endif()
    string(REPLACE "${_y_anchor}" "${_y_patch}" _ui "${_ui}")
endif()

string(REPLACE "snprintf(text_buf, 256, \"Best -\");" "snprintf(text_buf, 256, \"18 HOLES\");" _ui "${_ui}")
string(REPLACE "snprintf(num_text, 16, \"%d\", button_num + 1);" "snprintf(num_text, 16, \"WORKSHOP\");" _ui "${_ui}")
string(REPLACE "\"LEVEL %d\"" "\"HOLE %d\"" _ui "${_ui}")
string(FIND "${_ui}" "\"WORKSHOP\"" _workshop_pos)
if(_workshop_pos EQUAL -1)
    message(FATAL_ERROR "The Workshop course-pack label was not applied.")
endif()
file(WRITE "${_ui_c}" "${_ui}")

file(READ "${_golf_c}" _golf)
string(REPLACE "data/levels/level-1.level" "data/levels/workshop-01.level" _golf "${_golf}")
string(FIND "${_golf}" "data/levels/workshop-01.level" _entry_pos)
if(_entry_pos EQUAL -1)
    message(FATAL_ERROR "Could not set Workshop as the initial menu scene.")
endif()
file(WRITE "${_golf_c}" "${_golf}")
message(STATUS "Applied Golfgame 18-hole course-pack progression.")
