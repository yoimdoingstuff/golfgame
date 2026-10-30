# Ball cosmetics and win progression

Golf balls are the players. Customisation attaches to the ball itself; the game has no golfer characters or humanoid player bodies.

## Cosmetic slots

| Slot | Examples | Behaviour |
|---|---|---|
| Ball colour | red, blue, lime, purple, metallic, pastel | Changes the ball material. Keep enough contrast between players. |
| Hat/accessory | top hat, propeller cap, tiny crown, party hat, traffic cone | Small low-poly mesh attached above the ball; follows it as it rolls. |
| Trail | sparkles, stars, smoke puffs, rainbow ribbon, leaves | Cosmetic particles emitted behind a moving ball. |

Cosmetics never change mass, friction, bounce, aiming, shot power or collision. Unlocks are purely visual.

## Example win unlocks

| Wins | Reward |
|---:|---|
| 0 | Starter colour set |
| 1 | Party hat |
| 3 | Propeller cap |
| 5 | Sparkle trail |
| 10 | Crown accessory |
| 15 | Rainbow trail |
| 25 | Metallic ball colours |
| 50 | Champion trail |

Keep basic readable colours available from the start. Rewards add flair, not essential visibility or gameplay advantages.

## Profile

A versioned local profile should store the display name, total wins, completed matches, unlocked cosmetic IDs and equipped colour/hat/trail. The host can replicate cosmetic IDs so everybody renders the same ball.

Do not trust a client's claimed win count or unlocked item IDs as proof. The current peer-hosted prototype carries cosmetic identifiers for display only; profile persistence and unlock validation are future work.

## Implementation order

1. Choose each ball's colour from a fixed high-contrast palette.
2. Replicate colour and cosmetic IDs in player snapshots.
3. Add one simple hat mesh attached to the ball.
4. Add one inexpensive trail effect with particle limits.
5. Add local profile saving and win-based unlock thresholds.
6. Add a ball-customisation menu.

The networking prototype already carries ball colour, hat ID and trail ID. Rendering, persistence, unlock validation and the customisation menu are not yet implemented.
