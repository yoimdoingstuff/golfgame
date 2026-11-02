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
# Replace the stock Play button and level-picker controls with Golfgame's own Workshop entry CTA.
string(FIND "${_ui}" "/* Golfgame custom front-end owns the Play and course-pack controls. */" _custom_menu_already)
if(_custom_menu_already EQUAL -1)
    set(_menu_start_anchor "static void _golf_ui_main_menu(float dt) {")
    set(_menu_end_anchor "static void _golf_ui_camera_controls(float dt) {")
    string(FIND "${_ui}" "${_menu_start_anchor}" _menu_start)
    string(FIND "${_ui}" "${_menu_end_anchor}" _menu_end)
    if(_menu_start EQUAL -1 OR _menu_end EQUAL -1 OR _menu_end LESS _menu_start)
        message(FATAL_ERROR "Could not locate the original Open-Golf main-menu function.")
    endif()
    string(SUBSTRING "${_ui}" 0 ${_menu_start} _menu_prefix)
    string(SUBSTRING "${_ui}" ${_menu_end} -1 _menu_suffix)
    set(_custom_main_menu [=[
static void _golf_ui_main_menu(float dt) {
    /* Golfgame custom front-end owns the Play and course-pack controls. */
    golf_ui_layout_t *layout = golf_data_get_ui_layout("data/ui/ui.ui");
    _golf_ui_text_name(layout, "main_text", 1);
    _golf_ui_text_name(layout, "main2_text", 1);
    _golf_ui_text_name(layout, "main_tagline_text", 1);

    float fade_in_length = CFG_NUM(game_cfg, "game_fade_in_length");
    if (golf->main_menu.t < fade_in_length) {
        float alpha = 1 - (golf->main_menu.t / fade_in_length);
        _golf_ui_draw_fade(alpha);
    }
}

]=])
    string(CONCAT _ui "${_menu_prefix}" "${_custom_main_menu}" "${_menu_suffix}")
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
