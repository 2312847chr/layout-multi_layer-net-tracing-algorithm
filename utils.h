#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

namespace tracer {

static inline std::string Trim(const std::string& s) {
  size_t b = 0, e = s.size();
  while (b < e && (s[b]==' '||s[b]=='\t'||s[b]=='\r'||s[b]=='\n')) ++b;
  while (e > b && (s[e-1]==' '||s[e-1]=='\t'||s[e-1]=='\r'||s[e-1]=='\n')) --e;
  return s.substr(b, e-b);
}

static inline bool IsLayerTokenChar(char c) {
  return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';
}

static inline bool IsLayerLine(const std::string& s) {
  if (s.empty()) return false;
  for (char c: s) if (!IsLayerTokenChar(c)) return false;
  return true;
}

static inline std::vector<std::string> SplitWS(const std::string& s) {
  std::istringstream iss(s);
  std::vector<std::string> out;
  std::string t;
  while (iss >> t) out.push_back(t);
  return out;
}

} // namespace tracer
