// src/layout_reader.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "rule_parser.h"

namespace tracer {

struct Polygon {
  std::vector<Point> pts;  // CCW
  int32_t minx=0, miny=0, maxx=0, maxy=0;
};

struct LayerData { std::vector<Polygon> polys; };

struct LayoutDB { std::unordered_map<std::string, LayerData> layers; };

bool LoadLayoutNeededLayers(const std::string& layout_path, const RuleFile& rule, LayoutDB& out);

} // namespace tracer
