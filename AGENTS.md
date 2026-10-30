# Agent instructions

Read this file before making changes.

## Product rules

- This is a compact Windows multiplayer minigolf game inspired by the minigolf minigame in GMod Tower, not the Tower Plaza.
- **The golf balls are the players.** Never add golfer avatars, humanoid rigs, character movement or a persistent social hub.
- Every player can putt at the same time. Do not add turn queues or a current-player turn system.
- The target is a lightweight portable Windows game. Do not introduce Unity, Godot, Electron, a browser runtime or a heavyweight engine without an explicit product decision.
- Create original Golfgame branding, course content and UI changes. Do not reuse proprietary GMod Tower or Tower Unite assets, maps, logos or exact interface artwork.
- Cosmetics are visual only. They must never affect physics, collision, visibility advantage or shot power.
- Use only ordinary LAN networking and explicitly permitted services. Never disguise traffic or bypass school/work network controls.

## Upstream integration

- vendor/Open-Golf is a pinned Git submodule. Its upstream project is MIT-licensed.
- Clone with submodules: git clone --recurse-submodules https://github.com/yoimdoingstuff/golfgame.git
- Existing checkout: git submodule update --init --recursive
- Do not copy the entire upstream repository into a second directory or silently remove its source/asset notices.
- Preserve vendor/Open-Golf/LICENSE and the licence files under vendor/Open-Golf/src/3rd_party and vendor/Open-Golf/data.
- docs/THIRD_PARTY_NOTICES.md is an index, not a substitute for actual upstream notices.
- CMake builds the upstream golf target and replaces only its main.c platform entry point with src/integration/opengolf_main.c. Renderer, UI, course, physics, data and audio code stay upstream unless an intentional patch is documented.
- `scripts/patch-opengolf-file.cmake` applies an idempotent, narrow safety patch to upstream `src/common/file.c`. The upstream `golf_file_append_extension()` copied an entire path-buffer capacity from the end of the existing path, overrunning the stack buffer during data indexing. Keep this patch until upstream fixes the bug and the runtime screenshot test passes.

## Current architecture

- vendor/Open-Golf/src/golf: upstream game logic, renderer, UI and one-ball gameplay.
- vendor/Open-Golf/src/common: upstream level/data/asset/physics/common systems.
- vendor/Open-Golf/src/3rd_party: upstream bundled dependencies.
- vendor/Open-Golf/data: upstream levels, models, textures, UI data, fonts and audio.
- src/integration/opengolf_main.c: Golfgame application entry point adapted from upstream main.c. Keep the upstream MIT attribution comment and update it if the upstream source changes.
- src/integration/golf_multiplayer.c/.h: adapter that initialises, updates and shuts down tg_network, provides the main-menu Multiplayer panel, handles replicated course-start events and retains a diagnostics tab.
- src/network/tg_network.c/.h: Golfgame's C11 UDP LAN layer.
- src/demo/net_demo.c: command-line network test tool.
- src/desktop/win_main.c: old prototype, not the primary game target.
- CMakeLists.txt: builds the Open-Golf target as golfgame.exe on Windows and golfgame on Linux.
- scripts/copy-opengolf-data.cmake: copies runtime assets and applies the Golfgame title to the menu data in the build directory.
- scripts/capture-linux.sh: launches the Linux game under Xvfb/Mesa and captures labelled visual checkpoints under logs/.
- scripts/capture-windows.ps1: optional local Windows screenshot helper; the hosted Windows runner is not used for interactive visual capture.
- .github/workflows/build.yml: Windows native build/package, Linux network smoke tests and Linux Mesa/Xvfb visual capture.

## Multiplayer limitations

Open-Golf's gameplay currently owns one golf ball in one golf_game_t. The Golfgame network module can form rooms, synchronise course start and relay ball snapshots, but the upstream collision, input, camera, score and render systems are not converted to multiple simultaneous balls yet. Do not claim the game is fully multiplayer. The next major architectural task is to make ball state per player and feed every ball through the physics/render path while preserving the current single-player game flow as a test mode.

The host currently relays client-reported state. It is not an authoritative or secure public-internet protocol. Treat client input/state as untrusted. Before public play, add sequence numbers, input validation, rate limits, host-side shot/stroke/result verification, robust packet serialisation and reconnect handling.

## Build and test

### Windows

    git submodule update --init --recursive
    cmake -S . -B build -A x64
    cmake --build build --config Release --target golf tg_net_demo

Game executable: build/golfgame.exe. Runtime assets: build/data.

### Linux game

Install the X11/OpenGL/ALSA development packages used in the CI workflow, then:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target golf tg_net_demo

### Linux networking smoke test

    cmake -S . -B build -DGOLFGAME_BUILD_OPEN_GOLF=OFF -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target tg_net_demo
    sh ./scripts/test-local.sh

## Visual testing

The visual workflow runs scripts/capture-linux.sh under Xvfb with Mesa software OpenGL and uploads golfgame-visual-logs even when capture fails. Inspect logs/screenshots/, capture-report.txt, runtime-stdout.txt and runtime-stderr.txt from that artifact after UI, rendering, data or startup changes.

Required checkpoints:
- Startup / main menu
- Course selector
- Loaded course/gameplay
- Multiplayer diagnostics console

These Linux captures validate the shared UI and renderer but do not prove Windows driver/runtime compatibility. A successful compile is not a successful screenshot test or physical Windows play-test. Be explicit about which checks passed.

## Change workflow

1. Read AGENTS.md, TODO.md and relevant docs.
2. Inspect actual source before trusting old roadmap statements or assumptions.
3. Make a focused change and preserve the product rules.
4. Keep C11 warning flags enabled.
5. Run the relevant build and tests. Network changes require the localhost smoke test.
6. For graphical changes, inspect the CI screenshots, stdout and stderr. When possible also run on a physical Windows machine.
7. Update TODO.md, README.md and design docs when implementation changes.
8. Review diffs for accidental binaries, generated logs, credentials, licence removals and unrelated changes.
9. Never call a compile successful playtesting. State exactly what was and was not tested.
