if(NOT DEFINED OPENGOLF_DATA_DIR OR NOT DEFINED DESTINATION_DIR OR NOT DEFINED GOLFGAME_DATA_DIR)
    message(FATAL_ERROR "OPENGOLF_DATA_DIR, DESTINATION_DIR and GOLFGAME_DATA_DIR are required.")
endif()
if(NOT EXISTS "${OPENGOLF_DATA_DIR}/static_data.static_data")
    message(FATAL_ERROR "Open-Golf runtime data directory is incomplete: ${OPENGOLF_DATA_DIR}")
endif()
if(NOT EXISTS "${GOLFGAME_DATA_DIR}/workshop-course-pack.cmake" OR
   NOT EXISTS "${GOLFGAME_DATA_DIR}/config/game.cfg" OR
   NOT EXISTS "${GOLFGAME_DATA_DIR}/ui/ui.ui")
    message(FATAL_ERROR "Golfgame course pack, config or UI assets are missing.")
endif()

file(REMOVE_RECURSE "${DESTINATION_DIR}")
file(MAKE_DIRECTORY "${DESTINATION_DIR}")
file(COPY "${OPENGOLF_DATA_DIR}/" DESTINATION "${DESTINATION_DIR}")

# Remove the bundled Open-Golf sample levels. The packaged game ships only Golfgame's authored holes.
file(REMOVE_RECURSE "${DESTINATION_DIR}/levels")
file(MAKE_DIRECTORY "${DESTINATION_DIR}/levels")
include("${GOLFGAME_DATA_DIR}/workshop-course-pack.cmake")
foreach(_index RANGE 1 ${GOLFGAME_COURSE_COUNT})
    if(_index LESS 10)
        set(_padded "0${_index}")
    else()
        set(_padded "${_index}")
    endif()
    set(_variable "GOLFGAME_COURSE_${_padded}")
    file(WRITE "${DESTINATION_DIR}/levels/workshop-${_padded}.level" "${${_variable}}")
endforeach()

file(COPY "${GOLFGAME_DATA_DIR}/config/game.cfg" DESTINATION "${DESTINATION_DIR}/config/")
file(COPY "${GOLFGAME_DATA_DIR}/ui/ui.ui" DESTINATION "${DESTINATION_DIR}/ui/")
message(STATUS "Staged Golfgame's original Workshop course pack, UI and 18-hole progression.")
