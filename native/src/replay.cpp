#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include "canvas.h"
#include "include/codec/SkCodec.h"
#include "include/codec/SkPngDecoder.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkData.h"
#include "include/core/SkStream.h"
#include "include/encode/SkPngEncoder.h"

namespace wf {

/* ---------- Files ---------- */
sk_sp<SkImage> loadPng(const std::string &path) {
  auto data = SkData::MakeFromFileName(path.c_str()); if (!data) return nullptr;
  auto codec = SkPngDecoder::Decode(data, nullptr); if (!codec) return nullptr;
  SkBitmap bm; bm.allocPixels(SkImageInfo::Make(codec->dimensions(), kRGBA_8888_SkColorType, kPremul_SkAlphaType));
  if (codec->getPixels(bm.pixmap()) != SkCodec::kSuccess) return nullptr;
  bm.setImmutable(); return bm.asImage();
}
bool savePng(const std::string &path, const void *px, int w, int h, size_t rb) {
  SkPixmap pm(SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType), px, rb);
  SkFILEWStream f(path.c_str()); return f.isValid() && SkPngEncoder::Encode(&f, pm, {});
}

/* ---------- Replay: run one frame's recorded calls, then compare with Chrome's pixels ---------- */
static std::vector<std::string> split(const std::string &s) { std::vector<std::string> v; size_t a = 0, b; while ((b = s.find('\t', a)) != std::string::npos) { v.push_back(s.substr(a, b - a)); a = b + 1; } v.push_back(s.substr(a)); return v; }

struct Result { long total = 0, off1 = 0, off8 = 0, off32 = 0; double sum = 0, ms = 0; int worst = 0; std::map<std::string, int> unknown; };

static void replayFrame(const std::string &dir, const std::string &name, Canvas *&main, Result &res) {
  std::ifstream in(dir + "/" + name + ".txt"); std::string line; std::map<std::string, std::unique_ptr<Canvas>> cv; std::map<std::string, GradientRef> gr; std::map<std::string, sk_sp<SkShader>> pt;
  static std::vector<std::unique_ptr<Canvas>> keep;
  while (std::getline(in, line)) {
    auto a = split(line); if (a.size() < 2) continue;
    auto d = [&](size_t i) { return i < a.size() ? std::stod(a[i]) : 0.0; }; const std::string &id = a[0], &op = a[1]; size_t n = a.size() - 2;
    if (op == "canvas") { auto c = std::make_unique<Canvas>(int(d(2)), int(d(3))); if (auto img = loadPng(dir + "/" + a[4])) c->loadImage(img); cv[id] = std::move(c); continue; }
    auto began = std::chrono::steady_clock::now();
    struct Clock { decltype(began) b; double &ms; ~Clock() { ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - b).count(); } } clock{began, res.ms};
    if (op == "lgrad") { gr[a[2]] = Canvas::createLinearGradient(d(3), d(4), d(5), d(6)); continue; }
    if (op == "rgrad") { gr[a[2]] = Canvas::createRadialGradient(d(3), d(4), d(5), d(6), d(7), d(8)); continue; }
    if (op == "stop") { gr[a[2]]->addColorStop(d(3), a[4]); continue; }
    if (op == "pattern") { pt[a[2]] = Canvas::createPattern(cv[a[3]]->image()); continue; }
    Canvas *c = cv[id].get(); if (!c) continue;
    if (op == "fillStyle" || op == "strokeStyle") {
      bool f = op == "fillStyle"; const std::string &v = a[2];
      if (v[0] == '@') { if (f) c->setFillStyle(gr[v]); else c->setStrokeStyle(gr[v]); }
      else if (v[0] == '%') { if (f) c->setFillPattern(pt[v]); else c->setStrokePattern(pt[v]); }
      else { if (f) c->setFillStyle(v); else c->setStrokeStyle(v); }
    }
    else if (op == "globalAlpha") c->setGlobalAlpha(d(2));
    else if (op == "globalCompositeOperation") c->setGlobalCompositeOperation(a[2]);
    else if (op == "lineWidth") c->setLineWidth(d(2));
    else if (op == "lineCap") c->setLineCap(a[2]);
    else if (op == "lineJoin") c->setLineJoin(a[2]);
    else if (op == "miterLimit") c->setMiterLimit(d(2));
    else if (op == "lineDashOffset") c->setLineDashOffset(d(2));
    else if (op == "setLineDash") { std::vector<float> v; for (size_t i = 2; i < a.size(); i++) if (!a[i].empty()) v.push_back(float(d(i))); c->setLineDash(v); }
    else if (op == "font") c->setFont(a[2]);
    else if (op == "textAlign") c->setTextAlign(a[2]);
    else if (op == "textBaseline") c->setTextBaseline(a[2]);
    else if (op == "shadowColor") c->setShadowColor(a[2]);
    else if (op == "shadowBlur") c->setShadowBlur(d(2));
    else if (op == "shadowOffsetX") c->setShadowOffsetX(d(2));
    else if (op == "shadowOffsetY") c->setShadowOffsetY(d(2));
    else if (op == "save") c->save();
    else if (op == "restore") c->restore();
    else if (op == "translate") c->translate(d(2), d(3));
    else if (op == "scale") c->scale(d(2), d(3));
    else if (op == "rotate") c->rotate(d(2));
    else if (op == "setTransform") c->setTransform(d(2), d(3), d(4), d(5), d(6), d(7));
    else if (op == "beginPath") c->beginPath();
    else if (op == "closePath") c->closePath();
    else if (op == "moveTo") c->moveTo(d(2), d(3));
    else if (op == "lineTo") c->lineTo(d(2), d(3));
    else if (op == "quadraticCurveTo") c->quadraticCurveTo(d(2), d(3), d(4), d(5));
    else if (op == "bezierCurveTo") c->bezierCurveTo(d(2), d(3), d(4), d(5), d(6), d(7));
    else if (op == "rect") c->rect(d(2), d(3), d(4), d(5));
    else if (op == "arc") c->arc(d(2), d(3), d(4), d(5), d(6), d(7) != 0);
    else if (op == "ellipse") c->ellipse(d(2), d(3), d(4), d(5), d(6), d(7), d(8), d(9) != 0);
    else if (op == "fill") c->fill(n && a[2] == "evenodd");
    else if (op == "stroke") c->stroke();
    else if (op == "clip") c->clip(n && a[2] == "evenodd");
    else if (op == "fillRect") c->fillRect(d(2), d(3), d(4), d(5));
    else if (op == "strokeRect") c->strokeRect(d(2), d(3), d(4), d(5));
    else if (op == "clearRect") c->clearRect(d(2), d(3), d(4), d(5));
    else if (op == "fillText") c->fillText(a[2], d(3), d(4));
    else if (op == "strokeText") c->strokeText(a[2], d(3), d(4));
    else if (op == "drawImage") { Canvas *s = cv[a[2]].get(); if (s) { auto img = s->image(); if (n == 3) c->drawImage(img, d(3), d(4)); else if (n == 5) c->drawImage(img, d(3), d(4), d(5), d(6)); else c->drawImage(img, d(3), d(4), d(5), d(6), d(7), d(8), d(9), d(10)); } }
    else res.unknown[op]++;
  }
  keep.push_back(std::move(cv["0"])); main = keep.back().get();
}

