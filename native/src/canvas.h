#pragma once
#include <memory>
#include <string>
#include <vector>
#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkImage.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkShader.h"
#include "include/core/SkSurface.h"

namespace wf {

/* ---------- Colors, gradients, patterns ---------- */
SkColor4f parseColor(const std::string &s);

struct Gradient {
  bool radial = false; float x0 = 0, y0 = 0, r0 = 0, x1 = 0, y1 = 0, r1 = 0;
  std::vector<float> at; std::vector<SkColor4f> col; sk_sp<SkShader> built;
  void addColorStop(double o, const std::string &c);
  sk_sp<SkShader> shader();
};
using GradientRef = std::shared_ptr<Gradient>;

struct Style { SkColor4f color = {0, 0, 0, 1}; GradientRef grad; sk_sp<SkShader> pat; };

/* ---------- Canvas: the browser's 2D calls, same names and same rules ---------- */
class Canvas {
public:
  Canvas(int w, int h);
  int width() const { return w_; } int height() const { return h_; }
  sk_sp<SkImage> image(); const void *pixels(); size_t rowBytes() const;
  void loadImage(sk_sp<SkImage> img);

  void save(); void restore();
  void translate(double x, double y); void scale(double x, double y); void rotate(double a);
  void setTransform(double a, double b, double c, double d, double e, double f);

  void setFillStyle(const std::string &c); void setFillStyle(GradientRef g); void setFillPattern(sk_sp<SkShader> s);
  void setStrokeStyle(const std::string &c); void setStrokeStyle(GradientRef g); void setStrokePattern(sk_sp<SkShader> s);
  void setGlobalAlpha(double a); void setGlobalCompositeOperation(const std::string &op);
  void setLineWidth(double w); void setLineCap(const std::string &c); void setLineJoin(const std::string &j); void setMiterLimit(double m);
  void setLineDash(std::vector<float> d); void setLineDashOffset(double o);
  void setFont(const std::string &f); void setTextAlign(const std::string &a); void setTextBaseline(const std::string &b);
  void setShadowColor(const std::string &c); void setShadowBlur(double b); void setShadowOffsetX(double x); void setShadowOffsetY(double y);
  static GradientRef createLinearGradient(double x0, double y0, double x1, double y1);
  static GradientRef createRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1);
  static sk_sp<SkShader> createPattern(sk_sp<SkImage> img);

  void beginPath(); void closePath(); void moveTo(double x, double y); void lineTo(double x, double y);
  void quadraticCurveTo(double cx, double cy, double x, double y); void bezierCurveTo(double a, double b, double c, double d, double x, double y);
  void rect(double x, double y, double w, double h); void arc(double x, double y, double r, double a0, double a1, bool ccw = false);
  void ellipse(double x, double y, double rx, double ry, double rot, double a0, double a1, bool ccw = false);
  void fill(bool evenOdd = false); void stroke(); void clip(bool evenOdd = false);
  void fillRect(double x, double y, double w, double h); void strokeRect(double x, double y, double w, double h); void clearRect(double x, double y, double w, double h);
  void fillText(const std::string &t, double x, double y); void strokeText(const std::string &t, double x, double y); double measureText(const std::string &t);
  void drawImage(sk_sp<SkImage> img, double dx, double dy); void drawImage(sk_sp<SkImage> img, double dx, double dy, double dw, double dh);
  void drawImage(sk_sp<SkImage> img, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh);

  static bool loadFonts(const std::string &dir);

private:
  struct State {
    Style fill, stroke; float alpha = 1; SkBlendMode blend = SkBlendMode::kSrcOver;
    float lineWidth = 1, miter = 10, dashOff = 0; SkPaint::Cap cap = SkPaint::kButt_Cap; SkPaint::Join join = SkPaint::kMiter_Join; std::vector<float> dash;
    SkFont font; bool textCenter = false, textRight = false; int baseline = 0;
    SkColor4f shadow = {0, 0, 0, 0}; float blur = 0, sx = 0, sy = 0;
  };
  SkPaint paintFor(const Style &s, bool stroking);
  template <class F> void paintWith(const SkPaint &p, F draw);
  void textAt(const std::string &t, double x, double y, bool stroking);
  void shiftPath(const SkMatrix &change);
  int w_, h_; sk_sp<SkSurface> surf_; SkCanvas *c_; State st_; std::vector<State> stack_;
  SkPath path_; bool hasPoint_ = false;
};

}
