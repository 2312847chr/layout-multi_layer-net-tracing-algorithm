// src/rule_parser.h
#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>

namespace tracer {

struct Point { int32_t x=0, y=0; };

struct ViaRule { std::vector<std::string> layers; };

struct GateRule {
  bool has_gate = false;
  std::string poly_layer;
  std::string aa_layer;
};

struct RuleFile {
  std::vector<std::pair<std::string, Point>> starts; // 1 or 2
  std::vector<ViaRule> via_rules;
  GateRule gate;
  std::unordered_set<std::string> needed_layers;
};

struct CmdArgs {
  std::string layout_path, rule_path, output_path;
  int threads = 1;
};

bool ParseArgs(int argc, char** argv, CmdArgs& out);
bool LoadRule(const std::string& path, RuleFile& out);

} // namespace tracer
