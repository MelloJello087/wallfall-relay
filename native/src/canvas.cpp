#include "canvas.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <unordered_map>
#include <hb.h>
#include "include/core/SkColorSpace.h"
#include "include/core/SkData.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkRect.h"
#include "include/core/SkStream.h"
#include "include/core/SkTypeface.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkGradientShader.h"
#include "include/effects/SkImageFilters.h"
#include "include/ports/SkTypeface_win.h"

namespace wf {
static const double TAU = 6.283185307179586;
static float deg(double r) { return float(r * 360 / TAU); }

/* ---------- Colors ---------- */
SkColor4f parseColor(const std::string &in) {
  std::string s; for (char c : in) if (c != ' ') s += char(tolower(c));
  auto hx = [&](size_t i, size_t n) { return float(std::stoi(s.substr(i, n), nullptr, 16)) / (n == 1 ? 15 : 255); };
  if (s[0] == '#') {
    if (s.size() == 4 || s.size() == 5) return {hx(1, 1), hx(2, 1), hx(3, 1), s.size() == 5 ? hx(4, 1) : 1};
    return {hx(1, 2), hx(3, 2), hx(5, 2), s.size() == 9 ? hx(7, 2) : 1};
  }
  if (s.rfind("rgb", 0) == 0) { float v[4] = {0, 0, 0, 1}; int n = 0; const char *p = s.c_str() + s.find('(') + 1; while (*p && *p != ')' && n < 4) { char *e; v[n++] = strtof(p, &e); p = *e == ',' ? e + 1 : e; } return {v[0] / 255, v[1] / 255, v[2] / 255, v[3]}; }
  if (s == "transparent") return {0, 0, 0, 0};
  if (s == "white") return {1, 1, 1, 1};
  return {0, 0, 0, 1};
}

/* ---------- Gradients ---------- */
void Gradient::addColorStop(double o, const std::string &c) {
  size_t i = at.size(); while (i > 0 && at[i - 1] > o) i--;
  at.insert(at.begin() + i, float(o)); col.insert(col.begin() + i, parseColor(c)); built = nullptr;
}
sk_sp<SkShader> Gradient::shader() {
  if (built || at.empty()) return built;
  std::vector<float> p = at; std::vector<SkColor4f> c = col;
  if (p.front() > 0) { p.insert(p.begin(), 0); c.insert(c.begin(), c.front()); }
  if (p.back() < 1) { p.push_back(1); c.push_back(c.back()); }
  if (radial) built = SkGradientShader::MakeTwoPointConical({x0, y0}, r0, {x1, y1}, r1, c.data(), nullptr, p.data(), int(p.size()), SkTileMode::kClamp);
  else { SkPoint e[2] = {{x0, y0}, {x1, y1}}; built = SkGradientShader::MakeLinear(e, c.data(), nullptr, p.data(), int(p.size()), SkTileMode::kClamp); }
  return built;
}

/* ---------- Fonts ---------- */
static std::map<std::string, sk_sp<SkTypeface>> faces;
static sk_sp<SkFontMgr> fontMgr;
bool Canvas::loadFonts(const std::string &dir) {
  fontMgr = SkFontMgr_New_DirectWrite(); if (!fontMgr) return false;
  const std::pair<const char *, const char *> list[] = {{"silkscreen", "Silkscreen-Regular.ttf"}, {"fell", "IMFeENrm28P.ttf"}, {"fell-italic", "IMFeENit28P.ttf"}, {"fellsc", "IMFeENsc28P.ttf"}, {"caveat", "Caveat.ttf"}};
  bool ok = true;
  for (auto &[k, f] : list) { auto t = fontMgr->makeFromFile((dir + "/" + f).c_str()); ok = ok && t; faces[k] = t; }
  return ok;
}
static SkFont fontFrom(const std::string &spec) {
  std::string s = spec; bool italic = s.find("italic") != std::string::npos;
  size_t px = s.find("px"), b = px; while (b > 0 && (isdigit((unsigned char)s[b - 1]) || s[b - 1] == '.')) b--;
  float size = px == std::string::npos ? 10 : strtof(s.c_str() + b, nullptr);
  std::string fam = px == std::string::npos ? "" : s.substr(px + 2);
  const char *key = fam.find("IM Fell English SC") != std::string::npos ? "fellsc" : fam.find("IM Fell English") != std::string::npos ? (italic ? "fell-italic" : "fell") : fam.find("Caveat") != std::string::npos ? "caveat" : "silkscreen";
  SkFont f(faces[key], size); f.setEdging(SkFont::Edging::kAntiAlias); f.setHinting(SkFontHinting::kNone); f.setSubpixel(true); f.setLinearMetrics(true);
  return f;
}

/* ---------- Text shaping: HarfBuzz, like Chrome, with fallback for missing characters ---------- */
struct HbFace { hb_font_t *font = nullptr; float upem = 1000; };
static std::map<SkTypefaceID, HbFace> hbFaces;
static HbFace &hbFace(const sk_sp<SkTypeface> &t) {
  HbFace &h = hbFaces[t->uniqueID()]; if (h.font) return h;
  int idx = 0; auto st = t->openStream(&idx); size_t n = st ? st->getLength() : 0; char *mem = (char *)malloc(n ? n : 1); if (st) st->read(mem, n);
  hb_blob_t *blob = hb_blob_create(mem, unsigned(n), HB_MEMORY_MODE_READONLY, mem, free); hb_face_t *face = hb_face_create(blob, idx); hb_blob_destroy(blob);
  h.upem = float(hb_face_get_upem(face)); h.font = hb_font_create(face); hb_font_set_scale(h.font, int(h.upem), int(h.upem)); hb_face_destroy(face);
  return h;
}
static size_t utf8Next(const std::string &s, size_t i, SkUnichar &c) {
  unsigned char b = s[i]; int n = b < 0x80 ? 1 : b < 0xE0 ? 2 : b < 0xF0 ? 3 : 4; c = n == 1 ? b : b & (0xFF >> (n + 1));
  for (int k = 1; k < n && i + k < s.size(); k++) c = (c << 6) | (s[i + k] & 0x3F);
  return i + n;
}
struct ShapedRun { SkFont font; std::vector<SkGlyphID> glyphs; std::vector<SkPoint> pos; };
struct Shaped { std::vector<ShapedRun> runs; float width = 0; };
static std::unordered_map<std::string, Shaped> shapedCache;
static const Shaped &shapeText(const SkFont &f, const std::string &t) {
  std::string key = std::to_string(f.getTypeface()->uniqueID()) + "/" + std::to_string(f.getSize()) + "/" + t; auto hit = shapedCache.find(key); if (hit != shapedCache.end()) return hit->second;
  if (shapedCache.size() > 4000) shapedCache.clear();
  struct Run { sk_sp<SkTypeface> face; size_t a, b; }; std::vector<Run> runs; sk_sp<SkTypeface> main = sk_ref_sp(f.getTypeface());
  for (size_t i = 0; i < t.size();) {
    SkUnichar c; size_t next = utf8Next(t, i, c); sk_sp<SkTypeface> use = main;
    if (!main->unicharToGlyph(c)) {
      static sk_sp<SkTypeface> mono = fontMgr->matchFamily("Consolas")->matchStyle(SkFontStyle::Normal());
      auto alt = main == faces["silkscreen"] && mono && mono->unicharToGlyph(c) ? mono : fontMgr->matchFamilyStyleCharacter(nullptr, main->fontStyle(), nullptr, 0, c); if (alt) use = alt;
    }
    if (!runs.empty() && runs.back().face->uniqueID() == use->uniqueID()) runs.back().b = next; else runs.push_back({use, i, next});
    i = next;
  }
  Shaped out; float x = 0;
  for (auto &r : runs) {
    HbFace &h = hbFace(r.face); hb_buffer_t *buf = hb_buffer_create(); hb_buffer_add_utf8(buf, t.data() + r.a, int(r.b - r.a), 0, -1); hb_buffer_guess_segment_properties(buf); hb_shape(h.font, buf, nullptr, 0);
    unsigned n = 0; auto *gi = hb_buffer_get_glyph_infos(buf, &n); auto *gp = hb_buffer_get_glyph_positions(buf, &n); float k = f.getSize() / h.upem;
    ShapedRun sr; sr.font = f; sr.font.setTypeface(r.face);
    for (unsigned i = 0; i < n; i++) { sr.glyphs.push_back(SkGlyphID(gi[i].codepoint)); sr.pos.push_back({x + gp[i].x_offset * k, -gp[i].y_offset * k}); x += gp[i].x_advance * k; }
    out.runs.push_back(std::move(sr)); hb_buffer_destroy(buf);
  }
  out.width = x; return shapedCache[key] = std::move(out);
}

static std::unordered_map<uint64_t, std::optional<SkPath>> glyphPaths;
static void drawGlyphs(SkCanvas *c, const Shaped &sh, float x, float y, const SkPaint &q) {
  for (auto &r : sh.runs) for (size_t i = 0; i < r.glyphs.size(); i++) {
    uint64_t key = uint64_t(r.font.getTypeface()->uniqueID()) << 32 | uint64_t(r.glyphs[i]) << 16 | uint64_t(r.font.getSize() * 16);
    auto it = glyphPaths.find(key); if (it == glyphPaths.end()) it = glyphPaths.emplace(key, r.font.getPath(r.glyphs[i])).first;
    if (!it->second) continue;
    c->save(); c->translate(std::floor((x + r.pos[i].fX) * 4 + .5f) / 4, std::round(y + r.pos[i].fY)); c->drawPath(*it->second, q); c->restore();
  }
}

static float emAscent(const SkFont &f) {
  uint8_t b[4] = {0, 0, 0, 0}; f.getTypeface()->getTableData(SkSetFourByteTag('O', 'S', '/', '2'), 68, 4, b);
  float up = float(int16_t(b[0] << 8 | b[1])), dn = -float(int16_t(b[2] << 8 | b[3]));
  return up + dn > 0 ? std::round(f.getSize() * up / (up + dn) * 64) / 64 : f.getSize() * .8f;
}

/* ---------- Canvas ---------- */
Canvas::Canvas(int w, int h) : w_(w), h_(h) {
  surf_ = SkSurfaces::Raster(SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB()));
  c_ = surf_->getCanvas(); c_->clear(SK_ColorTRANSPARENT); st_.font = fontFrom("10px sans-serif");
}
sk_sp<SkImage> Canvas::image() { return surf_->makeImageSnapshot(); }
const void *Canvas::pixels() { SkPixmap p; return surf_->peekPixels(&p) ? p.addr() : nullptr; }
size_t Canvas::rowBytes() const { SkPixmap p; return surf_->peekPixels(&p) ? p.rowBytes() : 0; }
void Canvas::loadImage(sk_sp<SkImage> img) { SkPaint p; p.setBlendMode(SkBlendMode::kSrc); c_->drawImage(img, 0, 0, SkSamplingOptions(), &p); }

