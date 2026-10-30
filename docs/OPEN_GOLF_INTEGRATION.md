# Open-Golf integration

## Upstream source

Golfgame uses Open-Golf as a pinned Git submodule:

- Repository: https://github.com/mgerdes/Open-Golf
- Pinned commit: b9029f1e80c3110ae3d4fc9300854bdaeef96508
- Licence: MIT, retained at vendor/Open-Golf/LICENSE.
- Scope: upstream renderer, UI, Sokol application framework, course/level system, physics, assets, audio and editor sources.

The submodule is deliberate. It avoids copying a large third-party tree into Golfgame's own history and makes upstream provenance/update review straightforward.

## Build integration

The root CMake project creates tg_network and then adds vendor/Open-Golf. On Windows it runs scripts/patch-opengolf-d3d11.cmake before adding the subdirectory, changing only the Windows backend macro from SOKOL_GLCORE33 to SOKOL_D3D11; the Golfgame target links d3d11 and dxgi. The Linux OpenGL backend is left as upstream supplied it. The patch modifies the local submodule CMakeLists.txt during configure and is intentionally re-applied if the submodule is reset. It removes upstream's src/golf/main.c from the golf target and substitutes src/integration/opengolf_main.c. That application entry point is derived from upstream main.c, retains the MIT attribution, and calls golf_mp_init(), golf_mp_update() and golf_mp_shutdown() at the correct lifecycle points.

src/integration/golf_multiplayer.c/.h wraps the existing Golfgame UDP API. It is linked to the upstream golf target and adds a Multiplayer diagnostics tab to the built-in ImGui debug console. Press the grave/backtick key to open the debug console. The tab currently exposes basic host, LAN discovery, localhost join, start and leave actions.

The Open-Golf UI is data-driven. The build copies upstream data to the runtime build/data folder and changes only the displayed OPEN GOLF title to GOLFGAME in that generated copy. The submodule's original files are not modified by this title override.

## Multiplayer work still required

Open-Golf's game implementation stores one ball in golf_game_t. Full simultaneous multiplayer requires a careful refactor, not just broadcasting the existing one-ball struct:

1. Introduce per-player ball state with position, velocity, orientation, water/out-of-bounds/hole state, stroke count and cosmetic IDs.
2. Update input/aiming to control only the local player's ball.
3. Run collision and movement for each active ball against the course and moving obstacles.
4. Render every ball, with stable player labels and visual-only cosmetics.
5. Interpolate remote snapshots and avoid letting packet jitter affect local input.
6. Preserve simultaneous putting; completed balls remain at the cup while others finish.
7. Make the host authoritative for shot acceptance, strokes, hole completion and course results.

The network protocol currently relays client-reported state and is not secure for public internet use. Do not claim complete multiplayer or public-game security until validation and authority are implemented.

## Runtime visual checks

scripts/capture-windows.ps1 launches build/golfgame.exe from the build directory and captures the main menu, course selector, gameplay and network diagnostics console. It saves PNGs and process logs under logs/. The Windows Actions workflow uploads this directory as golfgame-visual-logs. Download the artifact from the workflow run and inspect it after changes to the renderer, UI, course loading, input or lifecycle.

The screenshot automation is a smoke test, not a substitute for manual testing on a real Windows desktop or testing two separate PCs on a permitted LAN.
