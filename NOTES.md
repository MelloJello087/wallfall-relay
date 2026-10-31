# Wallfall notes

The rules are in [CLAUDE.md](CLAUDE.md). This file is the live status. Keep it short: delete what is finished or no longer true.

If two people edit this file at once and git reports a conflict, keep both sets of lines.

## Claims
Who is working on what right now. Add a line before you start and remove it when you've pushed.

Format: `- who: what, in which part of the game (date)`

- MelloJello087's agent: slower movement; Attack/Defense/Support roles and 6 new classes; charged attack, a second skill key, air and down attacks for every class; accounts with cloud saves (server.js + game). Classes, combat, movement, server (2026-10-31)

## Handoff
Newest first. Keep each entry to a few lines: what changed, what is unfinished, what the other person should know. Delete entries once they stop being useful.

- **2026-10-31, MelloJello087's agent, v0.1.6:** MelloJello087 wants the double jump earned by everyone, so old saves lose it too: `worldV` 4 migration sets `dbl:false`, un-finds the Wingbone, clears a lit lantern in a room that needs the double jump (G L M N Q R X), and starts you at the Training Yard. Progress, items and bosses stay. Also removed the free color picker from the Look screen (he didn't like it) and added preset swatches instead: 7 more skin tones, 4 tunics, 3 eye glows, 2 lamp flames. The one-tone skin box stays. Looks saved with a picked color still draw. Tested in Chrome: old saves at worldV 2 and 3, the second jump does nothing, wardrobe works.
- **2026-10-31, MelloJello087's agent, v0.1.5:** New games start with only the dash. The ability order is now: grapple (Ropewalk, as before), then the **Wingbone** double jump (new item `Y`, Wallwalk 28,15, right before the 5-high pillar), then the **Iron Claws** wall jump (new item `H`, Belfry 55,7; hold toward a wall in the air to slide, jump to kick off). Save fields `dbl` and `wall`; `airJ()` replaces `cls().airJumps || 1`. To make up for it, the normal jump is a bit higher (-8.3, clears 4 tiles), and two ledges were added so you can't get stuck without the double jump: Breach (63-65, row 12) and Undercroft (2-5, row 12). (Superseded in v0.1.6: old saves lose the double jump too.) Checked with a reachability script and headless runs; the claws open no new areas yet, they just make climbing easier. The HUD shows 5 badges now, and the cog counter moved right.
- **2026-10-31, MelloJello087's agent, v0.1.4:** Every class has a second move on a new key (Q or L; a Skill button on touch) with a cooldown, shown bottom left. `SKILLS` table plus `useSkill`/`skillTick` in the "Class skills" section: Riposte Spin, Fire Nova, Earthsplitter, Volley, Shadowstep, Bone Spikes, Shield Charge, Rising Pike, Cluster Charge, Thunderfall, Whirlwind, Palm Blast, Buckshot. The touch buttons are now two groups (`.tpad`, `.tact`). Tested headless: every class's skill fires; 11 of 13 hurt enemies standing next to you (Rising Pike only hits above you, Whirlwind needs them closer).
- **2026-10-31, MelloJello087's agent, v0.1.3:** Parry: new key (E or K, rebindable; a Parry button on touch). A 14-frame window; a hit inside it does no damage, staggers the nearest enemy for about 1.3 s (`e.stun`, skipped AI in `updateEnemy`), and resets the parry so they chain. Enemy shots inside the window come back as your own fire bolt. Bosses don't stagger. Tested headless: parried hit kept full health and stunned the enemy; an unparried one hurt.
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
