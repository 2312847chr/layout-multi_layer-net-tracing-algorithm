// src/rule_parser.cpp
#include "rule_parser.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace tracer {

bool ParseArgs(int argc, char** argv, CmdArgs& out) {
  for (int i=1;i<argc;i++) {
    std::string a = argv[i];
    auto need = [&](const char* key)->std::string{
      if (i+1>=argc) { std::cerr<<"Missing value for "<<key<<"\n"; return ""; }
      return argv[++i];
    };
    if (a=="-layout") out.layout_path = need("-layout");
    else if (a=="-rule") out.rule_path = need("-rule");
    else if (a=="-output") out.output_path = need("-output");
    else if (a=="-thread") out.threads = std::max(1, std::atoi(need("-thread").c_str()));
  }
  return !out.layout_path.empty() && !out.rule_path.empty() && !out.output_path.empty();
}

static bool ParseStartLine(const std::string& line, std::string& layer, Point& p) {
  auto s = Trim(line);
  auto sp = s.find(' ');
  if (sp==std::string::npos) return false;
  layer = s.substr(0, sp);

  auto lp = s.find('(', sp);
  auto cm = s.find(',', lp);
  auto rp = s.find(')', cm);
  if (lp==std::string::npos||cm==std::string::npos||rp==std::string::npos) return false;

  p.x = (int32_t)std::stoll(s.substr(lp+1, cm-lp-1));
  p.y = (int32_t)std::stoll(s.substr(cm+1, rp-cm-1));
  return true;
}

bool LoadRule(const std::string& path, RuleFile& out) {
  std::ifstream fin(path);
  if (!fin) { std::cerr<<"Cannot open rule: "<<path<<"\n"; return false; }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(fin, line)) {
    line = Trim(line);
    if (!line.empty()) lines.push_back(line);
  }

  enum Mode { NONE, START, VIA, GATE } mode = NONE;

  for (size_t i=0;i<lines.size();i++) {
    const std::string& L = lines[i];
    if (L=="StartPos") { mode = START; continue; }
    if (L=="Via") { mode = VIA; continue; }
    if (L=="Gate") { mode = GATE; continue; }

    if (mode==START) {
      std::string layer; Point p;
      if (ParseStartLine(L, layer, p)) out.starts.push_back({layer,p});
      continue;
    }
    if (mode==VIA) {
      auto toks = SplitWS(L);
      if (!toks.empty()) { ViaRule vr; vr.layers = toks; out.via_rules.push_back(vr); }
      continue;
    }
    if (mode==GATE) {
      auto toks = SplitWS(L);
      if (toks.size()>=2) {
        out.gate.has_gate = true;
        out.gate.poly_layer = toks[0];
        out.gate.aa_layer   = toks[1];
      }
      continue;
    }
  }

  if (out.starts.empty()) {
    std::cerr<<"Rule missing StartPos entries\n";
    return false;
  }

  out.needed_layers.clear();
  for (auto& s : out.starts) out.needed_layers.insert(s.first);
  for (auto& vr : out.via_rules) for (auto& ly : vr.layers) out.needed_layers.insert(ly);
  if (out.gate.has_gate) {
    out.needed_layers.insert(out.gate.poly_layer);
    out.needed_layers.insert(out.gate.aa_layer);
  }
  return true;
}

} // namespace tracer
