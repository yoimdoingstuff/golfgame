# golfgame

A compact, portable Windows multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. **The golf balls are the players.** There are no golfer avatars, humanoid rigs, character movement systems, or Plaza/social-hub world.

The graphical client uses the upstream Open-Golf engine, course renderer, UI, physics, level loader and asset pipeline. Golfgame keeps its C11 LAN networking layer and replaces only the upstream application entry point to initialise and update that layer.

## Current state

- Open-Golf is pinned as a Git submodule under vendor/Open-Golf.
- The Windows client is built as golfgame.exe with a 1280x720 landscape window.
- The upstream game's main menu, course selector, course data, renderer, physics, audio and editor code are used.
- A front-end Multiplayer panel is available on the main menu with player/room name fields, LAN hosting and discovery, direct-address join, discovered-room join buttons, a player roster, host-only Start Match and Leave Room. The Multiplayer diagnostics tab remains available in the debug console (grave/backtick key).
- The existing network protocol is still a prototype and remote players are not yet represented as independent Open-Golf physics balls. Linking the library alone does not make the game fully multiplayer.

## Clone

Clone with submodules so the upstream game source and its original notices are present:

    git clone --recurse-submodules https://github.com/yoimdoingstuff/golfgame.git
    cd golfgame

For an existing checkout:

    git submodule update --init --recursive

## Build

Requires CMake 3.16+ and a C11/C++11 compiler.

### Windows (Visual Studio)

    cmake -S . -B build -A x64
    cmake --build build --config Release --target golf tg_net_demo

The game is build/golfgame.exe. The network test utility is build/Release/tg_net_demo.exe. Runtime course and asset files are copied to build/data.

### Linux (game and network test)

Install the X11/OpenGL development libraries used by upstream Open-Golf, then:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target golf tg_net_demo
    sh ./scripts/test-local.sh

The Linux build uses the upstream Sokol OpenGL 3.3 backend.

## Visual CI logs

Windows CI compiles and packages the actual Windows executable. A separate Linux visual job builds the same Open-Golf game with Mesa software OpenGL under Xvfb, then captures startup, main menu, course selection, gameplay and network diagnostics screens. It uploads PNGs, runtime stdout/stderr and a capture report under the golfgame-visual-logs artifact.

The Linux captures validate the shared renderer/UI, course loading and the main-menu multiplayer lobby, including creation of a local room. A configure-time patch bounds two unsafe Open-Golf path-extension copies that previously caused a startup stack-buffer overflow. They do not replace a real Windows hardware/runtime test.

Open the latest GitHub Actions run and download the golfgame-visual-logs artifact:
https://github.com/yoimdoingstuff/golfgame/actions

The logs directory is ignored by Git so generated screenshots do not pollute source history.

## Multiplayer design

- LAN discovery and direct IPv4/hostname join use UDP port 27890.
- Rooms support up to 16 players including the host.
- Every player is intended to putt simultaneously and be represented by one golf ball.
- Cosmetics are visual-only and never affect physics, collision or shot power.
- Managed networks can isolate clients or block peer traffic. No transport guarantees connectivity on restricted networks.
- Internet relay and native Windows Bluetooth transport are not implemented.

Before public internet play, the networking protocol needs host-authoritative simulation, shot and score validation, sequence numbers, rate limiting, robust serialisation, compatibility handling and reconnect support.

## Project documents

- TODO.md: short actionable priorities.
- AGENTS.md: instructions for future coding agents.
- docs/PROJECT_PLAN.md: milestone roadmap.
- docs/OPEN_GOLF_INTEGRATION.md: upstream integration and multiplayer adaptation notes.
- docs/THIRD_PARTY_NOTICES.md: licence and attribution guide.
- docs/COURSE_DIRECTION.md and docs/ART_STYLE_AND_HUD.md: game feel and presentation direction.
- docs/COSMETICS_AND_PROGRESSION.md: ball cosmetics and win rewards.
