# Open-Golf integration notes

The network module is engine-independent and can be added to an Open-Golf working tree without bringing in a new engine. This repository does not redistribute Open-Golf source or assets. Retain the upstream MIT copyright/license notice and all third-party notices when integrating them.

## CMake integration

Copy `src/network/tg_network.h` and `src/network/tg_network.c` into the upstream source tree, for example under `src/multiplayer/`.

```cmake
add_library(tg_network STATIC src/multiplayer/tg_network.c)
target_include_directories(tg_network PUBLIC src/multiplayer)
if(WIN32)
    target_link_libraries(tg_network PUBLIC ws2_32)
endif()
```

Link `tg_network` to the `golf` target in `src/golf/CMakeLists.txt`.

## Integration points

- `src/golf/main.c`: initialise networking, call `tg_net_update(dt)` once per frame and shut it down during cleanup.
- `src/golf/ui.c`: add Host/Find Match/Join by IP to the existing menu. Do not create a 3D lobby.
- `src/golf/game.h`: add up to 16 match player records, each with its own ball state, strokes, finish flag and cosmetic IDs. The old global ball cannot represent every player.
- `src/golf/game.c`: accept shots only from the active player. The host runs the existing ball physics and broadcasts authoritative results.
- Rendering: draw one ball per player with its colour, optional accessory and optional trail. Do not build golfer avatars.

## Protocol limits

The prototype snapshots player ID/name, ball position, strokes, flags, colour, hat ID and trail ID. Cosmetic IDs are display metadata, not verified unlocks.

LAN broadcast is limited to the local IPv4 broadcast segment. Direct join uses UDP port 27890. This is not NAT traversal or an internet relay. The protocol is not a secure public-game protocol yet; add sequence IDs, validation, rate limits, authoritative score logic and protocol negotiation before release. Bluetooth and internet relay are not implemented.
