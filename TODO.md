# TODO

Short, current priority list. Check items only when implemented and tested. Read AGENTS.md before changing the code.

## Immediate

- [x] Replace the homemade Win32/OpenGL UI and demo course with the pinned Open-Golf game, renderer, UI, physics and level data.
- [x] Link the Golfgame UDP networking library into the Open-Golf application lifecycle.
- [x] Add a Multiplayer diagnostics tab to the Open-Golf debug console.
- [x] Add Windows and Linux CI build jobs plus an always-uploaded Linux visual-log artifact.
- [x] Add labelled visual capture checkpoints using Mesa software OpenGL under Xvfb.
- [ ] Successfully build the Linux visual client and inspect main-menu, course-select, gameplay and network-console screenshots.
- [ ] Investigate the Windows hosted-runner startup access violation (0xC0000005) and test the actual Windows game on a physical Windows PC.
- [ ] Add normal Host / Find LAN / Join by IP / lobby controls to the game's actual front-end UI. The current controls are diagnostics-only.
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

The imported game is still single-player gameplay. The UDP module is initialised and updated, and its diagnostic controls can host/discover/join/start/leave rooms, but remote clients are not yet wired into Open-Golf's single-ball collision and rendering systems. The Windows executable compiles, but Windows hosted-runner runtime capture has crashed before creating a window. Linux Mesa/Xvfb captures are being added as a reliable shared-renderer/UI check; real Windows and multi-PC playtesting remain outstanding.
