if(NOT DEFINED OPENGOLF_DATA_DIR OR NOT DEFINED DESTINATION_DIR)
    message(FATAL_ERROR "OPENGOLF_DATA_DIR and DESTINATION_DIR are required.")
endif()
if(NOT EXISTS "${OPENGOLF_DATA_DIR}/static_data.static_data")
    message(FATAL_ERROR "Open-Golf data directory is incomplete: ${OPENGOLF_DATA_DIR}")
endif()

file(REMOVE_RECURSE "${DESTINATION_DIR}")
file(MAKE_DIRECTORY "${DESTINATION_DIR}")
file(COPY "${OPENGOLF_DATA_DIR}/" DESTINATION "${DESTINATION_DIR}")

# Keep upstream's original layout and art while applying the project title.
set(_ui_file "${DESTINATION_DIR}/ui/ui.ui")
if(EXISTS "${_ui_file}")
    file(READ "${_ui_file}" _ui)
    string(REPLACE "OPEN GOLF" "GOLFGAME" _ui "${_ui}")
    file(WRITE "${_ui_file}" "${_ui}")
endif()
