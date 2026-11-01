# Wallfall native (C++)

**Owned by mlrgamestudios's agent. No one else edits this folder.**

If you are MelloJello087's agent: do not create, change, move or delete anything in `native/`, and keep working on `game/Wallfall.html` as normal. If you think something here needs changing, write it under Open questions in NOTES.md instead.

This is a C++ version of Wallfall for a Steam release. It has to look, sound and play the same as the browser game, so `game/Wallfall.html` is the reference it is matched against. The browser game is not being replaced and carries on as usual.

**Status:** the proof works, but the game itself is not ported yet. See [RESEARCH.md](RESEARCH.md) for the plan and the proof's results.

## Build
Needs Visual Studio 2026 and CMake. Skia and SDL3 download by themselves into `deps/` on the first run, checked against pinned hashes.
```
cmake -S native -B native/build -G "Visual Studio 18 2026" -A x64
cmake --build native/build --config Release
```

## Run the proof
`native/build/Release/wallfall-proof.exe` opens the window. Move with the arrows, WASD or a controller stick. A or Space plays the jump sound, X or J the heavy sound, Esc quits.

## Compare with the browser game
```
cd native/tests && npm install && node record.js
sh native/tests/compare.sh
```
`record.js` plays the real game in Chrome and saves each chosen frame's drawing calls, Chrome's pixels and the sound effects to `tests/frames/`. `compare.sh` replays them in C++ and scores the match. It also writes `.cpp.png` and `.diff.png` beside each frame.

## Files
- `src/canvas.*`: the browser's 2D drawing calls, same names and rules, on Skia.
- `src/sound.*`: Chrome's oscillators and the game's `tone()`.
- `src/replay.cpp`: replays recorded frames and scores them.
- `src/proof.cpp`: the window, controller and sound checks.
- `assets/fonts/`: the game's fonts, with their licences.
