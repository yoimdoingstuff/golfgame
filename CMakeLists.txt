cmake_minimum_required(VERSION 3.16)
project(golfgame LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(GOLFGAME_BUILD_OPEN_GOLF "Build the integrated Open-Golf game client" ON)

add_library(tg_network STATIC src/network/tg_network.c)
target_include_directories(tg_network PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/network")

if(WIN32)
    target_link_libraries(tg_network PUBLIC ws2_32)
    target_compile_definitions(tg_network PRIVATE WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_definitions(tg_network PRIVATE _POSIX_C_SOURCE=200809L)
endif()

add_executable(tg_net_demo src/demo/net_demo.c)
target_link_libraries(tg_net_demo PRIVATE tg_network)
if(NOT WIN32)
    target_compile_definitions(tg_net_demo PRIVATE _POSIX_C_SOURCE=200809L)
endif()

if(GOLFGAME_BUILD_OPEN_GOLF)
    set(OPENGOLF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Open-Golf")
    if(NOT EXISTS "${OPENGOLF_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "Open-Golf submodule is missing. Run: git submodule update --init --recursive")
    endif()

    add_subdirectory("${OPENGOLF_DIR}" "${CMAKE_BINARY_DIR}/opengolf")

    if(NOT TARGET golf)
        message(FATAL_ERROR "The pinned Open-Golf source did not define the expected 'golf' target.")
    endif()

    # Replace only the upstream platform entry point. Renderer, UI, course,
    # physics, data and audio source remain from the pinned upstream project.
    get_target_property(_golf_sources golf SOURCES)
    list(FILTER _golf_sources EXCLUDE REGEX "(^|/)main\\.c$")
    set_property(TARGET golf PROPERTY SOURCES "${_golf_sources}")
    target_sources(golf PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/integration/opengolf_main.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/integration/golf_multiplayer.c")
    target_include_directories(golf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(golf PRIVATE tg_network)
    set_target_properties(golf PROPERTIES OUTPUT_NAME "golfgame")

    add_custom_command(TARGET golf POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DOPENGOLF_DATA_DIR=${OPENGOLF_DIR}/data"
            "-DDESTINATION_DIR=$<TARGET_FILE_DIR:golf>/data"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/copy-opengolf-data.cmake"
        COMMENT "Copying Open-Golf course and runtime data")
elseif(WIN32)
    message(FATAL_ERROR "The Windows game target requires GOLFGAME_BUILD_OPEN_GOLF=ON.")
else()
    message(STATUS "Open-Golf game disabled; building the standalone networking test target only.")
endif()

if(MSVC)
    target_compile_options(tg_network PRIVATE /W4)
    target_compile_options(tg_net_demo PRIVATE /W4)
else()
    target_compile_options(tg_network PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tg_net_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
