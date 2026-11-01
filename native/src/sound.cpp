#include "sound.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>
#include <map>

namespace wf {
static const double PI = 3.141592653589793;
static const int SIZE = 4096, BANDS = 3, RANGES = 36;

/* ---------- Band-limited tables, built the way Chrome builds them ---------- */
static void inverseFFT(std::vector<std::complex<double>> &a) {
  int n = int(a.size());
  for (int i = 1, j = 0; i < n; i++) { int b = n >> 1; for (; j & b; b >>= 1) j ^= b; j ^= b; if (i < j) std::swap(a[i], a[j]); }
  for (int len = 2; len <= n; len <<= 1) {
    std::complex<double> w = std::polar(1.0, 2 * PI / len);
    for (int i = 0; i < n; i += len) { std::complex<double> u = 1; for (int k = 0; k < len / 2; k++) { auto x = a[i + k], y = a[i + k + len / 2] * u; a[i + k] = x + y; a[i + k + len / 2] = x - y; u *= w; } }
  }
}
static double partial(Wave t, int n) {
  double odd = n & 1 ? 1 : 0;
  switch (t) {
    case Wave::Sine: return n == 1 ? 1 : 0;
    case Wave::Square: return 2.0 / (n * PI) * (1 - std::pow(-1.0, n));
    case Wave::Sawtooth: return 2.0 / (n * PI) * (n & 1 ? 1 : -1);
    default: return odd * 8 / (PI * PI * n * n) * ((n - 1) % 4 == 0 ? 1 : -1);
  }
}
struct Table { std::vector<float> s; };
static std::map<std::pair<int, int>, Table> tables;
static std::map<int, double> norms;
static std::vector<float> buildTable(Wave t, int range, double scale) {
  int maxP = SIZE / 2, parts = int(maxP * std::pow(2.0, -range * 400.0 / 1200.0));
  std::vector<std::complex<double>> f(SIZE);
  for (int n = 1; n < maxP && n <= parts; n++) { double b = partial(t, n); f[n] = {0, -b / 2}; f[SIZE - n] = {0, b / 2}; }
  inverseFFT(f);
  std::vector<float> s(SIZE + 1); for (int i = 0; i < SIZE; i++) s[i] = float(f[i].real() * scale); s[SIZE] = s[0];
  return s;
}
static const std::vector<float> &table(Wave t, int range) {
  auto key = std::make_pair(int(t), range); auto it = tables.find(key); if (it != tables.end()) return it->second.s;
  if (!norms.count(int(t))) { auto s = buildTable(t, 0, 1); double m = 0; for (float v : s) m = std::max(m, double(std::fabs(v))); norms[int(t)] = 1 / m; }
  return tables[key].s = buildTable(t, range, norms[int(t)]);
}

/* ---------- tone(): same maths as the game ---------- */
void addTone(std::vector<float> &out, int rate, double f, double dur, Wave type, double vol, double to, double delay) {
  size_t a = size_t(std::ceil(delay * rate)), n = size_t(std::round((dur + .02) * rate)), end = std::min(out.size(), a + n);
  double lowest = rate / 2.0 / (SIZE / 2), ph = 0, inc = double(SIZE) / rate;
  for (size_t i = a; i < end; i++) {
    double t = (i - a) / double(rate), k = std::min(1.0, t / dur), fr = to ? f * std::pow(to / f, k) : f, g = vol * std::pow(.0001 / vol, k);
    double pitch = std::clamp(1 + std::log2(fr / lowest) * 1200 / 400, 0.0, RANGES - 1.0); int r1 = int(pitch), r2 = std::min(r1 + 1, RANGES - 1); double mix = pitch - r1;
    auto &lo = table(type, r1); auto &hi = table(type, r2); int p = int(ph); double u = ph - p;
    double s1 = lo[p] + (lo[p + 1] - lo[p]) * u, s2 = hi[p] + (hi[p + 1] - hi[p]) * u;
    out[i] += float(g * ((1 - mix) * s1 + mix * s2));
    ph += fr * inc; while (ph >= SIZE) ph -= SIZE;
  }
}

/* ---------- Sound effects, copied from the game's sfx table ---------- */
static const std::map<std::string, std::function<void(std::vector<float> &, int)>> SFX = {
  {"jump", [](auto &o, int r) { addTone(o, r, 300, .1, Wave::Square, .03, 480); }},
  {"air", [](auto &o, int r) { addTone(o, r, 480, .12, Wave::Triangle, .04, 820); }},
  {"dash", [](auto &o, int r) { addTone(o, r, 200, .15, Wave::Sawtooth, .03, 90); }},
  {"swing", [](auto &o, int r) { addTone(o, r, 900, .06, Wave::Triangle, .025, 400); }},
  {"heavy", [](auto &o, int r) { addTone(o, r, 160, .28, Wave::Sawtooth, .05, 55); addTone(o, r, 70, .3, Wave::Sine, .08, 40); }},
  {"fire", [](auto &o, int r) { addTone(o, r, 260, .3, Wave::Sawtooth, .035, 110); addTone(o, r, 520, .15, Wave::Triangle, .02, 300); }},
  {"shoot", [](auto &o, int r) { addTone(o, r, 1100, .05, Wave::Square, .02, 500); addTone(o, r, 200, .08, Wave::Triangle, .03, 120); }},
  {"hit", [](auto &o, int r) { addTone(o, r, 130, .1, Wave::Square, .05, 60); }},
};
const std::vector<std::string> &sfxNames() { static std::vector<std::string> v = [] { std::vector<std::string> n; for (auto &[k, f] : SFX) n.push_back(k); return n; }(); return v; }
std::vector<float> renderSfx(const std::string &name, int rate, double seconds) { std::vector<float> o(size_t(rate * seconds)); auto i = SFX.find(name); if (i != SFX.end()) i->second(o, rate); return o; }

}
