// src/ortho_rect.h
#pragma once
#include <vector>
#include <cstdint>
#include "layout_reader.h"

namespace tracer {

struct Rect { int32_t x1,y1,x2,y2; }; // [x1,x2) [y1,y2)

std::vector<Rect> DecomposeToRects(const Polygon& poly);           // polygon -> rects
std::vector<Rect> RectDifference(const std::vector<Rect>& A, const std::vector<Rect>& B); // A - B
std::vector<std::vector<Point>> RectsToPolygons(const std::vector<Rect>& rects); // rects -> boundary polygons

} // namespace tracer
