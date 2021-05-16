#ifndef SRC_UTILS_CONFIG_H_
#define SRC_UTILS_CONFIG_H_

#include <string>

struct Config {
  enum Level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
  };

  int log_level;
  std::string path;
  size_t tolerance;
  size_t compare_diff;
  double strict_rate;
  double fuzzy_rate;
};

#endif  // SRC_UTILS_CONFIG_H_
