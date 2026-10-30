cmake_minimum_required(VERSION 3.16)
project(golfgame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Open-Golf uses the non-standard M_PI constant in its shared math header.
# Strict C11 mode on glibc hides it unless the default feature set is enabled.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(_DEFAULT_SOURCE)
endif()
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

    # Fix an upstream stack-buffer overflow in golf_file_append_extension()
    # before compiling the vendored source. This patch is idempotent and fails
    # closed if the pinned source no longer matches the code we audited.
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DOPENGOLF_DIR=${OPENGOLF_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/patch-opengolf-file.cmake"
        RESULT_VARIABLE _opengolf_file_patch_result)
    if(NOT _opengolf_file_patch_result EQUAL 0)
        message(FATAL_ERROR "Could not safely patch the Open-Golf file path helper.")
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

    # CMake only permits TARGET POST_BUILD declarations in the directory
    # where the target was created. Open-Golf creates golf in its own
    # subdirectory, so stage the data through a normal build dependency.
    add_custom_target(golfgame_runtime_data
        COMMAND "${CMAKE_COMMAND}"
            "-DOPENGOLF_DATA_DIR=${OPENGOLF_DIR}/data"
            "-DDESTINATION_DIR=${CMAKE_BINARY_DIR}/data"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/copy-opengolf-data.cmake"
        COMMENT "Copying Open-Golf course and runtime data")
    add_dependencies(golf golfgame_runtime_data)
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
