// src/geom_ortho.h
#pragma once
#include "layout_reader.h"
namespace tracer {
bool PointInPolyInclusiveOrtho(const Point& pt, const Polygon& poly);
bool PolyIntersectOrtho(const Polygon& a, const Polygon& b);
}
