# TODO

Living priority list for the current prototype. Check items off only after the work is implemented and the relevant test has passed. See AGENTS.md before changing code and the project plan for the broader direction.

## Next up

- [ ] **Run the Windows game for real.** Launch the latest golfgame.exe, test menu/lobby/course input, resizing, aiming, shooting, collisions, cosmetics and results. Fix any runtime-only problems.
- [ ] **Test two Windows PCs on a permitted LAN.** Verify discovery, direct-IP join, simultaneous shots, remote ball updates, disconnects and host exit. Record network restrictions separately from game bugs.
- [ ] **Make multiplayer state trustworthy.** Validate packet sizes and player input, add sequence numbers/timestamps and rate limits, reject impossible ball states, and move shot/stroke/result authority to the host.
- [ ] **Smooth remote players.** Buffer and interpolate remote ball snapshots so other players do not jump between network updates.
- [ ] **Build the real multi-hole match flow.** Add data-driven hole loading, hole/par definitions, cumulative per-player scores, a full scorecard and match results. Award a win once at match completion, not once per hole.
- [ ] **Make obstacles fully playable.** Add and test collision/trigger behaviour for ramps, bumpers, moving arms, tunnels and hazards. A rendered obstacle is not complete until it affects gameplay correctly.
- [ ] **Finish camera and scoreboard controls.** Add reliable camera follow/orbit/reset and spectating, and show all players without silently truncating the scoreboard.

## Then

- [ ] Test 2, 4, 8 and 16 players; include packet loss, latency, late packets, disconnects and host shutdown.
- [ ] Add reconnect and clear user-facing connection/error states.
- [ ] Add versioned profile storage and validate cosmetic unlocks from trusted match results.
- [ ] Add original sound effects/music and polish ball trails, hats, course props and UI without bloating the portable build.
- [ ] Create several original themed holes and a maintainable course data format; consider a course editor only after the runtime format is stable.
- [ ] Measure and track the final Windows ZIP size; keep the game portable and avoid unnecessary runtime dependencies.
- [ ] Consider an internet relay only when the service and traffic are explicitly permitted. Consider Bluetooth only if a maintainable Windows implementation is practical.

## Not currently implemented

- Secure host-authoritative gameplay and public-internet security.
- Remote-ball interpolation and robust reconnect handling.
- Full 18-hole course progression and cumulative scorecard.
- Tested multi-machine 2-16 player gameplay.
- Internet relay and native Windows Bluetooth transport.
- Full audio, polished course library and course editor.

Do not mark any of the above complete solely because the relevant code compiles.
