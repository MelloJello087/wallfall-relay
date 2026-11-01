#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include "canvas.h"
#include "sound.h"

namespace wf { int replay(const std::string &dir, const std::string &name); bool savePng(const std::string &path, const void *px, int w, int h, size_t rb); }
using namespace wf;

static const int VW = 512, VH = 288, RATE = 48000;

/* ---------- Sound check: compare with Chrome's recording ---------- */
static int compareSound(const std::string &dir) {
  int bad = 0;
  for (auto &n : sfxNames()) {
    std::ifstream f(dir + "/" + n + ".f32", std::ios::binary); if (!f) { printf("%s: no recording\n", n.c_str()); bad++; continue; }
    std::vector<char> raw((std::istreambuf_iterator<char>(f)), {}); size_t c = raw.size() / 4; const float *ref = (const float *)raw.data(); auto mine = renderSfx(n, RATE, double(c) / RATE);
    double e = 0, a = 0, b = 0, ab = 0, peak = 0; size_t m = std::min(c, mine.size());
    for (size_t i = 0; i < m; i++) { double d = ref[i] - mine[i]; e += d * d; a += ref[i] * ref[i]; b += mine[i] * mine[i]; ab += double(ref[i]) * mine[i]; peak = std::max(peak, std::fabs(d)); }
    double corr = ab / std::sqrt(a * b + 1e-30), db = 10 * std::log10((e + 1e-30) / (a + 1e-30));
    printf("%-7s match %8.4f%%   leftover difference %6.1f dB   largest gap %.5f\n", n.c_str(), corr * 100, db, peak);
    if (corr < .995) bad++;
  }
  return bad;
}

/* ---------- Scene for the live window ---------- */
struct Live { double x = VW / 2, y = VH / 2, t = 0; std::string pad = "none"; };
static void drawScene(Canvas &c, const Live &l) {
  auto sky = Canvas::createLinearGradient(0, 0, 0, VH); sky->addColorStop(0, "#1a0d14"); sky->addColorStop(.6, "#5a2418"); sky->addColorStop(1, "#d8602a");
  c.setGlobalCompositeOperation("source-over"); c.setGlobalAlpha(1); c.setFillStyle(sky); c.fillRect(0, 0, VW, VH);
  c.setFillStyle("#0b0706"); c.beginPath(); c.moveTo(0, VH); for (int i = 0; i <= 12; i++) c.lineTo(i * VW / 12.0, VH - 40 - 30 * std::fabs(std::sin(i * 1.7))); c.lineTo(VW, VH); c.closePath(); c.fill();
  auto glow = Canvas::createRadialGradient(l.x, l.y, 0, l.x, l.y, 40); glow->addColorStop(0, "rgba(255,170,80,.9)"); glow->addColorStop(1, "rgba(255,120,40,0)");
  c.setGlobalCompositeOperation("lighter"); c.setFillStyle(glow); c.fillRect(l.x - 40, l.y - 40, 80, 80); c.setGlobalCompositeOperation("source-over");
  c.setFillStyle("#f2e2c4"); c.fillRect(std::round(l.x) - 3, std::round(l.y) - 6, 6, 12);
  c.setFont("italic 15px \"IM Fell English\""); c.setTextAlign("center"); c.setTextBaseline("alphabetic"); c.fillText("Somebody opened the gate.", VW / 2, 44);
  c.setFont("8px Silkscreen, ui-monospace, monospace"); c.setFillStyle("#e3c8a0"); c.fillText("Controller: " + l.pad, VW / 2, 60); c.fillText("Move, A or Space: jump sound, X or J: heavy sound, Esc: quit", VW / 2, 272);
}

/* ---------- Controller self-test, using SDL's pretend controller ---------- */
static int padTest() {
  if (!SDL_Init(SDL_INIT_GAMEPAD)) { printf("SDL gamepad start failed: %s\n", SDL_GetError()); return 1; }
  int real = 0; SDL_JoystickID *ids = SDL_GetGamepads(&real); printf("real controllers plugged in: %d\n", real);
  for (int i = 0; i < real; i++) printf("   %s\n", SDL_GetGamepadNameForID(ids[i]));
  SDL_free(ids);
  SDL_VirtualJoystickDesc d; SDL_INIT_INTERFACE(&d); d.type = SDL_JOYSTICK_TYPE_GAMEPAD; d.naxes = SDL_GAMEPAD_AXIS_COUNT; d.nbuttons = SDL_GAMEPAD_BUTTON_COUNT; d.name = "Wallfall test pad";
  d.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1; d.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
  SDL_JoystickID id = SDL_AttachVirtualJoystick(&d); if (!id) { printf("virtual pad failed: %s\n", SDL_GetError()); return 1; }
  SDL_UpdateGamepads(); SDL_Gamepad *g = SDL_OpenGamepad(id); SDL_Joystick *j = SDL_OpenJoystick(id); if (!g || !j) { printf("open failed: %s\n", SDL_GetError()); return 1; }
  int bad = 0; auto check = [&](const char *n, bool ok) { printf("%s %s\n", ok ? "pass" : "FAIL", n); if (!ok) bad++; };
  SDL_SetJoystickVirtualButton(j, SDL_GAMEPAD_BUTTON_SOUTH, true); SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_LEFTX, 32767); SDL_UpdateGamepads();
  check("A press is read", SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_SOUTH)); check("stick right is read", SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX) > 30000);
  SDL_SetJoystickVirtualButton(j, SDL_GAMEPAD_BUTTON_SOUTH, false); SDL_UpdateGamepads(); check("A release is read", !SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_SOUTH));
  SDL_CloseGamepad(g); SDL_DetachVirtualJoystick(id); SDL_Quit(); return bad;
}

