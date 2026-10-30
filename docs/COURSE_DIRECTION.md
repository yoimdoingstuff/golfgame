# Course and match direction

This is a compact GMod Tower-style minigolf match, not a persistent social world. Players load into a match and are represented by their golf balls.

## Match flow

1. Main menu: Play, Find Match, Host Match, Join by IP, Settings.
2. Small room screen: choose course/options; up to 16 players join.
3. Course intro: short flyover and all golf balls spawn at their tees.
4. Round: everybody can aim and shoot simultaneously. All balls and stroke totals remain visible while players watch one another.
5. Results: scorecard, winner, win progress, rematch and return to menu.

## Course feel

- Compact, self-contained holes, not a giant open world.
- Bright, readable low-poly geometry and exaggerated obstacle scale.
- Playful obstacles: moving platforms, rotating arms, launch pads, tunnels, banked walls, lifts, bumpers, ramps and shortcuts.
- Themes could include toy room, rooftop, workshop, pirate ship or neon arcade.
- Keep visual clutter away from ball paths so shots remain readable.

## Ball presentation

- Each player owns one golf ball with a unique player ID and display name.
- Ball colour is the primary visual identifier.
- Optional hats/accessories attach to the top of the ball and follow its position/rotation.
- Optional trails emit behind moving balls, with particle caps and distance-based reduction.
- No player body, golfer mesh, skeleton or humanoid animation system.
- Players can select another ball to spectate, smoothly moving the camera focus.

## Fair multiplayer rules

- All players may shoot simultaneously.
- Players finish independently; completed balls stay by the cup while others finish.
- Host relays ball state and coordinates match completion; authoritative shot validation is planned.
- Clients interpolate remote ball motion from host snapshots.
- Late join is disabled after the first hole starts in the initial release.