void Canvas::save() { stack_.push_back(st_); c_->save(); }
void Canvas::restore() { if (stack_.empty()) return; SkMatrix was = c_->getLocalToDeviceAs3x3(); st_ = std::move(stack_.back()); stack_.pop_back(); c_->restore(); SkMatrix now = c_->getLocalToDeviceAs3x3(), inv; if (now.invert(&inv)) { path_.transform(SkMatrix::Concat(inv, was)); } }
void Canvas::shiftPath(const SkMatrix &change) { SkMatrix inv; if (change.invert(&inv)) path_.transform(inv); }
void Canvas::translate(double x, double y) { c_->translate(float(x), float(y)); shiftPath(SkMatrix::Translate(float(x), float(y))); }
void Canvas::scale(double x, double y) { c_->scale(float(x), float(y)); shiftPath(SkMatrix::Scale(float(x), float(y))); }
void Canvas::rotate(double a) { c_->rotate(deg(a)); shiftPath(SkMatrix::RotateDeg(deg(a))); }
void Canvas::setTransform(double a, double b, double c, double d, double e, double f) {
  SkMatrix was = c_->getLocalToDeviceAs3x3(), m = SkMatrix::MakeAll(float(a), float(c), float(e), float(b), float(d), float(f), 0, 0, 1), inv;
  c_->setMatrix(m); if (m.invert(&inv)) path_.transform(SkMatrix::Concat(inv, was));
}

