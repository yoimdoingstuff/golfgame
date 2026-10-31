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
- [x] Verify the Host Lobby click, room creation status and seven real visual checkpoints in CI.
- [x] Retry initial black frames rather than uploading blank screenshots.
- [x] Hide the Multiplayer panel while the course selector is open so its close control remains accessible.
- [x] Fix the escaped CMake variables in the Open-Golf draw hook patch; full CI now configures and builds on Windows and Linux.
- [x] Verify the full Actions pipeline: Windows x64 build/package, Linux Open-Golf build, Linux networking smoke test and seven Mesa/Xvfb visual checkpoints (run 41195877311).
- [ ] Run the packaged game on a physical Windows PC and test two Windows PCs on a permitted LAN; CI compilation does not prove Windows runtime behaviour.
- [x] Add normal Host / Find LAN / Join by IP / lobby controls to the game's actual front-end UI.
- [ ] Test two Windows PCs on a permitted LAN.

## Multiplayer gameplay

- [ ] Adapt Open-Golf's single golf_game_t ball state into one independent physics/simulation state per connected player.
- [ ] Render remote balls with their names and cosmetic IDs; interpolate snapshots smoothly.
- [ ] Keep simultaneous putting. Do not add turn queues or a current-player turn system.
- [ ] Make the host validate shots, strokes, hole completion and final match results.
- [ ] Build complete course progression and a shared scorecard for all players.
- [ ] Test 2, 4, 8 and 16 players, packet loss, disconnects and host shutdown.
- [ ] Define a transport-independent connection API so gameplay is not coupled to UDP sockets.
- [ ] Add LocalSend-inspired nearby discovery (mDNS/DNS-SD plus UDP fallback), a visible nearby-room list and explicit pairing.
- [ ] Define a small, versioned Golfgame connection invitation that carries room ID, candidate endpoints, protocol capabilities and short-lived join credentials; never include long-lived secrets.
- [ ] Support exporting/importing a .golfinvite file and QR/room-code handoff so LocalSend or another user-chosen sharing app can transfer connection details without players typing IPs.
- [ ] Validate invitation data, expire it, bind credentials to a room/session, prevent replay and require the host to approve joins.
- [ ] Probe candidate paths automatically and report whether direct LAN IPv4/IPv6 connectivity is actually reachable; exchanging an IP address alone does not defeat client isolation or firewall rules.
- [ ] Add an explicit, user-configured SOCKS5 proxy option and document its limitations for UDP/realtime traffic.
- [ ] Research Tor SOCKS/obfs4 compatibility in an isolated, authorised test environment; measure latency, packet loss and whether the required transport semantics are supported. Do not make Tor/obfs4 a default gameplay path.
- [ ] Evaluate standard ICE/STUN connectivity negotiation and an explicitly operated/approved relay for networks where direct peer connections are unavailable.
- [ ] Add connection diagnostics showing discovery method, candidate paths, selected transport, RTT, loss and clear failure reasons without logging credentials.
- [ ] Add optional peer/node graph experiments only after stable host/client gameplay; relay only authenticated Golfgame messages, with consent, hop limits, loop prevention and rate limits.
- [ ] Evaluate OS-supported Wi-Fi Direct / Nearby Share integrations per platform. Do not attempt to clone proprietary AirDrop/Quick Share protocols.
- [ ] Treat Bluetooth as optional discovery/pairing or a platform-supported network transport; do not assume BLE can carry real-time game traffic.
- [ ] Test multicast-disabled networks, client isolation, VLAN separation, blocked UDP, proxy-only access and permitted relay scenarios. Respect network-owner policies; do not disguise traffic or bypass access controls.

## Polish and release

- [ ] Replace Open-Golf title/branding where needed with original Golfgame branding and a coherent multiplayer flow.
- [ ] Add ball colours, hats and trails to the Open-Golf ball renderer without affecting physics.
- [ ] Add persistent cosmetic unlocks based on verified match wins.
- [ ] Review bundled third-party notices and measure the portable release ZIP size.
- [ ] Consider an internet relay only when the service and traffic are explicitly permitted. Bluetooth remains optional.

## Known limitations

The current CI pipeline is green (run 41195877311): the Windows x64 client builds and packages, the Linux client builds, the LAN smoke test passes, and seven visual checkpoints were captured under Mesa/Xvfb. The Windows runner only compiles/packages the game; physical Windows runtime and two-PC testing remain outstanding. The multiplayer gameplay is not complete: Open-Golf still owns one physics ball per client, and remote-ball presentation has not been validated in a real multi-client match. The host still relays client-reported state and is not an authoritative or secure public-internet protocol.
