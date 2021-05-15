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
  size_t min_length;
  double fuzzy_rate;
  double cover_rate;
};

#endif  // SRC_UTILS_CONFIG_H_
