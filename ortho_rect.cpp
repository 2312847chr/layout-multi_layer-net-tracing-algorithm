// src/ortho_rect.cpp
#include "ortho_rect.h"
#include <algorithm>
#include <unordered_map>
#include <set>

namespace tracer {

// ---- polygon -> rects (scan by unique y) ----
static void CollectUniqueY(const Polygon& p, std::vector<int32_t>& ys) {
  ys.clear();
  ys.reserve(p.pts.size());
  for (auto& pt: p.pts) ys.push_back(pt.y);
  std::sort(ys.begin(), ys.end());
  ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
}

static std::vector<int32_t> XCrossingsAtY(const Polygon& p, int32_t y) {
  std::vector<int32_t> xs;
  const auto& P = p.pts;
  int n=(int)P.size();
  for (int i=0;i<n;i++){
    auto a=P[i], b=P[(i+1)%n];
    if (a.y==b.y) continue; // ignore horizontal
    int32_t y1=a.y, y2=b.y;
    int32_t x1=a.x, x2=b.x;
    if (y1>y2) { std::swap(y1,y2); std::swap(x1,x2); }
    if (y<=y1 || y>y2) continue; // (y1,y2]
    // vertical edge => x constant
    xs.push_back(x1);
  }
  std::sort(xs.begin(), xs.end());
  return xs;
}

std::vector<Rect> DecomposeToRects(const Polygon& poly) {
  std::vector<Rect> rects;
  std::vector<int32_t> ys;
  CollectUniqueY(poly, ys);
  if (ys.size()<2) return rects;

  for (size_t i=0;i+1<ys.size();i++){
    int32_t y0=ys[i], y1=ys[i+1];
    if (y0==y1) continue;
    int32_t ymid = y0 + (y1-y0)/2;
    auto xs = XCrossingsAtY(poly, ymid);
    for (size_t k=0;k+1<xs.size();k+=2){
      int32_t x0=xs[k], x1c=xs[k+1];
      if (x0>x1c) std::swap(x0,x1c);
      if (x0==x1c) continue;
      rects.push_back(Rect{x0,y0,x1c,y1});
    }
  }
  return rects;
}

// ---- rect difference A - B (simple splitting) ----
static inline bool RectOverlap(const Rect& a, const Rect& b) {
  return !(a.x2<=b.x1 || b.x2<=a.x1 || a.y2<=b.y1 || b.y2<=a.y1);
}

static std::vector<Rect> SubtractOne(const Rect& a, const Rect& b) {
  // returns pieces of a after removing intersection with b
  if (!RectOverlap(a,b)) return {a};
  int32_t ix1=std::max(a.x1,b.x1), iy1=std::max(a.y1,b.y1);
  int32_t ix2=std::min(a.x2,b.x2), iy2=std::min(a.y2,b.y2);
  if (ix1>=ix2 || iy1>=iy2) return {a};

  std::vector<Rect> out;
  // top
  if (iy2 < a.y2) out.push_back(Rect{a.x1,iy2,a.x2,a.y2});
  // bottom
  if (a.y1 < iy1) out.push_back(Rect{a.x1,a.y1,a.x2,iy1});
  // left
  if (a.x1 < ix1) out.push_back(Rect{a.x1,iy1,ix1,iy2});
  // right
  if (ix2 < a.x2) out.push_back(Rect{ix2,iy1,a.x2,iy2});
  return out;
}

std::vector<Rect> RectDifference(const std::vector<Rect>& A, const std::vector<Rect>& B) {
  std::vector<Rect> cur = A;
  for (auto& b : B) {
    std::vector<Rect> next;
    for (auto& a : cur) {
      auto pieces = SubtractOne(a,b);
      next.insert(next.end(), pieces.begin(), pieces.end());
    }
    cur.swap(next);
    if (cur.empty()) break;
  }
  // remove degenerate
  std::vector<Rect> out;
  out.reserve(cur.size());
  for (auto& r: cur) if (r.x1<r.x2 && r.y1<r.y2) out.push_back(r);
  return out;
}

// ---- rects -> boundary polygons (edge cancel + loop trace) ----
struct Edge { int32_t x1,y1,x2,y2;
  bool operator<(const Edge& o) const {
    if (x1!=o.x1) return x1<o.x1;
    if (y1!=o.y1) return y1<o.y1;
    if (x2!=o.x2) return x2<o.x2;
    return y2<o.y2;
  }
};

static void AddOrCancel(std::multiset<Edge>& edges, const Edge& e) {
  Edge rev{e.x2,e.y2,e.x1,e.y1};
  auto it = edges.find(rev);
  if (it!=edges.end()) edges.erase(it);
  else edges.insert(e);
}

static std::vector<std::vector<Point>> TraceLoops(std::multiset<Edge>& edges) {
  using Key=uint64_t;
  auto pack=[&](int32_t x,int32_t y)->Key{ return (uint64_t)(uint32_t)x<<32 | (uint32_t)y; };

  std::unordered_map<Key, std::vector<Edge>> adj;
  adj.reserve(edges.size()*2);
  for (auto& e: edges) adj[pack(e.x1,e.y1)].push_back(e);

  auto dirRank=[](const Edge& e)->int{
    int32_t dx=e.x2-e.x1, dy=e.y2-e.y1;
    if (dy==0 && dx>0) return 0;
    if (dx==0 && dy>0) return 1;
    if (dy==0 && dx<0) return 2;
    return 3;
  };
  for (auto& kv: adj) {
    auto& v=kv.second;
    std::sort(v.begin(), v.end(), [&](const Edge& a,const Edge& b){return dirRank(a)<dirRank(b);});
  }

  std::set<Edge> used;
  std::vector<std::vector<Point>> polys;

  for (auto& e0: edges) {
    if (used.count(e0)) continue;
    std::vector<Point> poly;
    Edge cur=e0;
    used.insert(cur);
    poly.push_back(Point{cur.x1,cur.y1});
    while (true) {
      Point end{cur.x2,cur.y2};
      if (end.x==e0.x1 && end.y==e0.y1) break;
      poly.push_back(end);
      auto it=adj.find(pack(end.x,end.y));
      if (it==adj.end() || it->second.empty()) break;
      Edge nxt=it->second[0];
      for (auto& cand: it->second) {
        if (!(cand.x2==cur.x1 && cand.y2==cur.y1)) { nxt=cand; break; }
      }
      if (used.count(nxt)) break;
      used.insert(nxt);
      cur=nxt;
    }
    if (poly.size()>=4) polys.push_back(std::move(poly));
  }
  return polys;
}

std::vector<std::vector<Point>> RectsToPolygons(const std::vector<Rect>& rects) {
  std::multiset<Edge> edges;
  for (auto& r: rects) {
    if (r.x1>=r.x2 || r.y1>=r.y2) continue;
    AddOrCancel(edges, Edge{r.x1,r.y1,r.x2,r.y1});
    AddOrCancel(edges, Edge{r.x2,r.y1,r.x2,r.y2});
    AddOrCancel(edges, Edge{r.x2,r.y2,r.x1,r.y2});
    AddOrCancel(edges, Edge{r.x1,r.y2,r.x1,r.y1});
  }
  return TraceLoops(edges);
}

} // namespace tracer