void Canvas::setFillStyle(const std::string &c) { st_.fill = {parseColor(c), nullptr, nullptr}; }
void Canvas::setFillStyle(GradientRef g) { st_.fill = {{0, 0, 0, 1}, g, nullptr}; }
void Canvas::setFillPattern(sk_sp<SkShader> s) { st_.fill = {{0, 0, 0, 1}, nullptr, s}; }
void Canvas::setStrokeStyle(const std::string &c) { st_.stroke = {parseColor(c), nullptr, nullptr}; }
void Canvas::setStrokeStyle(GradientRef g) { st_.stroke = {{0, 0, 0, 1}, g, nullptr}; }
void Canvas::setStrokePattern(sk_sp<SkShader> s) { st_.stroke = {{0, 0, 0, 1}, nullptr, s}; }
void Canvas::setGlobalAlpha(double a) { if (a >= 0 && a <= 1) st_.alpha = float(a); }
void Canvas::setGlobalCompositeOperation(const std::string &op) {
  static const std::map<std::string, SkBlendMode> M = {{"source-over", SkBlendMode::kSrcOver}, {"lighter", SkBlendMode::kPlus}, {"multiply", SkBlendMode::kMultiply}, {"soft-light", SkBlendMode::kSoftLight}, {"color", SkBlendMode::kColor}, {"destination-in", SkBlendMode::kDstIn}, {"destination-out", SkBlendMode::kDstOut}, {"screen", SkBlendMode::kScreen}, {"overlay", SkBlendMode::kOverlay}};
  auto i = M.find(op); if (i != M.end()) st_.blend = i->second;
}
void Canvas::setLineWidth(double w) { if (w > 0) st_.lineWidth = float(w); }
void Canvas::setLineCap(const std::string &c) { st_.cap = c == "round" ? SkPaint::kRound_Cap : c == "square" ? SkPaint::kSquare_Cap : SkPaint::kButt_Cap; }
void Canvas::setLineJoin(const std::string &j) { st_.join = j == "round" ? SkPaint::kRound_Join : j == "bevel" ? SkPaint::kBevel_Join : SkPaint::kMiter_Join; }
void Canvas::setMiterLimit(double m) { st_.miter = float(m); }
void Canvas::setLineDash(std::vector<float> d) { if (d.size() % 2) d.insert(d.end(), d.begin(), d.end()); st_.dash = std::move(d); }
void Canvas::setLineDashOffset(double o) { st_.dashOff = float(o); }
void Canvas::setFont(const std::string &f) { st_.font = fontFrom(f); }
void Canvas::setTextAlign(const std::string &a) { st_.textCenter = a == "center"; st_.textRight = a == "right" || a == "end"; }
void Canvas::setTextBaseline(const std::string &b) { st_.baseline = b == "top" ? 1 : b == "middle" ? 2 : b == "bottom" ? 3 : b == "hanging" ? 4 : b == "ideographic" ? 3 : 0; }
void Canvas::setShadowColor(const std::string &c) { st_.shadow = parseColor(c); }
void Canvas::setShadowBlur(double b) { st_.blur = float(b); }
void Canvas::setShadowOffsetX(double x) { st_.sx = float(x); }
void Canvas::setShadowOffsetY(double y) { st_.sy = float(y); }

