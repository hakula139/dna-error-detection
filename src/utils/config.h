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
  size_t hash_size;
  size_t window_size;
  size_t chunk_size;
  size_t snake_min_len;
  size_t gap_max_diff;
  size_t delta_max_len;
  double strict_equal_rate;
  double fuzzy_equal_rate;
};

#endif  // SRC_UTILS_CONFIG_H_
