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

  std::string path;
  std::string ref_filename;
  std::string sv_filename;
  std::string seg_filename;
  std::string index_filename;
  std::string overlaps_filename;
  std::string deltas_filename;

  int log_level;
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
