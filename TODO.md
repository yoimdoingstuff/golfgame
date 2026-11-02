# TODO

Short, current priority list. Check items only when implemented and tested. Read AGENTS.md before changing the code.

## Product conversion and course content

- [x] Replace the shipped Open-Golf sample levels with an original Golfgame-authored 18-hole Workshop course pack.
- [x] Encode each course lightmap UV buffer in the upstream packed base64/int32 format and validate all 18 emitted level files before runtime capture.
- [x] Replace the 20-number sample level selector with one Workshop course-pack card and sequential hole progression.
- [x] Stage only Golfgame level files into the packaged runtime data; retain upstream engine models, textures, shaders, fonts and licence notices only where used.
- [x] Change the game-facing title, course-pack labels, hole-complete wording and level number to Golfgame / Workshop / HOLE.
- [x] Remove the stock Play button / numbered selector interaction and add a Golfgame Workshop entry CTA with a cohesive teal multiplayer panel theme.
- [x] Add an in-game HUD for hole, par, strokes, power and the current player list.
- [ ] Replace the remaining upstream UI layout/pixel-pack look in pause, finish and scorecard screens; the main-menu Play/selector flow now uses Golfgame-owned controls.
- [ ] Add more original course packs and a simple authoring/validation tool for course geometry.
- [ ] Add hole previews, themed props, ramps, tunnels, lifts, launchers and shortcuts with more distinctive visual identities.
- [ ] Inspect every Workshop hole screenshot and fix geometry, collision, camera or difficulty problems discovered in runtime.

## Immediate engineering

- [ ] Confirm Windows and Linux full builds, then inspect the new Workshop course selector and gameplay screenshots from CI. The first capture exposed a lightmap-UV serialization mismatch; a startup backtrace identified and the new pack now uses the correct upstream encoding.
- [ ] Verify no Open-Golf sample level files are present in the Windows package.
- [ ] Run the packaged game on a physical Windows PC and test two Windows PCs on a permitted LAN.
- [ ] Remove remaining Open-Golf branding, stock menu strings and unused product-facing references without removing upstream attribution/licence notices.
- [ ] Review the large portable ZIP and trim unused upstream assets safely.

## Multiplayer gameplay

- [ ] Refactor the single Open-Golf golf_game_t ball into independent per-player simulation states.
- [ ] Simulate every active ball against the same course and moving obstacles; remote balls must collide and interact, not just render snapshots.
- [ ] Keep simultaneous putting. Do not add turn queues or a current-player turn system.
- [ ] Add stable ball labels, colours, hats and trails; keep cosmetics visual-only.
- [ ] Make the host validate shots, strokes, hole completion and final match results.
- [ ] Preserve completed balls at the cup while other players finish.
- [ ] Build a persistent 18-hole scorecard, total strokes, par comparisons, winner/results screen and rematch flow.
- [ ] Test 2, 4, 8 and 16 players, packet loss, disconnects and host shutdown.

## Nearby networking

- [ ] Define a transport-independent connection API so gameplay is not coupled to UDP sockets.
- [ ] Add LocalSend-inspired nearby discovery (mDNS/DNS-SD plus UDP fallback), a visible nearby-room list and explicit pairing.
- [ ] Define a small, versioned Golfgame connection invitation that carries room ID, candidate endpoints, protocol capabilities and short-lived join credentials; never include long-lived secrets.
- [ ] Support exporting/importing a .golfinvite file and QR/room-code handoff so LocalSend or another user-chosen sharing app can transfer connection details without players typing IPs.
- [ ] Validate invitation data, expire it, bind credentials to a room/session, prevent replay and require the host to approve joins.
- [ ] Probe candidate paths automatically and report whether direct LAN IPv4/IPv6 connectivity is actually reachable.
- [ ] Add an explicit, user-configured SOCKS5 proxy option and document its limitations for UDP/realtime traffic.
- [ ] Research Tor SOCKS/obfs4 compatibility in an isolated, authorised test environment; measure latency and packet loss before considering any game transport.
- [ ] Evaluate standard ICE/STUN connectivity negotiation and an explicitly operated/approved relay where permitted.
- [ ] Add connection diagnostics showing discovery method, candidate paths, selected transport, RTT, loss and clear failure reasons without logging credentials.
- [ ] Test multicast-disabled networks, client isolation, VLAN separation, blocked UDP, proxy-only access and permitted relay scenarios. Respect network-owner policies.

## Known limitations

The Workshop levels and course-pack progression are now authored by Golfgame, but they are loaded through the upstream Open-Golf level/physics engine. The game still simulates one physical ball per client; remote balls are snapshot renderings rather than independent balls in a shared collision world. Player strokes are currently per-hole network values, not a verified cumulative 18-hole scorecard. The host still relays client-reported state and is not authoritative or safe for public-internet play. CI compilation does not prove physical Windows runtime or multi-PC LAN play.
