# Project plan

## Product direction

A compact, match-based multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. This is **not** the full Tower/Plaza social hub. Players join a match and are represented by their golf balls.

## Locked-in targets

- Portable Windows x64 executable, distributed as a ZIP with no installer.
- Native C/CMake and a small runtime footprint; no heavyweight engine runtime.
- Open-Golf is the upstream graphics/gameplay foundation, pinned as a Git submodule under vendor/Open-Golf.
- A Golfgame-owned C11 UDP layer provides the current LAN transport prototype.
- Low-poly, colourful courses with exaggerated, readable obstacles.
- Angled third-person camera and the upstream course/level system.
- 2-16 players per match, including the host.
- Simultaneous minigolf: every player may aim and putt at the same time.
- Player identity is expressed through ball colour, optional hat/accessory and trail.
- Win progression unlocks cosmetics only; no physics upgrades.
- Original Golfgame branding and new content. Preserve upstream Open-Golf assets and all licence notices when they are used.

## Current implementation status

The graphical client is based on upstream Open-Golf. The upstream UI, Sokol renderer, level loader, course data, physics and audio are built from the pinned submodule. The project-owned application entry point is adapted from upstream main.c and initialises/updates/shuts down the Golfgame network module.

### Implemented

- Open-Golf submodule pinned at commit b9029f1e80c3110ae3d4fc9300854bdaeef96508.
- Upstream golf target built as golfgame.exe on Windows and golfgame on Linux.
- Upstream main menu, course selector, course rendering, level assets and physics.
- C11 UDP networking library and command-line demo.
- Network module lifecycle integrated into the game application.
- Multiplayer diagnostics tab in the built-in debug console, with host, LAN discovery, localhost join, start and leave controls.
- Windows build/package workflow and Linux localhost network smoke-test workflow.
- Linux Mesa/Xvfb visual-capture automation. Actual screenshot generation remains to be verified.

### Important limitations

- The diagnostics panel is not yet the normal player-facing multiplayer menu/lobby.
- Open-Golf gameplay still maintains a single golf ball. Remote players are not yet integrated into its physics, input, rendering, camera and scoring systems.
- The current UDP host relays client-reported state and is not authoritative or secure against modified clients.
- Multi-machine 2-16 player tests have not yet been completed.
- The Windows executable compiled in CI, but its first hosted-runner runtime launch crashed with access violation 0xC0000005 before a visible window was created. The Windows build remains the release target; Linux Mesa/Xvfb is used for automated visual smoke tests, not as a substitute for Windows hardware testing.

## Architecture

- vendor/Open-Golf: pinned upstream renderer, UI, game, physics, level system, data, audio and third-party dependencies.
- src/integration/opengolf_main.c: project-owned Sokol application entry point adapted from upstream main.c.
- src/integration/golf_multiplayer.c/.h: lifecycle bridge and debug-console controls for the network API.
- src/network/tg_network.h: public networking API and shared room/player structures.
- src/network/tg_network.c: UDP transport, packet definitions, discovery, join/leave, snapshots and match-start state.
- src/demo/net_demo.c: command-line networking test client.
- CMakeLists.txt: builds tg_network and adds the Open-Golf submodule target.
- scripts/copy-opengolf-data.cmake: copies runtime data to the build output and replaces the displayed product title in that generated copy.
- scripts/capture-linux.sh: launches the Linux game under Xvfb/Mesa and stores labelled screenshots and runtime logs under logs/.
- .github/workflows/build.yml: Windows build/package, Linux network smoke test and Linux visual-capture job.

## Roadmap

### P0 - Establish the integrated client

- [x] Pin and build the upstream Open-Golf game.
- [x] Use upstream UI, rendering, physics and level assets rather than the homemade Win32 prototype.
- [x] Link the network library and integrate its application lifecycle.
- [x] Add a basic diagnostics panel for network smoke testing.
- [x] Add Windows build/package and Linux networking CI.
- [ ] Build the Linux visual target and inspect screenshots from Mesa/Xvfb.
- [ ] Diagnose the Windows hosted-runner startup crash and test on physical Windows hardware.
- [ ] Add standard host/find/join/lobby controls to the actual front-end UI.
- [ ] Test on a physical Windows machine.

### P1 - Simultaneous multiplayer

- [ ] Convert the single golf_game_t ball into per-player ball state.
- [ ] Update physics and collision to simulate every player's ball against the same course.
- [ ] Restrict aiming/input to the local ball while displaying/interpolating remote balls.
- [ ] Add player names, ball colours, hats and trails to the upstream renderer.
- [ ] Make the host authoritative for shots, strokes, hole completion and match results.
- [ ] Add full scorecard and result UI for all players.

### P2 - Reliability and release

- [ ] Harden the protocol with validation, sequence IDs, rate limits, defined serialization and compatibility handling.
- [ ] Add reconnect and host-shutdown handling.
- [ ] Test 2, 4, 8 and 16 players on real networks.
- [ ] Version cosmetic profiles and award unlocks based on verified match wins.
- [ ] Measure the portable Windows ZIP and avoid unnecessary runtime dependencies.

### P3 - Optional additions

- [ ] Approved internet relay, only where the network owner permits the service and traffic.
- [ ] Experimental native Windows Bluetooth transport if it is worth maintaining.
- [ ] Additional original course themes and a course editor after the multiplayer core is stable.

See TODO.md for the short actionable list, AGENTS.md for agent workflow and docs/OPEN_GOLF_INTEGRATION.md for details of the upstream adaptation.
