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
3. Open that address in a browser: the game loads right there. (`/health` shows "Wallfall relay is running".)
4. Send that address to everyone who wants to play.

The server listens on the `PORT` Railway gives it, or 8080 if there is none.

Railway gives the server its port through the `PORT` variable; the server reads it on its own.

## Playing together
1. Everyone opens the Railway address in a browser. The game runs there, and its co-op already talks to that same server.
2. One player clicks **Co-op** and **Host a game**, and shares the five-letter code. The others click **Co-op**, type the code and click **Join**.

(You can also download `game/Wallfall.html` and open it from your computer; then paste the Railway address into the **Server** box in the Co-op window.)

Each player keeps their own save. Up to four players per game.

## Test the server on your own computer (optional)
    npm start
Then open http://localhost:8080 in two browser tabs.
