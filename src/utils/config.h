#ifndef SRC_UTILS_CONFIG_H_
#define SRC_UTILS_CONFIG_H_

#include <string>

struct Config {
  bool debug;
  std::string path;
  size_t tolerance;
  double fuzzy_match_rate;
};

#endif  // SRC_UTILS_CONFIG_H_
