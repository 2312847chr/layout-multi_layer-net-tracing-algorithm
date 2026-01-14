// src/cli.cpp
#include "cli.h"
#include "rule_parser.h"
#include "layout_reader.h"
#include "engine.h"
#include "writer.h"
#include <iostream>

namespace tracer {

int RunCLI(int argc, char** argv) {
  CmdArgs args;
  if (!ParseArgs(argc, argv, args)) {
    std::cerr << "Usage:\n"
              << "  trace -layout layout.txt -rule rule.txt -output res.txt [-thread N]\n";
    return 1;
  }

  RuleFile rule;
  if (!LoadRule(args.rule_path, rule)) return 2;

  LayoutDB db;
  if (!LoadLayoutNeededLayers(args.layout_path, rule, db)) return 3;

  TraceResult res;
  if (!RunTrace(rule, db, args.threads, res)) return 4;

  if (!WriteResult(args.output_path, res)) return 5;

  std::cerr << "[OK] layers_out=" << res.by_layer.size()
            << " polys_out=" << res.total_polygons << "\n";
  return 0;
}

} // namespace tracer
