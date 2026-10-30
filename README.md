# golfgame

A compact, portable Windows multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. **The golf balls are the players.** There are no golfer avatars, humanoid rigs, character movement systems, or Plaza/social-hub world.

The graphical client now uses the upstream Open-Golf engine, course renderer, UI, physics, level loader and asset pipeline. Golfgame keeps its C11 LAN networking layer and replaces only the upstream application entry point to initialise and update that layer.

## Current state

- Open-Golf is pinned as a Git submodule under vendor/Open-Golf.
- The Windows client is built as golfgame.exe with a 1280x720 landscape window.
- The upstream game's main menu, course selector, course data, renderer, physics, audio and editor code are used.
- The multiplayer library is linked and its lifecycle is integrated. A Multiplayer diagnostics tab is available in the upstream debug console (press the grave/backtick key); it currently provides basic host, LAN discovery, localhost join, start and leave controls.
- The existing network protocol is still a prototype and remote players are not yet represented as independent Open-Golf physics balls. Linking the library alone does not make the game fully multiplayer.

## Clone

Clone with submodules so the upstream game source and its original notices are present:

    git clone --recurse-submodules https://github.com/yoimdoingstuff/golfgame.git
    cd golfgame

For an existing checkout:

    git submodule update --init --recursive

## Build

Requires CMake 3.16+ and a C11 compiler.

### Windows (Visual Studio)

    cmake -S . -B build -A x64
    cmake --build build --config Release --target golf tg_net_demo

The game is build/golfgame.exe. The network test utility is build/Release/tg_net_demo.exe. Runtime course and asset files are copied to build/data.

### Linux networking test

The integrated graphical game currently targets the Windows build workflow. The Linux job builds the standalone networking layer and runs the localhost smoke test:

    cmake -S . -B build -DGOLFGAME_BUILD_OPEN_GOLF=OFF -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target tg_net_demo
    sh ./scripts/test-local.sh

## Visual CI logs

The Windows Actions job launches the game, captures the main menu, course selector, gameplay and network debug console, and uploads them as the golfgame-visual-logs artifact. Screenshots and runtime stdout/stderr are written under logs/ in the Actions workspace. The logs directory is ignored by Git so generated screenshots do not pollute source history.

Open the latest GitHub Actions run and download the golfgame-visual-logs artifact to inspect the screenshots:
https://github.com/yoimdoingstuff/golfgame/actions

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
