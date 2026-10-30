# Project plan

## Product direction

A compact, match-based multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. This is **not** the full Tower/Plaza social hub. Players join a match and are represented by golf balls. There are no golfer characters, humanoid avatars, walking animations or lobby world.

## Locked-in targets

- Portable Windows x64 executable, distributed as a ZIP with no installer.
- Native C/CMake and a small runtime footprint; no Godot runtime.
- Open-Golf is the candidate rendering/physics foundation (C, CMake, Sokol and custom golf-ball collision/response; MIT license).
- Low-poly, colourful courses with exaggerated, readable obstacles.
- Angled third-person camera following the active ball.
- 2-16 players per match.
- Turn-based minigolf, with every player's ball visible.
- Player identity is expressed through ball colour, optional hat/accessory and trail.
- Win progression unlocks cosmetics only; no physics upgrades.

## First playable slice

1. Compact menu with Host, Find Match, Join by IP and Settings. No Plaza.
2. Rooms support up to 16 players.
3. One compact low-poly course with a complete start-to-finish match.
4. One golf ball per player, labelled by player name and coloured for easy identification.
5. Aim, power control, shot preview and camera orbit/follow.
6. Only the active player may shoot; host validates turns and scoring.
7. Other players can watch/spectate; scorecard and results screen show the winner.
8. Obstacles include ramps, banked turns, moving platforms, bumpers and a water hazard.

## Networking

### Transport order

- **LAN discovery:** UDP broadcast within the local IPv4 segment. Implemented in the prototype.
- **Direct join:** IPv4 or hostname, UDP port 27890. Implemented in the prototype.
- **Internet relay:** later, only using a service and traffic explicitly permitted by the network owner.
- **Bluetooth:** optional future Windows-native adapter; pairing and device support make it a separate integration.

Managed networks can use client isolation, VLANs, endpoint rules and firewalls. Those can block LAN traffic regardless of the game. The project will report connection failures rather than disguise traffic or bypass network controls.

### Match authority

The host will own turn order, accepted shots, ball simulation, strokes, hole completion and results. Clients send shot inputs only on their turn. The host simulates the current ball and broadcasts authoritative ball snapshots. Cosmetic IDs can be replicated, but unlock ownership is not authenticated in this prototype.

Before public release, add packet sequence numbers, rate limits, stronger peer validation, protocol negotiation, host-side shot/score validation and reconnect handling.

## Roadmap

### M0 - transport prototype

- [x] Standalone C11 networking library.
- [x] UDP LAN discovery and room advertisement.
- [x] Direct IPv4/hostname join.
- [x] 16-player room cap and periodic snapshots.
- [x] Basic leave and timeout handling.
- [x] Ball-only player state: ball position, colour, hat ID and trail ID.
- [ ] Windows-to-Windows and school-network tests.

### M1 - Open-Golf integration

- Integrate the network library with Open-Golf's Windows target.
- Preserve upstream MIT and third-party notices.
- Adapt the single-ball game state to one ball record per match player.
- Add Host/Find Match/Join by IP controls to the existing menu.
- Render remote golf balls and their cosmetics.
- Implement authoritative turn order and score validation.

### M2 - first playable match

- One polished low-poly course.
- 2-16 coloured golf balls, labels and turn indicators.
- Ball aiming, shooting, camera orbit and follow.
- Spectating, scorecard and match results.
- No golfer/avatar models.

### M3 - cosmetics and progression

- Unlock colours, hats and trails with wins/milestones.
- Save a versioned local profile.
- Validate unlocks through trusted profile/match state, not a client's claim.
- Keep cosmetic assets small and data-driven.

### M4 - reliability and packaging

- Test 2, 4, 8 and 16 players, packet loss, disconnects and host exits.
- Add sequence IDs, rate limiting and protocol compatibility checks.
- Strip editor/debug-only code from Release.
- Measure the portable ZIP size on actual builds.

### M5 - optional additions

- Approved internet relay, then experimental Bluetooth if worth maintaining.
- More short courses, mutators and eventually a simple course editor.

Upstream candidate: https://github.com/mgerdes/Open-Golf
