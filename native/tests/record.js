// Records real game frames and sounds from Chrome for the C++ comparison. Set LOCAL_FONTS=1 to use assets/fonts.
const fs = require('fs'), path = require('path');
const puppeteer = require('puppeteer-core');
const chrome = process.env.CHROME || 'C:/Program Files/Google/Chrome/Application/chrome.exe';
const src = path.resolve(process.argv[2] || path.join(__dirname, '../../game/Wallfall.html'));
const out = path.join(__dirname, 'frames'); fs.mkdirSync(out, {recursive:true});

const hook = `window.__w = {enterRoom, RLIST, p, sfx, makeEnemy, get room(){return room}, get save(){return save}, get mode(){return mode}, set mode(v){mode = v}, set ac(v){ac = v}, get version(){return VERSION}};\n`;
let html = fs.readFileSync(src, 'utf8'); const end = html.lastIndexOf('})();');
html = html.slice(0, end) + hook + html.slice(end);
if (process.env.LOCAL_FONTS) {
  const f = path.resolve(__dirname, '../assets/fonts').split(path.sep).join('/');
  const face = (fam, file, st = 'normal') => `@font-face{font-family:"${fam}";src:url("file:///${f}/${file}");font-style:${st}}`;
  html = html.replace(/<link rel="stylesheet" href="https:\/\/fonts[^>]*>/, '<style>' + face('Silkscreen', 'Silkscreen-Regular.ttf') + face('IM Fell English', 'IMFeENrm28P.ttf') + face('IM Fell English', 'IMFeENit28P.ttf', 'italic') + face('IM Fell English SC', 'IMFeENsc28P.ttf') + face('Caveat', 'Caveat.ttf') + '</style>');
}
const page0 = path.join(out, 'game-hooked.html'); fs.writeFileSync(page0, html);

function recorder() {
  const P = CanvasRenderingContext2D.prototype, ids = new WeakMap(), gr = new WeakMap(), pt = new WeakMap(); let nextId = 1, gn = 0, pn = 0;
  const scratch = document.createElement('canvas').getContext('2d');
  const toURL = HTMLCanvasElement.prototype.toDataURL;
  const R = window.__r = {on:false, arm:false, done:false, log:[], shots:{}, touched:new Set(), sent:new Set(), ref:null, calls:0};
  const idOf = c => { if (c.id === 'game') return '0'; if (!ids.has(c)) ids.set(c, String(nextId++)); return ids.get(c); };
  const fsGet = Object.getOwnPropertyDescriptor(P, 'fillStyle').get, fsSet = Object.getOwnPropertyDescriptor(P, 'fillStyle').set;
  const norm = c => { fsSet.call(scratch, '#000'); fsSet.call(scratch, c); return fsGet.call(scratch); };
  const sendGrad = g => { const d = gr.get(g); if (R.sent.has(d.id)) return d.id; R.sent.add(d.id); R.log.push(d.def); for (const [o, c] of d.stops) R.log.push(`_\tstop\t${d.id}\t${o}\t${c}`); return d.id; };
  const styleOf = v => typeof v === 'string' ? v : v instanceof CanvasGradient ? sendGrad(v) : pt.get(v);
  const STATE = ['globalAlpha', 'globalCompositeOperation', 'lineWidth', 'lineCap', 'lineJoin', 'miterLimit', 'lineDashOffset', 'font', 'textAlign', 'textBaseline', 'shadowColor', 'shadowBlur', 'shadowOffsetX', 'shadowOffsetY'];
  const touch = (c, g) => {
    const id = idOf(c); if (R.touched.has(id)) return id; R.touched.add(id);
    const f = `c${id}.png`; R.shots[f] = toURL.call(c, 'image/png'); R.log.push(`${id}\tcanvas\t${c.width}\t${c.height}\t${f}`);
    g = g || c.getContext('2d'); const t = g.getTransform(); R.log.push(`${id}\tsetTransform\t${t.a}\t${t.b}\t${t.c}\t${t.d}\t${t.e}\t${t.f}`);
    for (const k of STATE) R.log.push(`${id}\t${k}\t${g[k]}`);
    for (const k of ['fillStyle', 'strokeStyle']) { const v = styleOf(g[k]); if (v) R.log.push(`${id}\t${k}\t${v}`); }
    R.log.push(`${id}\tsetLineDash\t${g.getLineDash().join('\t')}`);
    return id;
  };
  const num = a => a === undefined ? '0' : a === true ? '1' : a === false ? '0' : String(a);
  for (const m of ['save', 'restore', 'translate', 'scale', 'rotate', 'setTransform', 'beginPath', 'closePath', 'moveTo', 'lineTo', 'quadraticCurveTo', 'bezierCurveTo', 'rect', 'arc', 'ellipse', 'fill', 'stroke', 'clip', 'fillRect', 'strokeRect', 'clearRect', 'fillText', 'strokeText']) {
    const o = P[m]; const pad = m === 'arc' ? 6 : m === 'ellipse' ? 8 : 0;
    P[m] = function (...a) { if (R.on) { const id = touch(this.canvas, this); const b = a.slice(); while (b.length < pad) b.push(undefined); R.log.push([id, m, ...b.map(num)].join('\t')); R.calls++; } return o.apply(this, a); };
  }
  { const o = P.setLineDash; P.setLineDash = function (d) { if (R.on) R.log.push(`${touch(this.canvas, this)}\tsetLineDash\t${d.join('\t')}`); return o.call(this, d); }; }
  { const o = P.drawImage; P.drawImage = function (s, ...a) { if (R.on) { const id = touch(this.canvas, this), sid = touch(s); R.log.push([id, 'drawImage', sid, ...a.map(num)].join('\t')); R.calls++; } return o.call(this, s, ...a); }; }
  { const o = P.createLinearGradient; P.createLinearGradient = function (...a) { const g = o.apply(this, a), id = '@' + (++gn); gr.set(g, {id, def:`_\tlgrad\t${id}\t${a.join('\t')}`, stops:[]}); return g; }; }
  { const o = P.createRadialGradient; P.createRadialGradient = function (...a) { const g = o.apply(this, a), id = '@' + (++gn); gr.set(g, {id, def:`_\trgrad\t${id}\t${a.join('\t')}`, stops:[]}); return g; }; }
  { const o = CanvasGradient.prototype.addColorStop; CanvasGradient.prototype.addColorStop = function (x, c) { const d = gr.get(this); if (d) d.stops.push([x, norm(c)]); return o.call(this, x, c); }; }
  { const o = P.createPattern; P.createPattern = function (s, r) { const q = o.call(this, s, r), id = '%' + (++pn); pt.set(q, id); if (R.on) R.log.push(`_\tpattern\t${id}\t${touch(s)}`); return q; }; }
  for (const k of ['fillStyle', 'strokeStyle']) { const d = Object.getOwnPropertyDescriptor(P, k); Object.defineProperty(P, k, {get:d.get, set(v) { d.set.call(this, v); if (R.on) { const id = touch(this.canvas, this), s = styleOf(d.get.call(this)); if (s) R.log.push(`${id}\t${k}\t${s}`); } }}); }
  for (const k of STATE) { const d = Object.getOwnPropertyDescriptor(P, k); Object.defineProperty(P, k, {get:d.get, set(v) { d.set.call(this, v); if (R.on) R.log.push(`${touch(this.canvas, this)}\t${k}\t${d.get.call(this)}`); }}); }
  const raf = window.requestAnimationFrame.bind(window);
  window.requestAnimationFrame = cb => raf(t => {
    if (R.arm && cb.name === 'frame') { R.arm = false; R.log = []; R.shots = {}; R.touched = new Set(); R.sent = new Set(); R.calls = 0; R.on = true; try { cb(t); } finally { R.on = false; R.ref = toURL.call(document.getElementById('game'), 'image/png'); R.done = true; } } else cb(t);
  });
}

const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const b = await puppeteer.launch({executablePath:chrome, headless:true, args:['--no-sandbox', '--mute-audio', '--disable-gpu', '--disable-accelerated-2d-canvas', '--disable-lcd-text', '--allow-file-access-from-files']});
  const page = await b.newPage(); await page.setViewport({width:1200, height:800});
  const errors = []; page.on('pageerror', e => errors.push(String(e)));
  await page.evaluateOnNewDocument(recorder);
  await page.goto('file:///' + page0.replace(/\\/g, '/'), {waitUntil:'load'});
  await page.waitForFunction(() => window.__w);
  await page.evaluate(async () => { await Promise.all(['8px Silkscreen', '13px "IM Fell English"', 'italic 13px "IM Fell English"', '20px "IM Fell English SC"', '20px Caveat'].map(f => document.fonts.load(f))); await document.fonts.ready; });
  await sleep(1200);
  const version = await page.evaluate(() => window.__w.version);
  const snap = async name => {
    await page.evaluate(() => { const R = window.__r; R.done = false; R.arm = true; });
    await page.waitForFunction(() => window.__r.done, {timeout:15000});
    const r = await page.evaluate(() => ({log:window.__r.log.join('\n'), shots:window.__r.shots, ref:window.__r.ref, calls:window.__r.calls}));
    fs.mkdirSync(path.join(out, name), {recursive:true});
    fs.writeFileSync(path.join(out, name, name + '.txt'), r.log);
    fs.writeFileSync(path.join(out, name, name + '.png'), Buffer.from(r.ref.split(',')[1], 'base64'));
    for (const [f, u] of Object.entries(r.shots)) fs.writeFileSync(path.join(out, name, f), Buffer.from(u.split(',')[1], 'base64'));
    console.log(name, r.calls, 'drawing calls');
  };
  await snap('title');
  const rooms = await page.evaluate(() => { const L = window.__w.RLIST, keep = []; for (let i = 0; i < 7; i++) keep.push(L[Math.floor(i * (L.length - 1) / 6)].key); return keep; });
  for (const [i, k] of rooms.entries()) {
    await page.evaluate(k => { const w = window.__w; w.save.cls = w.save.cls || 'warden'; w.save.clsChosen = true; w.save.seenIntro = true; w.enterRoom(k); w.mode = 'play'; }, k);
    await sleep(700);
    await page.evaluate(() => { window.__w.mode = 'play'; });
    await sleep(120);
    await snap('room' + (i + 1));
  }
  const sounds = await page.evaluate(async () => {
    const w = window.__w, r = {};
    for (const n of ['jump', 'air', 'dash', 'swing', 'heavy', 'fire', 'shoot', 'hit']) {
      const a = new OfflineAudioContext(1, 24000, 48000); w.ac = a; w.sfx[n](); const buf = await a.startRendering(), d = buf.getChannelData(0);
      r[n] = btoa(String.fromCharCode(...new Uint8Array(d.buffer).reduce((o, v, i) => { o.push(v); return o; }, [])));
    }
    w.ac = null; return r;
  });
  fs.mkdirSync(path.join(out, 'sound'), {recursive:true});
  for (const [n, s] of Object.entries(sounds)) fs.writeFileSync(path.join(out, 'sound', n + '.f32'), Buffer.from(s, 'base64'));
  fs.writeFileSync(path.join(out, 'scenes.txt'), ['title', ...rooms.map((_, i) => 'room' + (i + 1))].join('\n') + '\n');
  console.log('browser version', version, '| rooms', rooms.join(' '), '| page errors', errors.length, errors.slice(0, 3));
  await b.close();
})().catch(e => { console.error(e); process.exit(1); });
