#pragma once
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "layout_reader.h"

namespace tracer {

struct CellKey {
  int32_t gx, gy;
  bool operator==(const CellKey& o) const { return gx==o.gx && gy==o.gy; }
};

struct CellKeyHash {
  size_t operator()(const CellKey& k) const {
    return (uint64_t)(uint32_t)k.gx * 1315423911u ^ (uint64_t)(uint32_t)k.gy * 2654435761u;
  }
};

int32_t AutoCellSize(const std::vector<Polygon>& polys);

class SpatialIndex {
public:
  void Build(const std::vector<Polygon>& polys, int32_t cell_size);
  void QueryCandidates(const Polygon& q, std::vector<int>& out) const; // append
private:
  int32_t cell_ = 1024;
  std::unordered_map<CellKey, std::vector<int>, CellKeyHash> grid_;
};

} // namespace tracer