/* ---------- Live window ---------- */
static int live(const char *base, int shotAfter, const char *shotPath) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) { printf("SDL start failed: %s\n", SDL_GetError()); return 1; }
  SDL_Window *win; SDL_Renderer *ren;
  if (!SDL_CreateWindowAndRenderer("Wallfall (C++ proof)", VW * 2, VH * 2, SDL_WINDOW_RESIZABLE, &win, &ren)) { printf("window failed: %s\n", SDL_GetError()); return 1; }
  SDL_SetRenderVSync(ren, 1); SDL_SetRenderLogicalPresentation(ren, VW, VH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, VW, VH); SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
  SDL_AudioSpec spec{SDL_AUDIO_F32, 1, RATE}; SDL_AudioStream *snd = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  if (snd) SDL_ResumeAudioStreamDevice(snd); else printf("sound failed: %s\n", SDL_GetError());
  auto play = [&](const char *n) { if (!snd) return; auto s = renderSfx(n, RATE, .5); SDL_PutAudioStreamData(snd, s.data(), int(s.size() * sizeof(float))); };
  Canvas cv(VW, VH); Live l; SDL_Gamepad *pad = nullptr; bool run = true; double acc = 0; Uint64 prev = SDL_GetPerformanceCounter(); int frames = 0; long ticks = 0; play("jump");
  while (run) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) run = false;
      else if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) { if (e.key.key == SDLK_ESCAPE) run = false; if (e.key.key == SDLK_SPACE) play("jump"); if (e.key.key == SDLK_J) play("heavy"); }
      else if (e.type == SDL_EVENT_GAMEPAD_ADDED && !pad) { pad = SDL_OpenGamepad(e.gdevice.which); if (pad) l.pad = SDL_GetGamepadName(pad); }
      else if (e.type == SDL_EVENT_GAMEPAD_REMOVED && pad && SDL_GetGamepadID(pad) == e.gdevice.which) { SDL_CloseGamepad(pad); pad = nullptr; l.pad = "none"; }
      else if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) { if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) play("jump"); if (e.gbutton.button == SDL_GAMEPAD_BUTTON_WEST) play("heavy"); }
    }
    Uint64 now = SDL_GetPerformanceCounter(); acc += std::min(100.0, (now - prev) * 1000.0 / SDL_GetPerformanceFrequency()); prev = now;
    while (acc >= 1000.0 / 60) {
      const bool *k = SDL_GetKeyboardState(nullptr); double dx = k[SDL_SCANCODE_RIGHT] - k[SDL_SCANCODE_LEFT] + k[SDL_SCANCODE_D] - k[SDL_SCANCODE_A], dy = k[SDL_SCANCODE_DOWN] - k[SDL_SCANCODE_UP] + k[SDL_SCANCODE_S] - k[SDL_SCANCODE_W];
      if (pad) { double ax = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0, ay = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0; if (std::fabs(ax) > .25) dx += ax; if (std::fabs(ay) > .25) dy += ay; }
      l.x = std::clamp(l.x + dx * 2, 0.0, double(VW)); l.y = std::clamp(l.y + dy * 2, 0.0, double(VH)); l.t += 1; ticks++; acc -= 1000.0 / 60;
    }
    drawScene(cv, l); SDL_UpdateTexture(tex, nullptr, cv.pixels(), int(cv.rowBytes())); SDL_RenderClear(ren); SDL_RenderTexture(ren, tex, nullptr, nullptr);
    if (shotPath && ++frames == shotAfter) {
      SDL_Surface *s = SDL_RenderReadPixels(ren, nullptr); SDL_Surface *r = s ? SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32) : nullptr;
      if (r) savePng(shotPath, r->pixels, r->w, r->h, r->pitch); printf("window drew %d frames, %ld fixed updates, sound %s, controller: %s\n", frames, ticks, snd ? "open" : "failed", l.pad.c_str());
      if (snd) printf("sound device: %s, samples waiting: %d\n", SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(snd)), SDL_GetAudioStreamQueued(snd) / 4);
      SDL_DestroySurface(r); SDL_DestroySurface(s); run = false;
    }
    SDL_RenderPresent(ren);
  }
  if (snd) SDL_DestroyAudioStream(snd);
  if (pad) SDL_CloseGamepad(pad);
  SDL_DestroyTexture(tex); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit(); return 0;
}

int main(int argc, char **argv) {
  std::string base = SDL_GetBasePath() ? SDL_GetBasePath() : "./";
  if (!Canvas::loadFonts(base + "assets/fonts")) { printf("fonts failed to load\n"); return 1; }
  std::string m = argc > 1 ? argv[1] : "";
  if (m == "--replay" && argc >= 4) return replay(argv[2], argv[3]);
  if (m == "--sound" && argc >= 3) return compareSound(argv[2]);
  if (m == "--pad") return padTest();
  if (m == "--shot" && argc >= 3) return live(base.c_str(), 180, argv[2]);
  return live(base.c_str(), 0, nullptr);
}
