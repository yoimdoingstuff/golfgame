# Nearby and node-based networking

## Goal

Make joining a game feel as effortless as LocalSend, AirDrop or Quick Share: open Golfgame, see nearby players/devices, select one, approve the connection and join. These products are inspiration for the experience, not protocols to impersonate or reverse-engineer.

The networking layer should be transport-independent. The game should ask for a session connection; the transport layer chooses an available, permitted path and reports its status.

## Proposed connection model

### 1. Nearby discovery

- Advertise a small Golfgame service record on the local network using mDNS/DNS-SD where available.
- Retain the current UDP discovery broadcast as a fallback for networks where multicast service discovery is unavailable.
- Include only the minimum discovery metadata: protocol version, short room/device label, room ID, current/max player count and a temporary connection endpoint.
- Do not broadcast player secrets, persistent identifiers, profile data or authentication tokens.
- Show discovered rooms as nearby devices with a clear host/room name, player count and connection state.
- Keep direct IP/hostname and short room-code joining as fallback paths.

### 2. Pairing and trust

- A discovered device is not trusted merely because it is on the same LAN.
- Require an explicit join/accept action. For first-time pairing, show a short comparison code or QR code on both devices.
- Establish an authenticated encrypted session using a reviewed standard cryptographic library and ephemeral key agreement.
- Add replay protection, protocol-version negotiation, packet-size limits and rate limits.
- Store a remembered peer identity only after user consent, with a way to remove it.
- Never roll a custom encryption protocol.

### 3. Node graph

Represent each running client as a node. A connection graph may contain:

- **Direct edge:** preferred connection between two peers on the same reachable network.
- **Coordinator node:** the room host manages membership, match start and authoritative match state.
- **Optional relay edge:** a peer may forward Golfgame packets for another peer only when that peer explicitly opts in and the network/service policy permits it.

The first implementation should remain simple: host-coordinated sessions with direct client-to-host links. A full mesh is not automatically better; with 16 players it increases connection count, NAT/firewall complexity and attack surface. Add direct peer edges only if a measured gameplay feature needs them.

Multi-hop forwarding is a later experiment, not a way to sneak traffic through blocked networks. Any relay must be visible, consent-based and limited to Golfgame traffic. Add a hop limit, loop prevention, bandwidth caps, peer revocation and end-to-end authentication before enabling it.

### 4. Transport priority

1. Direct LAN IPv4/IPv6 connection after nearby discovery.
2. Documented OS-supported Wi-Fi Direct / local peer APIs, if they are available and permitted on the target Windows versions.
3. Explicitly permitted internet relay service for players on different networks.
4. Bluetooth only as a small discovery/pairing channel by default. BLE is not a suitable assumed transport for frequent real-time ball snapshots. Investigate Bluetooth PAN only as a separate, platform-specific option.

Do not claim compatibility with AirDrop or Quick Share. Their protocols and system integrations are not generic public game networking APIs. Recreate the simple nearby-device experience with open standards and documented platform APIs instead.

## Implementation stages

- [ ] Define a transport interface so gameplay does not depend directly on UDP sockets.
- [ ] Add mDNS/DNS-SD advertisements and discovery, retaining current UDP broadcast fallback.
- [ ] Create a nearby-device list with room name, host, player count, protocol compatibility and signal/connection status.
- [ ] Add first-connection confirmation and short-code/QR pairing.
- [ ] Add authenticated encrypted sessions using a maintained library; document key lifecycle and peer revocation.
- [ ] Add IPv6 and robust interface selection for multi-adapter Windows PCs.
- [ ] Add connection diagnostics showing the selected path (direct LAN, Wi-Fi Direct, or approved relay).
- [ ] Evaluate optional peer-to-peer edges and multi-hop relay only after direct host/client multiplayer is stable and measured.
- [ ] Test client isolation, guest Wi-Fi, multiple network adapters, firewall-denied cases, packet loss and reconnects without attempting to bypass restrictions.

## Security and policy rules

- Discovery is not authentication.
- Never trust a peer's advertised player count, room name or state without validating the session.
- Do not forward arbitrary IP traffic. Relay only bounded Golfgame protocol messages.
- No stealth tunnels, traffic disguise, firewall evasion or automatic proxying.
- Fail clearly when a network blocks peer-to-peer communication. Offer manual join or an explicitly permitted relay instead.
- Do not log secrets, pairing codes, private keys or full packet payloads in CI artifacts.
