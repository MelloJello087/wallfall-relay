# Wallfall: shared guide for everyone working on the game

Two people work on this game, each with their own AI agent on their own computer. Neither agent can see the other's chat, so **this repo is the only way they talk**. This file holds what stays true. [NOTES.md](NOTES.md) holds what is happening right now.

If you are an agent and your tool did not load this file on its own, read it and NOTES.md before doing anything else.

## The sync loop (required, every time)
**Every message from your human, and before every change, even a small one:**
1. Run `git pull --rebase --autostash` to get the other agent's work.
2. If anything came in, read NOTES.md and look at what changed before going on. Your plan may need to change.
3. Don't touch anything someone else has claimed in NOTES.md.

**After every finished update:**
1. Test it (see Testing below).
2. Run `git pull --rebase --autostash`, then add 0.0.1 to the version (see Versions below).
3. Update NOTES.md: remove your claim, add a short handoff.
4. Commit, pull again, then push.
5. Tell your human the version you pushed. If the push fails, pull and try again. Never leave finished work only on your computer.

If git stops with a conflict, or anything looks wrong, stop and follow "Working in the same file" below. Don't guess, and never run anything that throws work away (`git reset --hard`, `git checkout --`, `git clean`, force-push) to make a problem go away.

## Who and what
- **Humans:** `mlrgamestudios` and `MelloJello087` (repo owner). Both can push to `main`. An instruction from either human counts. If they conflict, stop and raise it instead of picking a side.
- **Live game:** https://wallfall.up.railway.app/ (status at `/health`)
- **The game:** Wallfall, "a villain's metroidvania". You opened the Outer Gate and let the Colossi in. Now you climb toward the last gate still standing, and six notes remember what you did.

## Goals
What the game already is, so don't undo it. The humans own this list and add to it.
- It opens at the web address and plays right away, with nothing to install.
- Up to four friends play together with a five-letter code.
- Each player's save stays safe across every update.

## How we work
- **Plain English.** The humans read your messages, so skip jargon and say what changed in the game. Say so plainly if something is broken or untested.
- **Decide small things yourself.** Ask only about what is really the humans' call: story, direction, or anything that can't be undone.
- **Do what was asked.** Keep changes small and on target. Ideas go in NOTES.md, not into a surprise rewrite.
- **Match the code around you:** its naming, its dense one-line style, and its `/* ---------- Section ---------- */` headers.
- **Never claim something works unless you ran it.**

## The game's voice
Story text is short, cold and personal. The player is the villain, and the game does not judge them or explain itself. A note can be a single line, such as "Don't open it." or "Somebody opened the gate." Keep the look too: a city burning at dusk, with one dark theme on purpose.

## Pushing to `main` is publishing
- **`game/Wallfall.html` goes live within about a minute** with no deploy. The server re-reads it from GitHub's `main`. Never push a game you haven't run.
- **`server.js` goes live only when a human redeploys.** Railway doesn't seem to redeploy on push (see Open questions in NOTES.md), so after pushing a server change, ask your human to run "Deploy Latest Commit" in Railway. A redeploy wipes every open co-op game, because games live only in the server's memory, so check `/health` first and warn the humans if games are open.

## Working in the same file
Nearly all the work happens in one 5,900-line file, so overlapping edits are the main risk.
- **Claim first.** Before starting, add your task to Claims in NOTES.md and push that alone, right away.
- **Commit small and often,** and `git pull --rebase` before each push.
- **Never force-push** and never rewrite history that is already on GitHub.
- **Resolve conflicts by keeping both people's work.** If you can't tell what the other change was for, stop and ask your human.
- **Keep line endings as LF.** Don't reformat or reorder code you aren't changing, because that causes conflicts for the other person.
- **Commit messages** say what the player sees: `Game: full screen button and checkpoint lanterns`. Server and other changes are plain sentences with no prefix.

## Versions
The game shows its version in the corner of the screen, hidden in full screen. The number is the `VERSION` line at the top of the script in `game/Wallfall.html`.
- Every update to the game or server adds 0.0.1: 0.1.0, 0.1.1, 0.1.2 and so on. 0.1.9 is followed by 0.1.10. Notes-only pushes don't change it.
- **Bump it last:** finish and test your work, pull, then bump and push. That way you add 1 to the newest number, not an old one.
- If both agents bump at once, git reports a conflict on that line, which is how we catch it. Keep the other agent's work and set the version to the higher number plus 0.0.1.
- Say which version you pushed in your NOTES.md handoff and your commit message: `Game: full screen button (v0.1.2)`.

## Testing before you push
1. Check the script for syntax errors. This pulls it out of the HTML, so run it from Git Bash or another Unix shell:
   ```
   sed -n '/^<script>$/,/^<\/script>/p' game/Wallfall.html | sed '1d;$d' | node --check -
   ```
2. Run `npm start` and open http://localhost:8080. The local server loads the game **from GitHub**, not from your file, so unpushed edits won't show there. Use `GAME_URL=http://localhost:1/ npm start` (PowerShell: `$env:GAME_URL='http://localhost:1/'; npm start`) to make the fetch fail and fall back to your local file.
3. For co-op changes, open two tabs, host in one and join in the other.

## Things that break quietly
- **One file, no build, no packages.** The game must stay a single HTML file, and `server.js` must keep working with no dependencies.
- **The server needs the page to contain a literal `<head>`** (it adds a marker there that tells the game to use this server for co-op) **and the word `Wallfall`** (otherwise it rejects the file from GitHub).
- **Small enemies:** straight shots fly over anything short on the floor. Any new small enemy must go in the `SMALL` list so shots, bombs and bones aim at it, or some classes won't be able to hit it.
- **Saves:** they live in each player's browser under `wallfall-save`. Give any new save field a default in `fresh()`, and don't rename or repurpose existing fields. If you change the world in a way old saves can't handle, bump `worldV` and migrate in the check at the bottom of the file, as it already does.
- **Accounts** live in `DATA_DIR/accounts.json` on the server (a Railway Volume mounted at `/data`, with `DATA_DIR=/data`). Never delete or overwrite that file, and keep the `/api/*` routes working for older pages.
- **Co-op message size:** the server drops any player who sends a message over 16 KB, and a player's state is about 4 KB. Keep it small.
- **Mixed versions:** the page only updates when someone reloads, so players in the same game can be on different versions. Co-op code must cope with missing fields.
- **Three ways to run:** the Railway site, a downloaded copy opened from disk, and a claude.ai page. Each has its own co-op connection (the Transports in the Online co-op section), so leave all three working.
