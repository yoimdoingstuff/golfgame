# golfgame

A compact, portable Windows multiplayer minigolf game inspired by the minigolf minigame in GMod Tower. **The golf balls are the players.** There are no golfer avatars, humanoid rigs, character movement systems, or Plaza/social-hub world.

Players compete on compact, colourful low-poly courses, customise their balls with colours, hats and trails, and unlock cosmetic flair through wins.

## Current milestone: multiplayer transport prototype

This repository currently contains an original C11 networking proof-of-concept, kept separate from the game renderer. It is a transport foundation, not the complete game.

### Technology direction

- Native Windows x64 release, portable ZIP, no installer.
- C11 and CMake, without a heavyweight engine runtime.
- Open-Golf is the candidate rendering and golf-physics foundation: https://github.com/mgerdes/Open-Golf
- Sokol-based graphics/audio from Open-Golf; custom ball collision/response code.
- Low-poly themed courses and an angled third-person camera following the active ball.
- 2-16 players per match.

Open-Golf is MIT-licensed and already has minigolf ball physics, but it is single-player and currently assumes one ball. The integration work must retain upstream and third-party notices and add one ball state per player.

## Build

Requires CMake 3.16+ and a C11 compiler. Windows networking uses Winsock from the operating system; no external networking dependency is downloaded.

### Windows (Visual Studio)

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

The demo is normally at `build\Release\tg_net_demo.exe`.

### Windows (MinGW)

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Linux smoke test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./scripts/test-local.sh
```

## Test a LAN room

On computer A:

```text
tg_net_demo host Alice "Friday Match"
```

On computer B, on the same permitted local network:

```text
tg_net_demo scan
tg_net_demo join 192.168.1.23 Bob
```

Discovery and direct UDP joining use port `27890`. Direct IP is useful if broadcast discovery is disabled but direct LAN traffic is allowed. The prototype supports up to 16 players including the host.

## Multiplayer design

- LAN auto-discovery first, direct IPv4/hostname join as a fallback.
- An internet relay may be added only where the network owner explicitly permits the service and traffic.
- Bluetooth is an optional future native Windows adapter, not part of this milestone.
- Managed networks may isolate clients or block peer traffic; no transport guarantees connectivity everywhere.
- The host must ultimately own turn order, shot validation, ball simulation, strokes and results.
- Cosmetic IDs are replicated, but unlock ownership is not authenticated yet.

The current network protocol is a local prototype, not a secure public-game protocol. Before public release it needs stronger peer validation, packet sequencing, rate limits, input validation, protocol negotiation and reconnect handling.

## Gameplay direction

- Each player is represented by one golf ball.
- Ball colour identifies players; optional hats/accessories sit on the ball and optional trails follow it.
- Cosmetics are visual-only and never change mass, friction, bounce, collision or shot power.
- Wins unlock extra colours, hats and trails.
- One player shoots at a time; everyone else watches and spectates.
- Compact themed courses use ramps, tunnels, bumpers, lifts, launchers, moving obstacles, water hazards and silly shortcuts.

See [the roadmap](docs/PROJECT_PLAN.md), [course direction](docs/COURSE_DIRECTION.md), [cosmetics and progression](docs/COSMETICS_AND_PROGRESSION.md), and [Open-Golf integration notes](docs/OPEN_GOLF_INTEGRATION.md).
