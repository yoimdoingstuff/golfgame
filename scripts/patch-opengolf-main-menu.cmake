if(NOT DEFINED OPENGOLF_DIR)
    message(FATAL_ERROR "OPENGOLF_DIR is required.")
endif()
set(_ui_c "${OPENGOLF_DIR}/src/golf/ui.c")
if(NOT EXISTS "${_ui_c}")
    message(FATAL_ERROR "Open-Golf UI source was not found: ${_ui_c}")
endif()
file(READ "${_ui_c}" _contents)
set(_anchor [=[
    _golf_ui_text_name(layout, "main_text", 1);
    _golf_ui_text_name(layout, "main2_text", 1);
]=])
set(_replacement [=[
    _golf_ui_text_name(layout, "main_text", 1);
    _golf_ui_text_name(layout, "main2_text", 1);
    _golf_ui_text_name(layout, "main_tagline_text", 1);
]=])
string(FIND "${_contents}" "${_replacement}" _already)
if(_already EQUAL -1)
    string(FIND "${_contents}" "${_anchor}" _pos)
    if(_pos EQUAL -1)
        message(FATAL_ERROR "Could not find the main-menu title anchor.")
    endif()
    string(REPLACE "${_anchor}" "${_replacement}" _contents "${_contents}")
    file(WRITE "${_ui_c}" "${_contents}")
endif()
message(STATUS "Applied Golfgame main-menu tagline hook.")
