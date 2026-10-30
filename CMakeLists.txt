cmake_minimum_required(VERSION 3.16)
project(golfgame_network C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(tg_network STATIC src/network/tg_network.c)
target_include_directories(tg_network PUBLIC src/network)

if(WIN32)
    target_link_libraries(tg_network PUBLIC ws2_32)
    target_compile_definitions(tg_network PRIVATE WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)

    # Small native Win32 + OpenGL 1.1 client. Required graphics DLLs ship
    # with Windows; there is no engine runtime or third-party renderer DLL.
    add_executable(golfgame WIN32 src/desktop/win_main.c)
    target_link_libraries(golfgame PRIVATE tg_network opengl32 glu32 gdi32 user32)
    target_include_directories(golfgame PRIVATE src/network)
    target_compile_definitions(golfgame PRIVATE WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
endif()

add_executable(tg_net_demo src/demo/net_demo.c)
if(NOT WIN32)
    target_compile_definitions(tg_network PRIVATE _POSIX_C_SOURCE=200809L)
    target_compile_definitions(tg_net_demo PRIVATE _POSIX_C_SOURCE=200809L)
endif()
target_link_libraries(tg_net_demo PRIVATE tg_network)

if(MSVC)
    target_compile_options(tg_network PRIVATE /W4)
    target_compile_options(tg_net_demo PRIVATE /W4)
    if(WIN32)
        target_compile_options(golfgame PRIVATE /W4)
    endif()
else()
    target_compile_options(tg_network PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tg_net_demo PRIVATE -Wall -Wextra -Wpedantic)
    if(WIN32)
        target_compile_options(golfgame PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
