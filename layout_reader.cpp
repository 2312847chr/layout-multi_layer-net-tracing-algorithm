// src/layout_reader.cpp
#include "layout_reader.h"
#include "utils.h"
#include <fstream>
#include <iostream>

namespace tracer {

static bool ParsePolyLine(const std::string& line, Polygon& poly) {
  poly.pts.clear();
  bool first=true;
  int64_t minx=0,miny=0,maxx=0,maxy=0;

  size_t i=0, n=line.size();
  while (i<n) {
    while (i<n && line[i] != '(') i++;
    if (i>=n) break;
    i++;
    size_t cm = line.find(',', i);
    if (cm==std::string::npos) return false;
    size_t rp = line.find(')', cm);
    if (rp==std::string::npos) return false;

    int32_t x = (int32_t)std::stoll(line.substr(i, cm-i));
    int32_t y = (int32_t)std::stoll(line.substr(cm+1, rp-cm-1));
    poly.pts.push_back(Point{x,y});

    if (first) { minx=maxx=x; miny=maxy=y; first=false; }
    else {
      if (x<minx) minx=x; if (x>maxx) maxx=x;
      if (y<miny) miny=y; if (y>maxy) maxy=y;
    }
    i = rp+1;
  }

  if (poly.pts.size() < 4) return false;
  poly.minx=(int32_t)minx; poly.miny=(int32_t)miny; poly.maxx=(int32_t)maxx; poly.maxy=(int32_t)maxy;
  return true;
}

bool LoadLayoutNeededLayers(const std::string& layout_path, const RuleFile& rule, LayoutDB& out) {
  std::ifstream fin(layout_path);
  if (!fin) { std::cerr<<"Cannot open layout: "<<layout_path<<"\n"; return false; }

  out.layers.clear();
  std::string cur_layer;
  bool keep=false;

  std::string line;
  while (std::getline(fin, line)) {
    line = Trim(line);
    if (line.empty()) continue;

    if (IsLayerLine(line)) {
      cur_layer = line;
      keep = (rule.needed_layers.find(cur_layer) != rule.needed_layers.end());
      if (keep && out.layers.find(cur_layer)==out.layers.end()) out.layers[cur_layer]=LayerData{};
      continue;
    }

    if (keep && !cur_layer.empty()) {
      Polygon p;
      if (ParsePolyLine(line, p)) out.layers[cur_layer].polys.push_back(std::move(p));
    }
  }
  return true;
}

} // namespace tracer
