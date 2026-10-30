# Open-Golf integration

## Upstream source

Golfgame uses Open-Golf as a pinned Git submodule:

- Repository: https://github.com/mgerdes/Open-Golf
- Pinned commit: b9029f1e80c3110ae3d4fc9300854bdaeef96508
- Licence: MIT, retained at vendor/Open-Golf/LICENSE.
- Scope: upstream renderer, UI, Sokol application framework, course/level system, physics, assets, audio and editor sources.

The submodule is deliberate. It avoids copying a large third-party tree into Golfgame's own history and makes upstream provenance/update review straightforward.

## Build integration

The root CMake project creates tg_network and then runs `scripts/patch-opengolf-file.cmake` before adding vendor/Open-Golf. This idempotent patch replaces two unsafe full-buffer `strncpy` calls in upstream `golf_file_append_extension()` with correctly sized `snprintf` calls. The original code copied `GOLF_FILE_MAX_PATH` bytes beginning at `file.path + path_len`, corrupting stack memory during the initial asset scan. The patch fails closed if the pinned source no longer matches. It modifies only the local submodule worktree during configure, not the pinned upstream revision in Git. It removes upstream's src/golf/main.c from the golf target and substitutes src/integration/opengolf_main.c. That application entry point is derived from upstream main.c, retains the MIT attribution, and calls golf_mp_init(), golf_mp_update() and golf_mp_shutdown() at the correct lifecycle points.

src/integration/golf_multiplayer.c/.h wraps the existing Golfgame UDP API and adds a player-facing ImGui Multiplayer panel to the main menu. It provides player/room name fields, LAN hosting/discovery, direct IPv4/hostname join, discovered room joins, a room roster, host-only Start Match and Leave Room. It also retains a diagnostics tab in the built-in debug console (grave/backtick key). A replicated start flag calls golf_start_level(0) on connected clients. This starts the same course, but remote balls are still not rendered or simulated in a shared physics world.

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

scripts/capture-linux.sh launches the Linux game under Xvfb with Mesa software OpenGL. It captures PNG checkpoints for startup/main menu, the hosted LAN lobby, course selection, gameplay and the multiplayer diagnostics console, then writes screenshots and runtime stdout/stderr under logs/. The build-linux-visual GitHub Actions job uploads the logs directory as golfgame-visual-logs, even when capture fails.

These screenshots exercise the shared Open-Golf renderer/UI and course loading. They do not test the Windows OpenGL driver or replace manual Windows runtime and two-PC LAN tests.
