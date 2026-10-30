# Agent instructions

These instructions apply to the entire repository. Read this file before making changes.

## Project in one minute

Golfgame is a small, portable Windows multiplayer minigolf game. It is inspired by the minigolf minigame in GMod Tower, not the Tower Plaza or a persistent social hub.

**The golf balls are the players.** There are no golfer avatars, humanoid rigs, character controllers, walking animations or player bodies. A player is identified by their ball, name and cosmetic choices.

The intended game has compact, colourful low-poly courses, an angled third-person camera, and up to 16 players who can all aim and putt at the same time. Wins unlock visual cosmetics only.

## Non-negotiable design rules

1. **Ball-only player identity.** Never add humanoid player characters or a social-hub world.
2. **Simultaneous play.** Do not add turn queues or an active-player turn system. Each player can putt independently.
3. **Cosmetics are cosmetic.** Ball colour, hats and trails must not affect physics, collision, shot power or visibility in a way that gives a gameplay advantage.
4. **Small native build.** Keep the Windows build portable and lightweight. The current direction is C11, CMake, Win32 and the Windows system OpenGL 1.1/GLU libraries. Do not introduce Unity, Godot, Electron, a browser runtime, WebView or a large engine without an explicit product decision.
5. **Original content.** Create original course geometry, meshes, UI, textures and audio. Do not copy GMod Tower or Tower Unite proprietary assets, maps, logos, fonts or exact interface artwork. Check licences and retain notices for any third-party material.
6. **Network controls are respected.** Implement only ordinary LAN traffic and explicitly permitted services. Do not disguise traffic, tunnel around school/work filtering, or claim that LAN discovery will work on networks with client isolation.

## Current implementation

Treat code as the source of truth; some older planning notes describe ideas that were never implemented.

- src/desktop/win_main.c is the current native Win32/OpenGL graphical prototype. It contains the window, menus, HUD, course rendering, basic ball physics, input and local profile handling in one file.
- src/network/tg_network.h and src/network/tg_network.c are the C11 UDP networking layer. The public API is in the header. The current wire protocol version is defined in the C file.
- src/demo/net_demo.c is a small command-line host/scan/join test client.
- CMakeLists.txt defines the targets. The graphical golfgame target is built on Windows; the networking demo can also be built on Linux.
- scripts/test-local.sh runs a localhost UDP host/client smoke test.
- .github/workflows/build.yml builds the Windows client and package, then builds and smoke-tests the Linux networking demo.
- docs/ contains product, course, HUD, cosmetics and Open-Golf research notes.

The current graphics and physics are a **prototype**, not a finished game. It currently has one compact demo hole, basic ball movement and cosmetic rendering. A successful Windows CI build proves compilation and packaging, not that the graphical game has been launched or play-tested on a real Windows desktop.

Open-Golf is a research/reference project only. It is not vendored and is not a current build dependency. Do not start an integration or replace the current client with it unless the project owner explicitly changes direction.

## Architecture and important constraints

### Desktop client

- The current graphical client is intentionally kept in src/desktop/win_main.c. It uses Win32 APIs, OpenGL 1.1 immediate-mode drawing and GLU.
- The current code is largely organised around file-static state and helper functions. Follow existing naming and ownership patterns; avoid broad rewrites just to impose a new architecture.
- The game loop owns input, frame updates, networking updates and rendering. Preserve responsive non-blocking behaviour.
- The prototype uses simple local ball physics. Do not treat local simulation as authoritative multiplayer physics.
- The profile is currently stored in a small profile.ini using Windows profile APIs. Keep profile changes backward-compatible or add explicit versioning when the format grows.
- Avoid adding runtime DLLs, frameworks or large binary assets. Check the resulting release ZIP size when packaging changes.

### Networking

- tg_network.h defines the public API and shared player/room structures. Update it deliberately and keep callers in sync.
- The current transport is UDP on port 27890, with local IPv4 broadcast discovery and direct IPv4/hostname joining.
- TG_NET_MAX_PLAYERS is 16 including the host.
- The current packet structs are packed and carry a protocol version. Keep packet layouts, size checks and version handling consistent. Bump the protocol version when changing an incompatible wire format.
- Current packets are capped at 1200 bytes by the packet builder. Keep snapshots within this limit.
- Networking is driven by regular calls to tg_net_update(dt). The implementation currently uses one module-global context; do not assume the API is thread-safe or introduce background networking threads without designing synchronisation.
- The host currently relays client-reported ball state. It does **not** yet provide secure authoritative simulation, shot validation or score verification. Treat all client-supplied state as untrusted.
- The current protocol is a prototype, not suitable for untrusted public internet play. Before that, add robust input validation, sequence/timestamp handling, rate limits, host-side shot and score validation, protocol compatibility handling and reconnect/disconnect behaviour.
- Raw packed C structs are currently copied to/from UDP packets. Do not assume that representation is a permanent cross-architecture wire format; use explicit field encoding and defined byte order before claiming broad platform interoperability.
- UDP broadcast is not internet discovery or NAT traversal. An internet relay and a native Bluetooth adapter are not implemented.

### Gameplay

- Maintain one independent ball state for each player.
- Keep simultaneous putting; players finish a hole independently.
- A finished ball should remain visible at the cup while others finish.
- Keep player names and ball colours readable at normal camera distance. Colour should not be the only identifier.
- Course hazards and obstacles need clear collision rules and feedback.
- Moving obstacle visuals must not be mistaken for completed gameplay mechanics: verify that an obstacle actually affects ball physics before documenting it as functional.

## Build and test commands

### Windows, Visual Studio generator

Run from the repository root in PowerShell:

    cmake -S . -B build -A x64
    cmake --build build --config Release

The graphical client should be at build\Release\golfgame.exe; the networking demo should be at build\Release\tg_net_demo.exe.

### Windows, MinGW

    cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    cmake --build build

### Linux networking build and smoke test

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    sh ./scripts/test-local.sh

The Linux workflow does not build the Win32 graphical client. The local smoke test verifies a localhost UDP host/client exchange; it does not prove multi-machine LAN compatibility.

## Change workflow

1. Read this file, README.md, TODO.md and the relevant document under docs/.
2. Inspect the actual implementation before relying on a roadmap item or old comment. In particular, distinguish implemented, prototype-only, planned and untested behaviour.
3. Make the smallest cohesive change that preserves the design rules above. Avoid unrelated refactors.
4. Keep the C11 build warning-clean under the existing compiler warning flags. Do not silently remove warning flags to get a green build.
5. Run the relevant build and tests. For network changes, run the localhost smoke test. For Windows graphics changes, ensure the Windows CI job compiles and packages the client.
6. When possible, manually test on Windows and with at least two separate computers. Clearly record when only compilation or localhost tests were possible.
7. Update TODO.md when work is completed, new blockers are found or priorities change. Update the relevant design document when behaviour or architecture changes.
8. Review the diff for accidental binaries, generated build output, secrets, unrelated changes and third-party licence omissions.

## Documentation and reporting rules

- Never describe a planned feature as implemented.
- Never describe a successful compile as a successful runtime test.
- State exactly which tests ran and which could not be run.
- Keep commands copyable and paths accurate.
- Keep the main TODO short and actionable; move lengthy design detail into docs/.
- Preserve the repository's GPL-3.0 licence. Do not change the project licence or remove its notices.
