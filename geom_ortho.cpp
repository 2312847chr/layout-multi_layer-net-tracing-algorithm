// src/geom_ortho.cpp
#include "geom_ortho.h"
#include <algorithm>
#include <cstdint>

namespace tracer {

static inline bool BBoxOverlap(const Polygon& a, const Polygon& b) {
  return !(a.maxx < b.minx || b.maxx < a.minx || a.maxy < b.miny || b.maxy < a.miny);
}

static inline bool OnSegment(const Point& p, const Point& a, const Point& b) {
  int64_t x=p.x,y=p.y,x1=a.x,y1=a.y,x2=b.x,y2=b.y;
  if ((x2-x1)*(y-y1) != (y2-y1)*(x-x1)) return false;
  return (std::min(x1,x2) <= x && x <= std::max(x1,x2) &&
          std::min(y1,y2) <= y && y <= std::max(y1,y2));
}

bool PointInPolyInclusiveOrtho(const Point& pt, const Polygon& poly) {
  const auto& P = poly.pts;
  int n=(int)P.size();
  for (int i=0;i<n;i++){
    if (OnSegment(pt, P[i], P[(i+1)%n])) return true;
  }

  bool inside=false;
  int64_t x=pt.x, y=pt.y;
  for (int i=0;i<n;i++){
    int64_t x1=P[i].x, y1=P[i].y;
    int64_t x2=P[(i+1)%n].x, y2=P[(i+1)%n].y;
    if (y1>y2) { std::swap(y1,y2); std::swap(x1,x2); }
    if (y <= y1 || y > y2) continue;
    int64_t dy=y2-y1;
    if (dy==0) continue;
    int64_t left = x1*dy + (x2-x1)*(y-y1);
    int64_t right = x*dy;
    if (left >= right) inside = !inside;
  }
  return inside;
}

static inline bool SegIntersectManhattan(const Point& a1, const Point& a2, const Point& b1, const Point& b2) {
  bool aV = (a1.x==a2.x), aH=(a1.y==a2.y);
  bool bV = (b1.x==b2.x), bH=(b1.y==b2.y);

  if ((aV||aH) && (bV||bH)) {
    if (aV && bH) {
      int64_t ax=a1.x, by=b1.y;
      int64_t bminx=std::min(b1.x,b2.x), bmaxx=std::max(b1.x,b2.x);
      int64_t aminy=std::min(a1.y,a2.y), amaxy=std::max(a1.y,a2.y);
      return (bminx<=ax && ax<=bmaxx && aminy<=by && by<=amaxy);
    }
    if (aH && bV) return SegIntersectManhattan(b1,b2,a1,a2);
    if (aV && bV) {
      if (a1.x!=b1.x) return false;
      int64_t a0=std::min(a1.y,a2.y), a1y=std::max(a1.y,a2.y);
      int64_t b0=std::min(b1.y,b2.y), b1y=std::max(b1.y,b2.y);
      return !(a1y < b0 || b1y < a0);
    }
    if (aH && bH) {
      if (a1.y!=b1.y) return false;
      int64_t a0=std::min(a1.x,a2.x), a1x=std::max(a1.x,a2.x);
      int64_t b0=std::min(b1.x,b2.x), b1x=std::max(b1.x,b2.x);
      return !(a1x < b0 || b1x < a0);
    }
  }
  // fallback bbox
  int64_t aminx=std::min(a1.x,a2.x), amaxx=std::max(a1.x,a2.x);
  int64_t aminy=std::min(a1.y,a2.y), amaxy=std::max(a1.y,a2.y);
  int64_t bminx=std::min(b1.x,b2.x), bmaxx=std::max(b1.x,b2.x);
  int64_t bminy=std::min(b1.y,b2.y), bmaxy=std::max(b1.y,b2.y);
  return !(amaxx < bminx || bmaxx < aminx || amaxy < bminy || bmaxy < aminy);
}

bool PolyIntersectOrtho(const Polygon& a, const Polygon& b) {
  if (!BBoxOverlap(a,b)) return false;
  const auto& A=a.pts; const auto& B=b.pts;
  int na=(int)A.size(), nb=(int)B.size();

  for (int i=0;i<na;i++){
    Point a1=A[i], a2=A[(i+1)%na];
    for (int j=0;j<nb;j++){
      Point b1=B[j], b2=B[(j+1)%nb];
      if (SegIntersectManhattan(a1,a2,b1,b2)) return true;
    }
  }
  if (PointInPolyInclusiveOrtho(A[0], b)) return true;
  if (PointInPolyInclusiveOrtho(B[0], a)) return true;
  return false;
}

} // namespace tracer
