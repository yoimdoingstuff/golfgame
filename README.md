# Golfgame

An original, compact Windows multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. It is not a recreation of Garry's Mod, Tower Unite or Open-Golf. **The golf balls are the players:** no golfer avatars, humanoid rigs, character movement or social-hub world.

## What is in the game now

- An original **Workshop** course pack containing 18 sequential holes. The holes use procedurally authored fairways, raised platform geometry, retaining rails, ramps, bumpers and moving spinner obstacles.
- One course-pack selection called **Workshop**, followed by the 18-hole progression. The original numbered Open-Golf sample-level selector and sample level files are not part of the packaged game.
- A Golfgame-styled title/menu layout, course-pack card, hole-complete wording and in-game HUD for hole number, par, strokes and the current player list.
- Third-person ball gameplay and course physics built on the pinned Open-Golf renderer/physics/level-loading foundation.
- A C11 UDP LAN lobby for up to 16 players, with host/find/join controls and a multiplayer diagnostics tab.

**Important multiplayer limitation:** the underlying gameplay simulation still has one physical ball per client. Remote balls are currently presentation snapshots, not independently simulated balls sharing the same collision world. The HUD is not yet a persistent 18-hole scorecard, and hats/trails/cosmetic unlocks are not implemented yet.

## Engine and licensing

Open-Golf is retained as a pinned MIT-licensed technical foundation for Sokol rendering, course geometry, physics, asset loading and audio. Its original licence and third-party notices remain in the repository. Golfgame owns the course pack, course progression configuration, product branding, multiplayer integration and its menu/HUD changes. The build stages common engine runtime assets but removes the upstream sample data/levels before packaging and generates only Golfgame's Workshop levels.

## Clone

Clone with submodules so the engine source and original notices are present:

    git clone --recurse-submodules https://github.com/yoimdoingstuff/golfgame.git
    cd golfgame

For an existing checkout:

    git submodule update --init --recursive

## Build

Requires CMake 3.16+ and a C11/C++11 compiler.

### Windows (Visual Studio)

    cmake -S . -B build -A x64
    cmake --build build --config Release --target golf tg_net_demo

The game is build/golfgame.exe. The network test utility is build/Release/tg_net_demo.exe. Runtime assets are staged under build/data.

### Linux

Install the X11/OpenGL/ALSA development libraries used by CI, then:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target golf tg_net_demo
    sh ./scripts/test-local.sh

## Visual CI

The Windows CI job compiles and packages the Windows executable. Linux CI builds the same renderer/game source under Mesa software OpenGL and Xvfb, then captures labelled main-menu, lobby, course-selection, gameplay and diagnostics checkpoints. Download the golfgame-visual-logs artifact from the latest GitHub Actions run:

https://github.com/yoimdoingstuff/golfgame/actions

CI compilation and Mesa captures are not a substitute for running the executable on physical Windows hardware or testing two Windows PCs on a permitted LAN.

## Match design

- 2–16 players, each represented by one coloured golf ball.
- Everyone may aim and putt simultaneously. There are no turns.
- A player who holes out should remain at the cup while the others finish.
- Hats and trails are visual-only cosmetics planned for later work.
- Course progression and the future scorecard should span all 18 holes.

## Project documents

- TODO.md: actionable implementation priorities.
- AGENTS.md: source/layout rules and development workflow.
- docs/COURSE_DIRECTION.md: course and match design.
- docs/ART_STYLE_AND_HUD.md: visual direction and HUD hierarchy.
- docs/COSMETICS_AND_PROGRESSION.md: ball cosmetics and win rewards.
- docs/NEARBY_NETWORK.md: nearby discovery, invitations and transport research.
- docs/OPEN_GOLF_INTEGRATION.md: what remains upstream and what Golfgame overrides.
- docs/THIRD_PARTY_NOTICES.md: attribution and licence guide.
