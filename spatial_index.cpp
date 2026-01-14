#include "spatial_index.h"
#include <algorithm>

namespace tracer {

int32_t AutoCellSize(const std::vector<Polygon>& polys) {
  if (polys.empty()) return 1024;
  size_t n = polys.size();
  size_t step = std::max<size_t>(1, n / 2000);
  std::vector<int32_t> ws, hs;
  for (size_t i=0, cnt=0; i<n && cnt<2000; i+=step, ++cnt) {
    ws.push_back(std::max<int32_t>(1, polys[i].maxx - polys[i].minx));
    hs.push_back(std::max<int32_t>(1, polys[i].maxy - polys[i].miny));
  }
  std::nth_element(ws.begin(), ws.begin()+ws.size()/2, ws.end());
  std::nth_element(hs.begin(), hs.begin()+hs.size()/2, hs.end());
  int32_t med = std::max(ws[ws.size()/2], hs[hs.size()/2]);
  return std::max<int32_t>(64, med*4);
}

static inline void CellsForBBox(const Polygon& p, int32_t cell,
                                int32_t& gx0,int32_t& gy0,int32_t& gx1,int32_t& gy1){
  gx0 = p.minx / cell; gy0 = p.miny / cell;
  gx1 = p.maxx / cell; gy1 = p.maxy / cell;
}

void SpatialIndex::Build(const std::vector<Polygon>& polys, int32_t cell_size) {
  cell_ = cell_size>0?cell_size:1024;
  grid_.clear();
  grid_.reserve(polys.size());

  for (int i=0;i<(int)polys.size();i++){
    int32_t gx0,gy0,gx1,gy1;
    CellsForBBox(polys[i], cell_, gx0,gy0,gx1,gy1);
    for (int32_t gx=gx0; gx<=gx1; gx++){
      for (int32_t gy=gy0; gy<=gy1; gy++){
        grid_[CellKey{gx,gy}].push_back(i);
      }
    }
  }
}

void SpatialIndex::QueryCandidates(const Polygon& q, std::vector<int>& out) const {
  int32_t gx0,gy0,gx1,gy1;
  CellsForBBox(q, cell_, gx0,gy0,gx1,gy1);
  for (int32_t gx=gx0; gx<=gx1; gx++){
    for (int32_t gy=gy0; gy<=gy1; gy++){
      auto it = grid_.find(CellKey{gx,gy});
      if (it==grid_.end()) continue;
      out.insert(out.end(), it->second.begin(), it->second.end());
    }
  }
}

} // namespace tracer
