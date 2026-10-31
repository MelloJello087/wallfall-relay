# Wallfall notes

The rules are in [CLAUDE.md](CLAUDE.md). This file is the live status. Keep it short: delete what is finished or no longer true.

If two people edit this file at once and git reports a conflict, keep both sets of lines.

## Claims
Who is working on what right now. Add a line before you start and remove it when you've pushed.

Format: `- who: what, in which part of the game (date)`

- MelloJello087's agent: parrying, a new key, in player combat and hurt() (2026-10-31)
- MelloJello087's agent: a second attack (skill) for every class, in CLASSES and player combat (2026-10-31)
- MelloJello087's agent: start with only dash; double jump and wall jump become pickups, map adjusted so it still progresses, in rooms, items and movement (2026-10-31)

## Handoff
Newest first. Keep each entry to a few lines: what changed, what is unfinished, what the other person should know. Delete entries once they stop being useful.

- **2026-10-31, MelloJello087's agent, v0.1.2:** Look screen: every group (hood, tunic, eyes, lamp, skin) has a rainbow "pick any color" swatch, and skin has a "One tone, no shading" box. Saved in `save.look` as `<group>:'custom'` plus `<group>C:'#hex'` and `skinFlat`, so co-op partners see it too (old versions just fall back). Tested in Chrome: picking colors updates the preview and saves.

- **2026-10-30, mlrgamestudios's agent, v0.1.1:** Every class can now kill every small enemy. Before, Mage, Crow Hunter and Gunner shots flew over rats and spiders on flat ground, and Sapper bombs overshot anything close. Bolts, bombs and thrown bones now lean toward the small enemy types in `SMALL` when one is ahead and not behind a wall, and thralls throw bones at flyers. Melee and Stormcaller were already fine and did not change. Tested in Chrome: all 13 classes against rats, spiders, crows, wisps and shardlings, plus a co-op guest over the real relay. Eels were not tested, because the test floor had no water.
- **2026-10-30, mlrgamestudios's agent, v0.1.0:** Added the version mark in the corner of the screen, hidden in full screen. New rule: every game update adds 0.0.1. See Versions in CLAUDE.md, and always bump last, after your final pull.
- **2026-10-30, MelloJello087's agent:** Linked up: I've read CLAUDE.md and this file and will follow the sync loop from now on. I built the game so far and will now edit `game/Wallfall.html` here directly. Last things I pushed: full screen, checkpoint lanterns, and co-op showing each other's thralls, shots, bombs, lightning and blasts. On Railway auto-deploy: as of 2026-10-30 22:44 the live site still showed the old server after two pushes, so it does not seem to redeploy on its own; it needs "Deploy Latest Commit" in Railway.
- **2026-10-30, mlrgamestudios's agent:** Added a required sync loop at the top of CLAUDE.md: pull on every human message and before every change, then test, leave notes and push after every finished update. Follow it from now on.
- **2026-10-30, mlrgamestudios's agent:** Added CLAUDE.md and this file so both agents stay in sync. No game changes. Please read CLAUDE.md, and correct anything in it that doesn't match how you've been working.

## Open questions
Things only the humans can answer.

- What are the next goals for the game? The Goals list in CLAUDE.md only covers what already exists.
- Railway's Source settings show "Auto deploy unavailable" and "Could not load branches" (seen 2026-10-30), so pushes do not redeploy the server. Game updates don't need a redeploy and are unaffected. MelloJello087 needs to check the Railway GitHub App's access to this repo. Once auto-deploy works, set Watch Paths to `/server.js` and `/package.json`, or every push will drop open co-op games, then update "Pushing to main" in CLAUDE.md.

## Ideas
Not agreed to, just not forgotten.

- The co-op code mentions Steam lobbies as a possible future option.
