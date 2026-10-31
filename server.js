// Wallfall relay: a tiny WebSocket server that lets players find each other by a five-letter code
// and passes each player's state to everyone else in the same game. No packages needed.
const http = require('http');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const PORT = process.env.PORT || 8080;
const MAX_PLAYERS = 4;          // per game
const MAX_MESSAGE = 16 * 1024;  // bytes; a player's state is about 4 KB at most
const games = new Map();        // code -> Set of clients

// the game itself is served from the same address, so players just open the site and play.
// It is fetched fresh from GitHub (checked at most once a minute), so pushing a new game file updates the
// site without redeploying. If GitHub can't be reached, the copy that shipped with the server is used.
const GAME_FILE = path.join(__dirname, 'game', 'Wallfall.html');
const GAME_URL = process.env.GAME_URL || 'https://raw.githubusercontent.com/MelloJello087/wallfall-relay/main/game/Wallfall.html';
let cached = null, cachedAt = 0;
async function gamePage() {
  if (!cached || Date.now() - cachedAt > 60000) {
    cachedAt = Date.now();
    try {
      const r = await fetch(GAME_URL, {signal: AbortSignal.timeout(5000)});
      if (r.ok) { const t = await r.text(); if (t.includes('Wallfall')) cached = t; }
    } catch (e) {}
    if (!cached) try { cached = fs.readFileSync(GAME_FILE, 'utf8'); } catch (e) {}
  }
  // a marker tells the game it was loaded from this server, so co-op connects back here on its own
  return cached ? cached.replace('<head>', '<head><meta name="wallfall-relay" content="1">') : null;
}
const server = http.createServer(async (req, res) => {
  const url = (req.url || '/').split('?')[0];
  if (url === '/health' || url === '/status') {
    res.writeHead(200, {'Content-Type': 'text/plain'});
    res.end(`Wallfall relay is running. Games open: ${games.size}\n`);
    return;
  }
  if (url === '/' || url === '/index.html' || url === '/Wallfall.html') {
    const page = await gamePage();
    if (page) { res.writeHead(200, {'Content-Type': 'text/html; charset=utf-8', 'Cache-Control': 'no-cache'}); res.end(page); return; }
    res.writeHead(500, {'Content-Type': 'text/plain'}); res.end('The game file (game/Wallfall.html) is missing from the server.\n'); return;
  }
  res.writeHead(404, {'Content-Type': 'text/plain'}); res.end('Not found\n');
});

server.on('upgrade', (req, socket) => {
  const key = req.headers['sec-websocket-key'];
  if (!key || (req.headers.upgrade || '').toLowerCase() !== 'websocket') { socket.destroy(); return; }
  const accept = crypto.createHash('sha1').update(key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64');
  socket.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ' + accept + '\r\n\r\n');
  socket.setNoDelay(true);
  const client = {socket, id: crypto.randomBytes(4).toString('hex'), code: null, role: null, buf: Buffer.alloc(0), alive: true};
  socket.on('data', chunk => { client.buf = Buffer.concat([client.buf, chunk]); readFrames(client); });
  socket.on('close', () => leave(client));
  socket.on('error', () => leave(client));
});

// --- WebSocket framing (just what browsers send: masked text frames, ping, close)
function readFrames(c) {
  while (c.buf.length >= 2) {
    const b0 = c.buf[0], b1 = c.buf[1], op = b0 & 15, masked = b1 & 128;
    let len = b1 & 127, off = 2;
    if (len === 126) { if (c.buf.length < 4) return; len = c.buf.readUInt16BE(2); off = 4; }
    else if (len === 127) { if (c.buf.length < 10) return; len = Number(c.buf.readBigUInt64BE(2)); off = 10; }
    if (len > MAX_MESSAGE) { c.socket.destroy(); return; }
    const need = off + (masked ? 4 : 0) + len;
    if (c.buf.length < need) return;
    let data = c.buf.subarray(off + (masked ? 4 : 0), need);
    if (masked) { const m = c.buf.subarray(off, off + 4); data = Buffer.from(data); for (let i = 0; i < data.length; i++) data[i] ^= m[i & 3]; }
    c.buf = c.buf.subarray(need);
    if (op === 8) { c.socket.end(); return; }            // close
    if (op === 9) { send(c, data, 10); continue; }      // ping -> pong
    if (op === 1) onMessage(c, data.toString('utf8'));  // text
  }
}
function send(c, payload, op = 1) {
  if (c.socket.destroyed) return;
  const data = Buffer.isBuffer(payload) ? payload : Buffer.from(payload);
  let head;
  if (data.length < 126) head = Buffer.from([128 | op, data.length]);
  else if (data.length < 65536) { head = Buffer.alloc(4); head[0] = 128 | op; head[1] = 126; head.writeUInt16BE(data.length, 2); }
  else { head = Buffer.alloc(10); head[0] = 128 | op; head[1] = 127; head.writeBigUInt64BE(BigInt(data.length), 2); }
  c.socket.write(Buffer.concat([head, data]));
}
const sendJSON = (c, obj) => send(c, JSON.stringify(obj));

// --- games
function onMessage(c, text) {
  let m; try { m = JSON.parse(text); } catch (e) { return; }
  c.alive = true;
  if (m.t === 'join') {
    const code = String(m.code || '').toUpperCase().replace(/[^A-Z0-9]/g, '').slice(0, 5);
    if (code.length !== 5 || c.code) return;
    const game = games.get(code);
    if (m.role === 'host') {
      if (game && [...game].some(o => o.role === 'host')) return sendJSON(c, {t:'err', m:'taken'});
    } else {
      if (!game || ![...game].some(o => o.role === 'host')) return sendJSON(c, {t:'err', m:'No game is hosting with that code.'});
      if (game.size >= MAX_PLAYERS) return sendJSON(c, {t:'err', m:'That game is full.'});
    }
    c.code = code; c.role = m.role === 'host' ? 'host' : 'guest';
    if (!games.has(code)) games.set(code, new Set());
    games.get(code).add(c);
    sendJSON(c, {t:'ok', id:c.id});
  } else if (m.t === 's' && c.code) {
    const out = JSON.stringify({id:c.id, s:m.s});
    for (const o of games.get(c.code) || []) if (o !== c) send(o, out);
  }
}
function leave(c) {
  if (!c.code) return;
  const game = games.get(c.code); c.code = null;
  if (!game) return;
  game.delete(c);
  for (const o of game) sendJSON(o, {id:c.id, leave:true});
  if (!game.size) games.delete(c.code);
  if (!game.size) for (const [k, v] of games) if (v === game) games.delete(k);
}
// drop connections that have gone quiet
setInterval(() => {
  for (const game of games.values()) for (const c of game) { if (!c.alive) c.socket.destroy(); c.alive = false; }
}, 20000);

server.listen(PORT, '0.0.0.0', () => console.log('Wallfall listening on port ' + PORT + ' (game at /, status at /health)'));
