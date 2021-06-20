#ifndef SRC_UTILS_CONFIG_H_
#define SRC_UTILS_CONFIG_H_

#include <cstddef>

struct Config {
  enum Level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
  };

  static const char* PATH;
  static const char* REF_FILENAME;
  static const char* SV_FILENAME;
  static const char* SEG_FILENAME;
  static const char* INDEX_FILENAME;
  static const char* OVERLAPS_FILENAME;
  static const char* DELTAS_FILENAME;

  static const int LOG_LEVEL;

  static const size_t HASH_SIZE;
  static const size_t WINDOW_SIZE;
  static const size_t CHUNK_SIZE;
  static const size_t SNAKE_MIN_LEN;
  static const size_t GAP_MIN_DIFF;
  static const size_t GAP_MAX_DIFF;
  static const size_t DELTA_IGNORE_LEN;
  static const size_t DELTA_MIN_LEN;
  static const size_t DELTA_MAX_LEN;
  static const size_t DELTA_MAX_SIZE;
  static const size_t OVERLAP_MIN_COUNT;
  static const size_t OVERLAP_MIN_LEN;
  static const size_t MINIMIZER_MIN_COUNT;
  static const size_t MINIMIZER_MIN_LEN;
  static const double ERROR_MAX_SCORE;
  static const double MYERS_PENALTY;
  static const int DP_PENALTY;
  static const double STRICT_EQUAL_RATE;
  static const double FUZZY_EQUAL_RATE;
};

#endif  // SRC_UTILS_CONFIG_H_
