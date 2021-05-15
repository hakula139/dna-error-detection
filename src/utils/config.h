#ifndef SRC_UTILS_CONFIG_H_
#define SRC_UTILS_CONFIG_H_

#include <string>

struct Config {
  enum Level {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4,
  };

  int log_level;
  std::string path;
  size_t tolerance;
  double fuzzy_match_rate;
};

#endif  // SRC_UTILS_CONFIG_H_