GradientRef Canvas::createLinearGradient(double x0, double y0, double x1, double y1) { auto g = std::make_shared<Gradient>(); g->x0 = float(x0); g->y0 = float(y0); g->x1 = float(x1); g->y1 = float(y1); return g; }
GradientRef Canvas::createRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1) { auto g = createLinearGradient(x0, y0, x1, y1); g->radial = true; g->r0 = float(r0); g->r1 = float(r1); return g; }
sk_sp<SkShader> Canvas::createPattern(sk_sp<SkImage> img) { return img->makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat, SkSamplingOptions(SkFilterMode::kLinear)); }

SkPaint Canvas::paintFor(const Style &s, bool stroking) {
  SkPaint p; p.setAntiAlias(true); p.setBlendMode(st_.blend); SkColor4f c = s.color;
  sk_sp<SkShader> sh = s.grad ? s.grad->shader() : s.pat;
  if (sh) { c = {0, 0, 0, 1}; p.setShader(sh); if (s.grad) p.setDither(true); }
  c.fA *= st_.alpha; p.setColor4f(c);
  if (stroking) {
    p.setStyle(SkPaint::kStroke_Style); p.setStrokeWidth(st_.lineWidth); p.setStrokeCap(st_.cap); p.setStrokeJoin(st_.join); p.setStrokeMiter(st_.miter);
    if (!st_.dash.empty()) p.setPathEffect(SkDashPathEffect::Make(st_.dash, st_.dashOff));
  }
  if (st_.shadow.fA > 0 && (st_.blur > 0 || st_.sx || st_.sy)) {
    float sc = c_->getLocalToDeviceAs3x3().getScaleX(); sc = sc ? sc : 1; float sig = st_.blur / 2 / sc; SkColor4f sh2 = st_.shadow; sh2.fA *= st_.alpha;
    p.setImageFilter(SkImageFilters::DropShadow(st_.sx / sc, st_.sy / sc, sig, sig, sh2.toSkColor(), nullptr));
  }
  return p;
}
template <class F> void Canvas::paintWith(const SkPaint &p, F draw) {
  SkBlendMode m = st_.blend;
  if (m == SkBlendMode::kDstIn || m == SkBlendMode::kSrcIn || m == SkBlendMode::kSrcOut || m == SkBlendMode::kDstATop) {
    SkPaint layer; layer.setBlendMode(m); c_->saveLayer(nullptr, &layer); SkPaint q = p; q.setBlendMode(SkBlendMode::kSrcOver); draw(q); c_->restore();
  } else draw(p);
}

