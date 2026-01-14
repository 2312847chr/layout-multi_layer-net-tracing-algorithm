#include "writer.h"
#include <fstream>
#include <vector>
#include <algorithm>

namespace tracer {

static void WritePolyLine(std::ofstream& out, const std::vector<Point>& pts) {
  for (size_t i=0;i<pts.size();i++){
    out << "(" << pts[i].x << "," << pts[i].y << ")";
    if (i+1<pts.size()) out << ",";
  }
  out << "\n";
}

bool WriteResult(const std::string& path, const TraceResult& res) {
  std::ofstream out(path, std::ios::out | std::ios::binary);
  if (!out) return false;

  std::vector<std::string> layers;
  layers.reserve(res.by_layer.size());
  for (auto& kv: res.by_layer) layers.push_back(kv.first);
  std::sort(layers.begin(), layers.end());

  for (auto& layer: layers) {
    out << layer << "\n";
    const auto& polys = res.by_layer.at(layer);
    for (auto& poly: polys) WritePolyLine(out, poly);
  }
  return true;
}

} // namespace tracer
