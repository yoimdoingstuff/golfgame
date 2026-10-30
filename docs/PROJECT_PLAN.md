# Project plan

## Product direction

A compact, match-based multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. This is **not** the full Tower/Plaza social hub. Players join a match and are represented by their golf balls.

## Locked-in targets

- Portable Windows x64 executable, distributed as a ZIP with no installer.
- Native C/CMake and a small runtime footprint; no heavyweight engine runtime.
- Current graphical client: native Win32 + system OpenGL 1.1/GLU.
- Low-poly, colourful courses with exaggerated, readable obstacles.
- Angled third-person camera following the active ball.
- 2-16 players per match, including the host.
- Simultaneous minigolf: every player may aim and putt at the same time.
- Player identity is expressed through ball colour, optional hat/accessory and trail.
- Win progression unlocks cosmetics only; no physics upgrades.
- Original art and sound. Do not reuse proprietary GMod Tower or Tower Unite content.

## Current implementation status

The repository has a working native graphical **prototype** and a separate C11 UDP networking **prototype**.

### Implemented in the repository

- C11 networking library and command-line demo.
- UDP LAN discovery, direct IPv4/hostname join, room advertisement and a 16-player cap.
- Basic leave/timeout handling, periodic player snapshots and match-start message/state.
- Ball-only player network records with position, rotation, colour, hat/trail IDs, strokes and flags.
- Native Win32 graphical target using system OpenGL 1.1 and GLU.
- Basic menu/lobby flow, one compact low-poly demo course, ball aiming/putting and simple local ball physics.
- Initial ball colour, hat and trail customisation with local profile.ini persistence.
- Windows build/package workflow and Linux networking smoke-test workflow.

### Prototype limitations that must remain visible in planning

- The graphical game has not yet been confirmed through a real Windows runtime play-test in this project workflow. CI compilation/package success is not runtime verification.
- The course is one demo hole, not an 18-hole course system.
- Physics and obstacle interactions are basic. Verify each obstacle's collision behaviour before calling it complete.
- The host currently relays client-reported ball states; it is not authoritative or secure against modified clients.
- Remote-ball interpolation, full scorecard/match flow, robust camera/spectating, reconnect handling and multi-machine 2-16 player testing remain incomplete.
- The Linux job builds/tests the networking demo, not the Windows graphical game.
- Internet relay and native Bluetooth transport are not implemented.

## Architecture

- src/desktop/win_main.c: native Win32 window, UI, OpenGL rendering, input, prototype course/physics and local profile.
- src/network/tg_network.h: public networking API and shared room/player structures.
- src/network/tg_network.c: UDP transport, packet definitions, discovery, join/leave, snapshots and match-start state.
- src/demo/net_demo.c: command-line networking test client.
- CMakeLists.txt: CMake targets and platform-specific libraries.
- .github/workflows/build.yml: Windows build/package and Linux build/smoke test.
- scripts/: local build/test helpers.
- docs/: detailed product, course, HUD, cosmetic and research notes.

Open-Golf remains an optional research reference, not a dependency or the active implementation plan. No Open-Golf source or assets are vendored in this repository.

## Roadmap

### P0 - Verify the existing prototype

- [x] Add the native Win32/OpenGL graphical prototype.
- [x] Add the initial menu, lobby, one-hole course and basic ball controls.
- [x] Add the C11 UDP room/network prototype and 16-player data model.
- [x] Add Windows build/package CI and Linux localhost network smoke test.
- [ ] Launch and manually test the graphical client on Windows.
- [ ] Test actual Windows-to-Windows LAN discovery and direct joining.

### P1 - Make one complete multiplayer match

- [ ] Finish shot input, ball physics and obstacle collision behaviour.
- [ ] Add smooth interpolation for remote balls and reliable match state.
- [ ] Make the host validate shots, strokes, hole completion and results.
- [ ] Complete player scoreboard and results UI for all players.
- [ ] Handle disconnects, host exit and recoverable connection errors.

### P2 - Build the full course experience

- [ ] Create data-driven hole definitions and course loading.
- [ ] Add 18-hole progression, par values and cumulative scorecard.
- [ ] Award match wins once after the full course and use verified wins for cosmetic unlocks.
- [ ] Add multiple original themed courses and more obstacle types.
- [ ] Add spectating and polished camera follow/orbit controls.

### P3 - Reliability and release

- [ ] Test 2, 4, 8 and 16 players on real networks.
- [ ] Harden the protocol with sequence IDs, timestamps, rate limits, validation, defined serialization and compatibility handling.
- [ ] Add robust reconnect and host-shutdown handling.
- [ ] Version the local profile format and keep cosmetics visual-only.
- [ ] Measure release ZIP size and verify a clean portable launch on supported Windows versions.

### P4 - Optional additions

- [ ] Approved internet relay, only where the network owner permits the service and traffic.
- [ ] Experimental native Windows Bluetooth transport if it is worth the complexity.
- [ ] Course editor, extra mutators and additional polish after the core match is reliable.

For the short actionable list, see TODO.md. Agent/contributor workflow rules are in AGENTS.md.