void Canvas::beginPath() { path_.reset(); hasPoint_ = false; }
void Canvas::closePath() { if (hasPoint_) path_.close(); }
void Canvas::moveTo(double x, double y) { path_.moveTo(float(x), float(y)); hasPoint_ = true; }
void Canvas::lineTo(double x, double y) { if (hasPoint_) path_.lineTo(float(x), float(y)); else moveTo(x, y); }
void Canvas::quadraticCurveTo(double cx, double cy, double x, double y) { if (!hasPoint_) moveTo(cx, cy); path_.quadTo(float(cx), float(cy), float(x), float(y)); }
void Canvas::bezierCurveTo(double a, double b, double c, double d, double x, double y) { if (!hasPoint_) moveTo(a, b); path_.cubicTo(float(a), float(b), float(c), float(d), float(x), float(y)); }
void Canvas::rect(double x, double y, double w, double h) { path_.moveTo(float(x), float(y)); path_.lineTo(float(x + w), float(y)); path_.lineTo(float(x + w), float(y + h)); path_.lineTo(float(x), float(y + h)); path_.close(); path_.moveTo(float(x), float(y)); hasPoint_ = true; }
void Canvas::arc(double x, double y, double r, double a0, double a1, bool ccw) { ellipse(x, y, r, r, 0, a0, a1, ccw); }
void Canvas::ellipse(double x, double y, double rx, double ry, double rot, double a0, double a1, bool ccw) {
  double s = std::fmod(a0, TAU); if (s < 0) s += TAU; double e = a1 + (s - a0);
  if (!ccw && e - s >= TAU) e = s + TAU;
  else if (ccw && s - e >= TAU) e = s - TAU;
  else if (!ccw && s > e) e = s + (TAU - std::fmod(s - e, TAU));
  else if (ccw && s < e) e = s - (TAU - std::fmod(e - s, TAU));
  SkRect oval = SkRect::MakeLTRB(float(x - rx), float(y - ry), float(x + rx), float(y + ry));
  float st = deg(s), sw = deg(e - s); SkPath part;
  if (std::fabs(std::fabs(sw) - 360) < 1e-4f) { float h = sw > 0 ? 180.f : -180.f; part.arcTo(oval, st, h, true); part.arcTo(oval, st + h, h, false); }
  else part.arcTo(oval, st, sw, true);
  if (rot) part.transform(SkMatrix::RotateDeg(deg(rot), {float(x), float(y)}));
  if (part.countPoints()) { SkPoint first = part.getPoint(0); if (hasPoint_) path_.lineTo(first); else path_.moveTo(first); }
  path_.addPath(part, SkPath::kExtend_AddPathMode); hasPoint_ = true;
}
void Canvas::fill(bool evenOdd) { SkPath p = path_; p.setFillType(evenOdd ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding); paintWith(paintFor(st_.fill, false), [&](const SkPaint &q) { c_->drawPath(p, q); }); }
void Canvas::stroke() { paintWith(paintFor(st_.stroke, true), [&](const SkPaint &q) { c_->drawPath(path_, q); }); }
void Canvas::clip(bool evenOdd) { SkPath p = path_; p.setFillType(evenOdd ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding); c_->clipPath(p, true); }
void Canvas::fillRect(double x, double y, double w, double h) { SkRect r = SkRect::MakeXYWH(float(x), float(y), float(w), float(h)); r.sort(); paintWith(paintFor(st_.fill, false), [&](const SkPaint &q) { c_->drawRect(r, q); }); }
void Canvas::strokeRect(double x, double y, double w, double h) { SkRect r = SkRect::MakeXYWH(float(x), float(y), float(w), float(h)); r.sort(); paintWith(paintFor(st_.stroke, true), [&](const SkPaint &q) { c_->drawRect(r, q); }); }
void Canvas::clearRect(double x, double y, double w, double h) { SkPaint p; p.setAntiAlias(true); p.setBlendMode(SkBlendMode::kClear); c_->drawRect(SkRect::MakeXYWH(float(x), float(y), float(w), float(h)), p); }

