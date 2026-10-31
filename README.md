# Wallfall: game + co-op relay server

This repo has two things:

- **`game/Wallfall.html`** is the whole game in one file. Download it and double-click it to play in a browser.
- **`server.js`** (with `package.json`) is a tiny relay server that lets players join each other's game with a five-letter code. It needs no packages, just Node 18 or newer.

## Setting up the server on Railway (one person does this once)
1. On railway.com, create a **New Project**, choose **Deploy from GitHub repo**, and pick this repository
   (or fork it to your own GitHub first and pick your fork).
   Railway sees `package.json` and runs `npm start` on its own. The game file in `game/` is ignored by the server.
2. Open the service, go to **Settings > Networking**, and click **Generate Domain**.
   You get an address like `wallfall-relay-production.up.railway.app`.
3. Open that address in a browser. It should say "Wallfall relay is running".
4. Send that address to everyone who wants to play.

Railway gives the server its port through the `PORT` variable; the server reads it on its own.

## Playing together
1. Everyone downloads `game/Wallfall.html` (open it on GitHub, click the download button) and opens it in a browser.
2. Click **Co-op** and paste the server address into the **Server** box. The game remembers it.
3. One player clicks **Host a game** and shares the five-letter code. The others type the code and click **Join**.

Each player keeps their own save. Up to four players per game.

## Test the server on your own computer (optional)
    npm start
Then type `localhost:8080` into the Server box in the game's Co-op window.
