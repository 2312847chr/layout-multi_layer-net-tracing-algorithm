#pragma once
#include "rule_parser.h"
#include "layout_reader.h"
#include <unordered_map>
#include <vector>

namespace tracer {

struct TraceResult {
  std::unordered_map<std::string, std::vector<std::vector<Point>>> by_layer;
  size_t total_polygons = 0;
};

bool RunTrace(const RuleFile& rule, const LayoutDB& db, int threads, TraceResult& out);

} // namespace tracer