double Canvas::measureText(const std::string &t) { return t.empty() ? 0 : shapeText(st_.font, t).width; }
void Canvas::textAt(const std::string &t, double x, double y, bool stroking) {
  SkFontMetrics m; st_.font.getMetrics(&m); float asc = std::round(-m.fAscent), w = float(measureText(t)), em = emAscent(st_.font), size = st_.font.getSize();
  float ox = st_.textCenter ? -w / 2 : st_.textRight ? -w : 0;
  float oy = st_.baseline == 1 ? em : st_.baseline == 2 ? em - size / 2 : st_.baseline == 3 ? em - size : st_.baseline == 4 ? asc * 8 / 10 : 0;
  paintWith(paintFor(stroking ? st_.stroke : st_.fill, stroking), [&](const SkPaint &q) { drawGlyphs(c_, shapeText(st_.font, t), float(x) + ox, float(y) + oy, q); });
}
void Canvas::fillText(const std::string &t, double x, double y) { textAt(t, x, y, false); }
void Canvas::strokeText(const std::string &t, double x, double y) { textAt(t, x, y, true); }

void Canvas::drawImage(sk_sp<SkImage> img, double dx, double dy) { drawImage(img, 0, 0, img->width(), img->height(), dx, dy, img->width(), img->height()); }
void Canvas::drawImage(sk_sp<SkImage> img, double dx, double dy, double dw, double dh) { drawImage(img, 0, 0, img->width(), img->height(), dx, dy, dw, dh); }
void Canvas::drawImage(sk_sp<SkImage> img, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh) {
  SkPaint base = paintFor(Style{}, false); base.setShader(nullptr); base.setColor4f({1, 1, 1, st_.alpha});
  SkRect src = SkRect::MakeXYWH(float(sx), float(sy), float(sw), float(sh)), dst = SkRect::MakeXYWH(float(dx), float(dy), float(dw), float(dh));
  paintWith(base, [&](const SkPaint &q) { c_->drawImageRect(img, src, dst, SkSamplingOptions(SkFilterMode::kLinear), &q, SkCanvas::kFast_SrcRectConstraint); });
}

}
