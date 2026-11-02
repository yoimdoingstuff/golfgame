# Agent instructions

Read this file before making changes.

## Product rules

- This is a compact Windows multiplayer minigolf game inspired by the minigolf minigame in GMod Tower, not the Tower Plaza.
- **The golf balls are the players.** Never add golfer avatars, humanoid rigs, character movement or a persistent social hub.
- Every player can putt at the same time. Do not add turn queues or a current-player turn system.
- The target is a lightweight portable Windows game. Do not introduce Unity, Godot, Electron, a browser runtime or a heavyweight engine without an explicit product decision.
- Golfgame owns the product identity, courses, progression and game-facing UI. Open-Golf is a technical renderer/physics/level-loader foundation, not the product or course catalogue.
- Create original Golfgame branding, course content and UI changes. Do not reuse proprietary GMod Tower or Tower Unite assets, maps, logos or exact interface artwork.
- Cosmetics are visual only. They must never affect physics, collision, visibility advantage or shot power.
- The original Workshop course pack is authored in assets/golfgame/workshop-course-pack.cmake. Keep its 18 holes, runtime config paths and HUD par table in sync.
- scripts/copy-opengolf-data.cmake must remove upstream sample levels from the staged runtime data. Do not reintroduce Open-Golf's sample levels into the game package.
- Use only ordinary LAN networking and explicitly permitted services. Never disguise traffic or bypass school/work network controls.
- Nearby-device connectivity should feel like LocalSend/AirDrop/Quick Share: discover nearby peers, show clear device/player identity, require user-approved pairing, and remember trusted peers only with consent.
- Prefer standards-based discovery (mDNS/DNS-SD and IPv4/IPv6 multicast/broadcast where supported) and direct peer-to-peer links. Keep manual IP/room-code joining as a reliable fallback.
- Think of the connection layer as a node graph: direct links are preferred; a room host coordinates the match; optional multi-hop game-packet relaying is a later, opt-in feature with explicit peer consent, hop limits, loop prevention, rate limits and encrypted/authenticated links.
- Do not promise interoperability with Apple's AirDrop or Google's Quick Share. Their device-discovery and transfer protocols are platform-specific/proprietary; implement a similar user experience using documented OS APIs and open standards instead.
- Bluetooth Low Energy is suitable for small discovery/pairing messages, not a default real-time gameplay transport. Investigate supported Bluetooth PAN or Wi-Fi Direct APIs separately and provide fallbacks when unavailable.
- Never create an unapproved tunnel, proxy or covert relay to get around school/work firewall or network policy. A relay is only for permitted game traffic and must be transparent to the user.

## Upstream integration

- vendor/Open-Golf is a pinned Git submodule. Its upstream project is MIT-licensed.
- Clone with submodules: git clone --recurse-submodules https://github.com/yoimdoingstuff/golfgame.git
- Existing checkout: git submodule update --init --recursive
- Do not copy the entire upstream repository into a second directory or silently remove its source/asset notices.
- Preserve vendor/Open-Golf/LICENSE and the licence files under vendor/Open-Golf/src/3rd_party and vendor/Open-Golf/data.
- docs/THIRD_PARTY_NOTICES.md is an index, not a substitute for actual upstream notices.
- CMake builds the upstream golf target and replaces its main.c platform entry point with src/integration/opengolf_main.c. Renderer, physics and level loader remain the engine foundation. Golfgame patches the course selector/title hooks and supplies its own level/config/UI content.
- scripts/patch-opengolf-file.cmake applies an idempotent, narrow safety patch to upstream src/common/file.c. Keep it until upstream fixes the bug and runtime checks pass.
- scripts/patch-opengolf-course-flow.cmake and scripts/patch-opengolf-main-menu.cmake are intentional, idempotent source patches. Update their anchors carefully when the pinned upstream submodule changes.

## Current architecture

- vendor/Open-Golf/src/golf: upstream renderer, UI drawing, one-ball gameplay and course rendering foundation.
- vendor/Open-Golf/src/common: upstream geometry, level/data/asset/physics/common systems.
- vendor/Open-Golf/src/3rd_party: upstream bundled dependencies.
- vendor/Open-Golf/data: common engine models, textures, shaders, fonts and audio. Its sample level files are not staged into Golfgame's runtime package.
- assets/golfgame/workshop-course-pack.cmake: original 18-hole level data source.
- assets/golfgame/config/game.cfg and assets/golfgame/ui/ui.ui: Golfgame progression config and game-facing UI layout.
- src/integration/opengolf_main.c: Golfgame application entry point adapted from upstream main.c. Keep the upstream MIT attribution comment and update it if upstream source changes.
- src/integration/golf_multiplayer.c/.h: LAN lifecycle/UI, hole HUD, player list and replicated course-start adapter.
- src/integration/golf_remote_balls.c: remote ball snapshot renderer; this is presentation only, not shared ball physics.
- src/network/tg_network.c/.h: Golfgame's C11 UDP LAN layer.
- src/demo/net_demo.c: command-line network test tool.
- src/desktop/win_main.c: old prototype, not the primary game target.
- CMakeLists.txt: builds the engine-based game as golfgame.exe on Windows and golfgame on Linux.
- scripts/copy-opengolf-data.cmake: stages shared engine runtime assets and only Golfgame course levels/config/UI.
- scripts/capture-linux.sh: launches the Linux game under Xvfb/Mesa and captures labelled visual checkpoints under logs/.
- .github/workflows/build.yml: Windows native build/package, Linux network smoke tests and Linux Mesa/Xvfb visual capture.

## Multiplayer limitations

The gameplay engine currently owns one golf ball per client. The lobby and course-start signal are networked, and remote ball snapshots can be displayed, but the upstream collision/input/camera/score systems are not converted to per-player state or a shared simulation. Do not claim the game is fully multiplayer. The next major gameplay task is independent ball state and collision for all players while retaining simultaneous putting.

The host currently relays client-reported state. It is not an authoritative or secure public-internet protocol. Treat client state as untrusted. Before public play, add sequence numbers, input validation, rate limits, host-side shot/stroke/result verification, robust serialisation and reconnect handling.

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

The visual workflow runs scripts/capture-linux.sh under Xvfb/Mesa software OpenGL and uploads golfgame-visual-logs even when capture fails. Inspect logs/screenshots/, capture-report.txt, runtime-stdout.txt and runtime-stderr.txt from that artifact after UI, renderer, level or menu changes.

Required checkpoints:
- Startup / main menu with Golfgame branding and matchmaking panel
- Single Workshop course-pack selection card
- Hole 1 gameplay and hole HUD
- Multiplayer diagnostics console

These Linux captures validate the shared engine renderer and Golfgame content pipeline but do not prove Windows driver/runtime compatibility. A successful compile is not a successful play-test. Be explicit about what checks passed.

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
