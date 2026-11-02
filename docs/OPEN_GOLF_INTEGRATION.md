# Open-Golf integration

## Upstream source and scope

Golfgame uses Open-Golf as a pinned Git submodule:

- Repository: https://github.com/mgerdes/Open-Golf
- Pinned commit: b9029f1e80c3110ae3d4fc9300854bdaeef96508
- Licence: MIT, retained at vendor/Open-Golf/LICENSE.
- Retained technical scope: Sokol renderer, course geometry and draw passes, one-ball physics, level/data loader, common textures/models/shaders/audio and third-party dependencies.

Open-Golf is an engine foundation only. Its sample courses, product name and numbered course catalogue are not shipped as Golfgame content. Do not restore the upstream levels or present the project as an Open-Golf reskin.

## Golfgame-owned content and flow

- assets/golfgame/workshop-course-pack.cmake stores the generated source data for 18 original Workshop holes. Each GEO entity's lightmap-section UV array is encoded using the upstream packed base64/int32 format (100,000 fixed-point scale), not a JSON number array.
- assets/golfgame/config/game.cfg points the 18-hole progression at the generated workshop-01.level through workshop-18.level.
- assets/golfgame/ui/ui.ui supplies the Golfgame title, Workshop subtitle, hole terminology and adjusted palette. The course-flow patch keeps the title/tagline but removes the stock Play button/selector interaction so the Golfgame ImGui CTA starts the Workshop directly.
- scripts/copy-opengolf-data.cmake copies shared engine runtime assets, removes the upstream data/levels directory, writes only the 18 Golfgame levels, and stages the Golfgame UI/config. scripts/validate-workshop.py checks the staged JSON, triangle indexes, materials, lightmap images and packed UV byte counts before launch.
- scripts/patch-opengolf-course-flow.cmake changes the upstream selector to a single Workshop pack card and changes dynamic level wording to hole wording. It also points the boot scene at the first Workshop hole.
- scripts/patch-opengolf-main-menu.cmake adds the Golfgame course/match tagline to the JSON-driven main menu.
- src/integration/golf_multiplayer.c adds the LAN flow, debug diagnostics, Golfgame-owned Workshop entry CTA, a teal theme and the first-pass in-game HUD/scoreboard.

The patches are narrow, idempotent and fail if the expected upstream anchors are absent. They modify only the local submodule worktree during CMake configure; they do not change the pinned upstream commit.

## Build integration and safety patch

The root CMake project creates tg_network and applies scripts/patch-opengolf-file.cmake before adding the submodule. This replaces two unsafe full-buffer strncpy calls in upstream golf_file_append_extension() with correctly sized snprintf calls. The original code could overrun a stack buffer during asset indexing. The patch fails closed if the pinned source no longer matches.

The upstream src/golf/main.c is removed from the golf target and replaced by src/integration/opengolf_main.c, adapted from upstream with its MIT attribution retained. The project-owned application entry point initialises, updates and shuts down the Golfgame network layer.

## Multiplayer limitations

Upstream gameplay stores one ball in golf_game_t. Full simultaneous multiplayer requires a careful refactor:

1. Introduce per-player ball state with position, velocity, orientation, water/out-of-bounds/hole state, stroke count and cosmetic IDs.
2. Update input/aiming to control only the local player's ball.
3. Run collision and movement for each active ball against the same course and moving obstacles.
4. Render every ball with stable name labels and visual-only cosmetics.
5. Interpolate remote snapshots without letting packet jitter affect local input.
6. Preserve completed balls at the cup while others finish.
7. Make the host authoritative for shots, strokes, hole completion and 18-hole results.

The host currently relays client-reported state and is not authoritative or secure for public internet play. Do not claim full multiplayer until per-player simulation and validation are implemented and tested.

## Runtime visual checks

scripts/capture-linux.sh launches the Linux game under Xvfb with Mesa software OpenGL. It captures startup/main menu, hosted LAN lobby, course-pack selector, gameplay and diagnostics checkpoints, then stores screenshots and runtime stdout/stderr under logs/. The GitHub Actions visual job uploads the logs even when capture fails.

The screenshots should now visibly show Golfgame branding, one Workshop course-pack card and original Workshop hole geometry. They validate the shared renderer/UI and course loader; they do not replace physical Windows runtime and multi-PC LAN tests.
