#include "engine.h"
#include "geom_ortho.h"
#include "spatial_index.h"
#include "ortho_rect.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

namespace tracer {

struct Node { std::string layer; int idx; };

static bool PolyContainsStart(const Polygon& p, const Point& s) {
  if (s.x < p.minx || s.x > p.maxx || s.y < p.miny || s.y > p.maxy) return false;
  return PointInPolyInclusiveOrtho(s, p);
}

static void BuildLayerIndices(
  const LayoutDB& db,
  std::unordered_map<std::string, SpatialIndex>& idxmap
) {
  idxmap.clear();
  for (auto& kv: db.layers) {
    const auto& layer = kv.first;
    const auto& polys = kv.second.polys;
    int32_t cs = AutoCellSize(polys);
    SpatialIndex si;
    si.Build(polys, cs);
    idxmap.emplace(layer, std::move(si));
  }
}

static void BuildViaAdj(
  const RuleFile& rule,
  std::unordered_map<std::string, std::vector<std::string>>& via_adj
) {
  via_adj.clear();
  for (auto& vr: rule.via_rules) {
    for (size_t i=0;i+1<vr.layers.size();i++){
      const auto& a = vr.layers[i];
      const auto& b = vr.layers[i+1];
      via_adj[a].push_back(b);
      via_adj[b].push_back(a);
    }
  }
}

static void BFS_MultiLayer(
  const RuleFile& rule,
  const LayoutDB& db,
  const std::unordered_map<std::string, SpatialIndex>& idxmap,
  const std::vector<std::pair<std::string, Point>>& starts,
  std::unordered_map<std::string, std::vector<char>>& visited_layer
) {
  visited_layer.clear();
  for (auto& kv: db.layers) {
    visited_layer[kv.first].assign(kv.second.polys.size(), 0);
  }

  std::unordered_map<std::string, std::vector<std::string>> via_adj;
  BuildViaAdj(rule, via_adj);

  std::queue<Node> q;

  // seed: ALL polygons containing each start point
  for (auto& st: starts) {
    auto it = db.layers.find(st.first);
    if (it==db.layers.end()) continue;
    const auto& polys = it->second.polys;
    for (int i=0;i<(int)polys.size();i++){
      if (PolyContainsStart(polys[i], st.second)) {
        if (!visited_layer[st.first][i]) {
          visited_layer[st.first][i]=1;
          q.push(Node{st.first,i});
        }
      }
    }
  }

  std::vector<int> cand;
  cand.reserve(2048);

  while (!q.empty()) {
    Node cur = q.front(); q.pop();
    const auto& layer = cur.layer;
    const auto& polys = db.layers.at(layer).polys;
    const Polygon& pu = polys[cur.idx];

    // same-layer expansion
    cand.clear();
    idxmap.at(layer).QueryCandidates(pu, cand);
    std::sort(cand.begin(), cand.end());
    cand.erase(std::unique(cand.begin(), cand.end()), cand.end());

    for (int v: cand) {
      if (v==cur.idx) continue;
      if (visited_layer[layer][v]) continue;
      if (PolyIntersectOrtho(pu, polys[v])) {
        visited_layer[layer][v]=1;
        q.push(Node{layer,v});
      }
    }

    // via expansion
    auto itadj = via_adj.find(layer);
    if (itadj!=via_adj.end()) {
      for (const auto& nb : itadj->second) {
        auto itL = db.layers.find(nb);
        if (itL==db.layers.end()) continue;
        const auto& polysB = itL->second.polys;

        cand.clear();
        idxmap.at(nb).QueryCandidates(pu, cand);
        std::sort(cand.begin(), cand.end());
        cand.erase(std::unique(cand.begin(), cand.end()), cand.end());

        for (int v: cand) {
          if (visited_layer[nb][v]) continue;
          if (PolyIntersectOrtho(pu, polysB[v])) {
            visited_layer[nb][v]=1;
            q.push(Node{nb,v});
          }
        }
      }
    }
  }
}

// --- Q3 AA cutting using rect decomposition ---
// AA_final = (AA - (AA ∩ LOW)) ∪ (AA ∩ HIGH)
// Here we approximate via rect operations exactly on Manhattan grid.
static std::vector<std::vector<Point>> CutAAByPoly_Rect(
  const Polygon& aa,
  const std::vector<const Polygon*>& poly_high,
  const std::vector<const Polygon*>& poly_low
){
  // 1) AA -> rects
  auto aa_rects = DecomposeToRects(aa);

  // 2) LOW polys -> rects union list (not merged, but ok for difference)
  std::vector<Rect> low_rects;
  for (auto* p : poly_low) {
    auto rs = DecomposeToRects(*p);
    low_rects.insert(low_rects.end(), rs.begin(), rs.end());
  }

  // 3) HIGH polys -> rects (for re-adding)
  std::vector<Rect> high_rects;
  for (auto* p : poly_high) {
    auto rs = DecomposeToRects(*p);
    high_rects.insert(high_rects.end(), rs.begin(), rs.end());
  }

  // 4) AA_cut = AA_rects - low_rects
  auto aa_cut = RectDifference(aa_rects, low_rects);

  // 5) AA_on = intersection(AA, HIGH) using rect overlap (rect-level)
  // Instead of boolean library, compute AA ∩ HIGH by intersecting each AA rect with each HIGH rect
  std::vector<Rect> aa_on;
  aa_on.reserve(aa_rects.size()/4 + 16);

  for (auto& ar : aa_rects) {
    for (auto& hr : high_rects) {
      int32_t x1 = std::max(ar.x1, hr.x1);
      int32_t y1 = std::max(ar.y1, hr.y1);
      int32_t x2 = std::min(ar.x2, hr.x2);
      int32_t y2 = std::min(ar.y2, hr.y2);
      if (x1 < x2 && y1 < y2) aa_on.push_back(Rect{x1,y1,x2,y2});
    }
  }

  // 6) final rects = aa_cut + aa_on (not merged; boundary cancel will handle overlaps)
  aa_cut.insert(aa_cut.end(), aa_on.begin(), aa_on.end());

  // 7) rects -> polygons
  return RectsToPolygons(aa_cut);
}

bool RunTrace(const RuleFile& rule, const LayoutDB& db, int threads, TraceResult& out) {
  (void)threads;
  out.by_layer.clear();
  out.total_polygons = 0;

  std::unordered_map<std::string, SpatialIndex> idxmap;
  BuildLayerIndices(db, idxmap);

  bool is_q3 = (rule.starts.size() >= 2) && rule.gate.has_gate;

  if (!is_q3) {
    // Q1/Q2
    std::unordered_map<std::string, std::vector<char>> vis;
    BFS_MultiLayer(rule, db, idxmap, {rule.starts[0]}, vis);

    for (auto& kv: vis) {
      const auto& layer = kv.first;
      const auto& flags = kv.second;
      const auto& polys = db.layers.at(layer).polys;
      std::vector<std::vector<Point>> outs;
      outs.reserve(polys.size());
      for (int i=0;i<(int)flags.size();i++){
        if (flags[i]) outs.push_back(polys[i].pts);
      }
      if (!outs.empty()) {
        out.by_layer[layer] = std::move(outs);
        out.total_polygons += out.by_layer[layer].size();
      }
    }
    return true;
  }

  // Q3
  // Phase A: start1 -> mark poly_high
  std::unordered_map<std::string, std::vector<char>> vis_s1;
  BFS_MultiLayer(rule, db, idxmap, {rule.starts[0]}, vis_s1);

  std::unordered_set<int> poly_high_set;
  auto itPoly = db.layers.find(rule.gate.poly_layer);
  if (itPoly != db.layers.end()) {
    const auto& f = vis_s1[rule.gate.poly_layer];
    for (int i=0;i<(int)f.size();i++) if (f[i]) poly_high_set.insert(i);
  }

  // Phase B: start2 -> trace connectivity
  std::unordered_map<std::string, std::vector<char>> vis_s2;
  BFS_MultiLayer(rule, db, idxmap, {rule.starts[1]}, vis_s2);

  // output all layers except AA first
  for (auto& kv: vis_s2) {
    const auto& layer = kv.first;
    if (layer == rule.gate.aa_layer) continue;
    const auto& flags = kv.second;
    const auto& polys = db.layers.at(layer).polys;

    std::vector<std::vector<Point>> outs;
    outs.reserve(polys.size());
    for (int i=0;i<(int)flags.size();i++){
      if (flags[i]) outs.push_back(polys[i].pts);
    }
    if (!outs.empty()) {
      out.by_layer[layer] = std::move(outs);
      out.total_polygons += out.by_layer[layer].size();
    }
  }

  // AA cutting
  auto itAA = db.layers.find(rule.gate.aa_layer);
  if (itAA != db.layers.end() && itPoly != db.layers.end()) {
    const auto& aa_polys  = itAA->second.polys;
    const auto& aa_flags  = vis_s2[rule.gate.aa_layer];
    const auto& poly_polys = itPoly->second.polys;

    std::vector<std::vector<Point>> aa_out;
    std::vector<int> cand;
    cand.reserve(2048);

    for (int ai=0; ai<(int)aa_flags.size(); ai++){
      if (!aa_flags[ai]) continue;
      const Polygon& aa = aa_polys[ai];

      // candidate poly intersecting AA
      cand.clear();
      idxmap.at(rule.gate.poly_layer).QueryCandidates(aa, cand);
      std::sort(cand.begin(), cand.end());
      cand.erase(std::unique(cand.begin(), cand.end()), cand.end());

      std::vector<const Polygon*> poly_high;
      std::vector<const Polygon*> poly_low;

      for (int pi: cand) {
        const Polygon& pp = poly_polys[pi];
        if (!PolyIntersectOrtho(aa, pp)) continue;
        if (poly_high_set.count(pi)) poly_high.push_back(&pp);
        else poly_low.push_back(&pp);
      }

      auto cut_polys = CutAAByPoly_Rect(aa, poly_high, poly_low);
      aa_out.insert(aa_out.end(), cut_polys.begin(), cut_polys.end());
    }

    if (!aa_out.empty()) {
      out.by_layer[rule.gate.aa_layer] = std::move(aa_out);
      out.total_polygons += out.by_layer[rule.gate.aa_layer].size();
    }
  }

  return true;
}

} // namespace tracer
