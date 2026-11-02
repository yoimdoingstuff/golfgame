# Project plan

## Product direction

A compact, match-based multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. This is not the full Tower/Plaza social hub. Players join a match and are represented by golf balls. All players can putt simultaneously.

## Locked-in targets

- Portable Windows x64 executable, distributed as a ZIP with no installer.
- Native C/CMake and a small runtime footprint; no heavyweight engine runtime.
- Open-Golf remains a pinned technical foundation for rendering, course geometry, physics, level loading and audio, not the product's game content or course catalogue.
- Golfgame owns its course pack, match flow, branding, multiplayer integration and the product-facing UI.
- 2–16 players per match, including the host.
- Low-poly, bright, toy-like courses with strong obstacle silhouettes.
- Angled third-person camera and golf balls as player identities.
- Simultaneous minigolf with no turn queue.
- Cosmetic ball colours, hats and trails; rewards never affect physics.

## Current implementation

The runtime data staging step removes upstream Open-Golf's sample level directory. It then emits Golfgame's original Workshop levels from assets/golfgame/workshop-course-pack.cmake, copies Golfgame's game configuration and UI, and retains common engine assets plus their required notices.

The initial course selector presents one Workshop pack. Selecting it starts an 18-hole sequential progression. The level label and finish wording use hole terminology. The main menu includes Golfgame branding, a course-pack card and a short match tagline. The gameplay HUD shows the current hole, par, local strokes, aiming power and connected-player list.

The renderer and one-ball physics still come from the pinned Open-Golf foundation. The current multiplayer layer hosts/finds/joins LAN rooms, replicates match start and displays/interpolates remote ball snapshots. It does not yet simulate all balls in the same world.

## Remaining limitations

- Each client still owns only one physical golf_game_t ball. The remote balls are not independently simulated and do not collide with the local ball.
- Scoreboard values are currently per-hole client-reported strokes, not a host-verified cumulative 18-hole scorecard.
- The host relays client-reported player state and is not authoritative or suitable for public internet play.
- Hats, trails, persistent cosmetics, course-pack previews and a fully original pause/results UI are not finished.
- CI builds and Linux software-renderer screenshots do not replace physical Windows and multi-PC LAN tests.

## Roadmap

### P0 - Make the product look and play like Golfgame

- [x] Remove upstream sample maps from the packaged runtime.
- [x] Create an original 18-hole Workshop course pack.
- [x] Make the selector choose a course pack, not an individual numbered stock level.
- [x] Add hole terminology and a first-pass in-game HUD.
- [ ] Replace the remaining upstream menu/pause/finish art and layout.
- [ ] Inspect all 18 holes in visual CI and tune the geometry/collisions.

### P1 - Real simultaneous multiplayer

- [ ] Create independent ball state for each connected player.
- [ ] Run collision/physics for all active balls on the shared course.
- [ ] Let only the local player control their ball; display stable remote name labels.
- [ ] Keep holed-out balls at the cup while other players finish.
- [ ] Add host-verified strokes and a cumulative 18-hole scorecard/results screen.

### P2 - Cosmetics and release

- [ ] Add ball colours, low-poly hats and trails.
- [ ] Award cosmetics from verified match wins.
- [ ] Test 2, 4, 8 and 16 players on real networks.
- [ ] Measure the portable Windows ZIP and remove unused runtime assets safely.

### P3 - Optional networking

- [ ] LocalSend-style discovery/invitation handoff.
- [ ] Explicitly permitted relay, SOCKS5 proxy experiments and controlled Tor/obfs4 transport research.
- [ ] Optional platform-specific Wi-Fi Direct or Bluetooth PAN experiments.

See TODO.md for the current actionable list and AGENTS.md for the coding workflow.
