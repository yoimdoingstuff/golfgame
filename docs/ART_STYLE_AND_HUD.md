# Art direction and HUD reference

The target is a **clean-room, original minigolf game inspired by GMod Tower Minigolf**. It is not a recreation of Garry's Mod, Tower Unite, their maps, logos, fonts, textures or exact interface artwork. The reference is the presentation and play feel: small contained holes, bright surfaces, playful props, golf balls as the player identities, a clear shot-power UI and a compact scorecard.

## Gameplay rules carried over

- GMod Tower's minigolf description lists 10 players and an 18-hole flow. This game keeps the 18-hole match structure but targets up to 16 players.
- All players putt through each hole simultaneously. There is no golfer avatar and no turn queue.
- Players are represented by coloured balls; accessories such as hats sit directly on the ball.
- A player who holes out keeps their ball at the cup while everyone else finishes.

Reference pages:
- GMod Tower Minigolf overview: https://www.gmtower.org/index.php?gm=minigolf&p=gamemodes
- GMod Tower gamemode list: https://www.gmtower.org/index.php?p=gamemodes
- Tower Unite official Steam screenshots (later visual evolution of the same minigolf concept): https://store.steampowered.com/app/394690/Tower_Unite/

## Course look

- Bright, clean greens with simple, readable silhouettes.
- Pale cream/white edging and low boundary walls around raised fairways.
- Large themed props that read clearly from the third-person camera: ramps, bumpers, tunnels, rotating arms, lifts, launch pads and shortcuts.
- A course should feel like a deliberately designed toy miniature, not a realistic country club.
- Each hole is a self-contained diorama with a recognisable theme. Early themes: workshop, candy, rooftop, tropical and neon arcade.
- Use a small number of strong colours per hole. Avoid noisy textures that make aim lines and balls hard to read.
- Give each player a high-contrast ball colour. Keep labels close to the ball and do not use colour alone as the only identifier.

## Camera

- Angled third-person camera behind and above the local ball.
- The camera follows the ball as it rolls, with smooth damping and enough look-ahead to keep the next obstacle visible.
- Camera orbit is optional input; a camera reset button returns to the default follow angle.
- Spectating can focus another player's ball without switching player identity.

## HUD layout

The reference HUD keeps essential golf information near the lower-left corner:
- A horizontal POWER bar with a clear power fill.
- A compact row of HOLE, PAR and STROKE chips below it.
- A small player count / connection state in the opposite lower corner.
- A TAB scoreboard across the upper centre with player names and per-hole strokes. It can be expanded to the full scorecard for all 18 holes.
- A restrained network indicator in the room/menu screen. Do not cover the course with debug panels.

Our visual treatment uses a dark teal translucent panel, pale text, aqua highlights, and warm gold accents. It follows the same information hierarchy without copying the source UI artwork.

## Ball cosmetics

- Ball colour: available immediately from a small high-contrast palette.
- Hat/accessory: a tiny low-poly mesh attached to the ball, following its orientation.
- Trail: small particles behind moving balls with a strict particle cap.
- Cosmetics are visual only. They must not affect physics, collision, aim, or shot power.
- Wins and milestones unlock cosmetics; never make a gameplay advantage depend on progression.

## Originality and assets

Create original meshes, textures, sounds, UI layouts and course geometry. Do not extract or reuse GMod Tower/Tower Unite assets, proprietary maps, logos, or exact interface graphics. Use properly licensed external assets only when required and retain their notices.
