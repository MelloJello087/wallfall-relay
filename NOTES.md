# Wallfall notes

The rules are in [CLAUDE.md](CLAUDE.md). This file is the live status. Keep it short: delete what is finished or no longer true.

If two people edit this file at once and git reports a conflict, keep both sets of lines.

## Claims
Who is working on what right now. Add a line before you start and remove it when you've pushed.

Format: `- who: what, in which part of the game (date)`

- mlrgamestudios: letting every class hit small enemies (bolts, bombs, melee reach, thralls), in the Class powers and Update sections of game/Wallfall.html (2026-10-30)

## Handoff
Newest first. Keep each entry to a few lines: what changed, what is unfinished, what the other person should know. Delete entries once they stop being useful.

- **2026-10-30, MelloJello087's agent:** Linked up: I've read CLAUDE.md and this file and will follow the sync loop from now on. I built the game so far and will now edit `game/Wallfall.html` here directly. Last things I pushed: full screen, checkpoint lanterns, and co-op showing each other's thralls, shots, bombs, lightning and blasts. On Railway auto-deploy: as of 2026-10-30 22:44 the live site still showed the old server after two pushes, so it does not seem to redeploy on its own; it needs "Deploy Latest Commit" in Railway.
- **2026-10-30, mlrgamestudios's agent:** Added a required sync loop at the top of CLAUDE.md: pull on every human message and before every change, then test, leave notes and push after every finished update. Follow it from now on.
- **2026-10-30, mlrgamestudios's agent:** Added CLAUDE.md and this file so both agents stay in sync. No game changes. Please read CLAUDE.md, and correct anything in it that doesn't match how you've been working.

## Open questions
Things only the humans can answer.

- What are the next goals for the game? The Goals list in CLAUDE.md only covers what already exists.
- Please confirm in Railway that auto-deploy is off. It looks off: the live site still had the old server after two pushes on 2026-10-30. If it's on, every push drops open co-op games, including pushes that only change notes.

## Ideas
Not agreed to, just not forgotten.

- The co-op code mentions Steam lobbies as a possible future option.
