# Nearby discovery, invitations and transport experiments

## Goal

Joining should feel like LocalSend: open Golfgame, see nearby rooms/devices, select one, approve the connection and join. LocalSend is inspiration for discovery and user-driven handoff, not a dependency or a protocol Golfgame should impersonate.

The connection layer must be transport-independent. Gameplay asks for a session; a connection manager discovers candidate paths, tests them and reports the selected transport and its limitations.

## Discovery and invitation handoff

1. Advertise minimal room metadata using mDNS/DNS-SD where available, retaining UDP discovery as a fallback.
2. Show nearby rooms with host/room name, player count, protocol compatibility and connection status.
3. If discovery is unavailable, let the host create a short-lived invitation. A .golfinvite file or QR code can be shared through LocalSend, email, messaging or another app chosen by the user.
4. Invitation contents should be versioned and minimal: room ID, candidate endpoints, supported transport capabilities, expiry and a short-lived, room-scoped join credential. Never put passwords, private keys, persistent authentication tokens or unrelated device data in the invitation.
5. Validate every field and endpoint, enforce size limits and expiry, prevent replay, and require host approval. Treat imported invitations as untrusted input.
6. Importing an invitation should populate the join UI automatically. The player should not need to manually copy IP addresses or ports.

An invitation transfers connection information; it does not make an unreachable endpoint reachable. Client isolation, VLAN separation and firewall rules can still prevent a direct connection.

## Connection negotiation

- Probe advertised IPv4 and IPv6 candidates and select a reachable direct path when available.
- Keep manual address and short room-code joining as fallbacks.
- Evaluate standard ICE/STUN for NAT traversal where supported and permitted.
- Provide a transparent, explicitly operated/approved relay fallback for peers that cannot connect directly.
- Expose diagnostics: discovery source, candidate paths, selected transport, connection time, RTT, loss and actionable failure reason.
- Do not silently proxy, tunnel or forward arbitrary network traffic. Any relay carries only authenticated, bounded Golfgame messages and is visible to participants.

## Proxy and obfuscation research

The transport API may support optional, user-configured proxy adapters for controlled testing. SOCKS5 is primarily a proxy interface; UDP support is not universal, and TCP-only proxy paths may be unsuitable for the game's realtime UDP snapshots.

Tor SOCKS and obfs4 may be evaluated as an experimental research transport in an isolated, authorised environment. This is not a promised production transport: Tor latency and congestion can make realtime gameplay poor, and obfs4 is not a general-purpose low-latency game relay. Measure connection success, RTT, jitter, loss, throughput and reconnect behaviour before considering any user-facing support.

Do not build covert tunnelling, traffic disguise or automatic proxy behaviour intended to circumvent a school/work network's controls. Use only networks and relays where the traffic is permitted, and fail clearly when policy or connectivity prevents a session.

## Pairing and security

- Discovery is not authentication.
- Require an explicit join/accept action and first-time pairing confirmation.
- Use a maintained cryptographic library and reviewed authenticated key exchange; never invent cryptography.
- Add replay protection, protocol-version negotiation, packet-size limits, rate limits and peer revocation.
- Keep credentials short-lived and scoped to a single room/session.
- Never log invitation secrets, pairing codes, private keys or full packet payloads.

## Node graph

Start with host-coordinated sessions and direct client-to-host links. A full mesh is not automatically better for 16 players. Optional peer-to-peer edges or multi-hop relays are later experiments, enabled only with explicit participant consent and permitted network/service policy. Require hop limits, loop prevention, bandwidth caps and end-to-end authentication.

## Test matrix

Test multicast-disabled networks, multiple adapters, IPv4/IPv6, client isolation, VLAN separation, blocked UDP, packet loss, NAT, proxy-only environments, host shutdown and reconnects. Include permitted relay tests and report blocked paths accurately. Never attempt to bypass access controls during testing.
