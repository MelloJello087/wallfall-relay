# Wallfall relay

A tiny server that lets Wallfall players join each other with a five-letter code.
It has no dependencies, just Node 18 or newer.

## Put it on Railway
1. Make a new GitHub repository (for example `wallfall-relay`) and upload these three files:
   `server.js`, `package.json`, `README.md`.
2. On railway.com, create a New Project, choose "Deploy from GitHub repo" and pick that repository.
   Railway sees `package.json` and runs `npm start` on its own.
3. Open the service, go to Settings > Networking, and click "Generate Domain".
   You get an address like `wallfall-relay-production.up.railway.app`.
4. Open that address in a browser. It should say "Wallfall relay is running".
5. In Wallfall (the downloaded file), open Co-op and paste the address into the Server box.
   Your friend pastes the same address. One of you hosts, the other joins with the code.

Railway gives the server its port through the PORT variable; the server reads it on its own.

## Test it on your own computer (optional)
    npm start
Then type `localhost:8080` into the Server box in the game's Co-op window.