int replay(const std::string &dir, const std::string &name) {
  Canvas *m = nullptr; Result r; double best = 1e9; for (int i = 0; i < 6; i++) { r.ms = 0; replayFrame(dir, name, m, r); best = std::min(best, r.ms); } r.ms = best;
  auto ref = loadPng(dir + "/" + name + ".png"); if (!m || !ref) { printf("%s: missing main canvas or reference\n", name.c_str()); return 1; }
  savePng(dir + "/" + name + ".cpp.png", m->pixels(), m->width(), m->height(), m->rowBytes());
  SkBitmap b; b.allocN32Pixels(ref->width(), ref->height()); SkPixmap pm(SkImageInfo::Make(ref->width(), ref->height(), kRGBA_8888_SkColorType, kPremul_SkAlphaType), nullptr, 0);
  std::vector<uint8_t> rp(size_t(ref->width()) * ref->height() * 4), diff(rp.size()); SkImageInfo info = SkImageInfo::Make(ref->width(), ref->height(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  ref->readPixels(nullptr, info, rp.data(), ref->width() * 4, 0, 0);
  const uint8_t *cp = (const uint8_t *)m->pixels(); size_t crb = m->rowBytes();
  for (int y = 0; y < ref->height(); y++) for (int x = 0; x < ref->width(); x++) {
    const uint8_t *p = rp.data() + (size_t(y) * ref->width() + x) * 4, *q = cp + y * crb + x * 4; int w = 0;
    for (int k = 0; k < 3; k++) { int e = std::abs(int(p[k]) - int(q[k])); w = std::max(w, e); r.sum += e; }
    r.total++; if (w > 1) r.off1++; if (w > 8) r.off8++; if (w > 32) r.off32++; r.worst = std::max(r.worst, w);
    uint8_t *o = diff.data() + (size_t(y) * ref->width() + x) * 4; int v = std::min(255, w * 8); o[0] = o[1] = o[2] = uint8_t(v); o[3] = 255;
  }
  savePng(dir + "/" + name + ".diff.png", diff.data(), ref->width(), ref->height(), size_t(ref->width()) * 4);
  double pc = 100.0 / r.total;
  printf("%-12s same or +-1: %6.2f%%   off by more than 8: %5.2f%%   more than 32: %5.2f%%   average %.3f / 255   worst %3d   drawn in %.2f ms\n", name.c_str(), 100 - r.off1 * pc, r.off8 * pc, r.off32 * pc, r.sum / r.total / 3, r.worst, r.ms);
  for (auto &[k, c] : r.unknown) printf("   not handled: %s x%d\n", k.c_str(), c);
  return 0;
}

}
