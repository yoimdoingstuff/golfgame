# TODO

Short, current priority list. Check items only when implemented and tested. Read AGENTS.md before changing the code.

## Immediate

- [x] Replace the homemade Win32/OpenGL UI and demo course with the pinned Open-Golf game, renderer, UI, physics and level data.
- [x] Link the Golfgame UDP networking library into the Open-Golf application lifecycle.
- [x] Add a Multiplayer diagnostics tab to the Open-Golf debug console.
- [x] Add Windows and Linux CI build jobs plus an always-uploaded Linux visual-log artifact.
- [x] Add labelled visual capture checkpoints using Mesa software OpenGL under Xvfb.
- [x] Build the Linux Open-Golf client and launch its Sokol window under Mesa/Xvfb.
- [x] Fix the upstream path-extension overflow and inspect the main-menu, course-select, gameplay and network-console screenshots.
- [x] Add a player-facing Multiplayer panel to the main menu with LAN host/discovery, direct address join, room list and lobby controls.
- [ ] Verify the new Host Lobby screenshot and adjust its automated click target if needed.
- [ ] Check whether the Open-Golf path-extension fix resolves the Windows hosted-runner access violation (0xC0000005), then test the actual Windows game on a physical Windows PC.
- [x] Add normal Host / Find LAN / Join by IP / lobby controls to the game's actual front-end UI.
- [ ] Test two Windows PCs on a permitted LAN.

## Multiplayer gameplay

- [ ] Adapt Open-Golf's single golf_game_t ball state into one independent physics/simulation state per connected player.
- [ ] Render remote balls with their names and cosmetic IDs; interpolate snapshots smoothly.
- [ ] Keep simultaneous putting. Do not add turn queues or a current-player turn system.
- [ ] Make the host validate shots, strokes, hole completion and final match results.
- [ ] Build complete course progression and a shared scorecard for all players.
- [ ] Test 2, 4, 8 and 16 players, packet loss, disconnects and host shutdown.

## Polish and release

- [ ] Replace Open-Golf title/branding where needed with original Golfgame branding and a coherent multiplayer flow.
- [ ] Add ball colours, hats and trails to the Open-Golf ball renderer without affecting physics.
- [ ] Add persistent cosmetic unlocks based on verified match wins.
- [ ] Review bundled third-party notices and measure the portable release ZIP size.
- [ ] Consider an internet relay only when the service and traffic are explicitly permitted. Bluetooth remains optional.

## Known limitations

The imported game still simulates only one visible ball per client. The lobby and course-start signal are networked, but remote clients are not yet rendered as separate balls or included in a shared collision/shot simulation. The Windows executable compiles, but Windows hosted-runner runtime capture has crashed before creating a window. Linux Mesa/Xvfb captures are being added as a reliable shared-renderer/UI check; real Windows and multi-PC playtesting remain outstanding.
