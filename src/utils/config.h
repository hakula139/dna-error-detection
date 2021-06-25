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

  // File input / Output

  static const char* PATH;
  static const char* REF_FILENAME;
  static const char* SV_FILENAME;
  static const char* SEG_FILENAME;
  static const char* INDEX_FILENAME;
  static const char* OVERLAPS_FILENAME;
  static const char* DELTAS_FILENAME;

  // Logging

  static const char* LOG_PATH;
  static const char* LOG_FILENAME;
  static const char* ERROR_LOG_FILENAME;
  static const Level LOG_LEVEL;
  static const size_t DISPLAY_SIZE;

  // Indexing

  static const size_t HASH_SIZE;
  static const size_t WINDOW_SIZE;
  static const size_t CHUNK_SIZE;

  // Finding minimizers

  static const size_t OVERLAP_MIN_COUNT;
  static const size_t MINIMIZER_MIN_COUNT;
  static const size_t MINIMIZER_MIN_LEN;
  static const size_t MINIMIZER_MAX_DIFF;

  // Finding deltas

  static const size_t DENSITY_WINDOW_SIZE;
  static const double NOISE_RATE;
  static const size_t DELTA_IGNORE_LEN;
  static const size_t DELTA_MIN_LEN;
  static const size_t DELTA_MAX_LEN;
  static const size_t PADDING_SIZE;
  static const size_t SNAKE_MIN_LEN;
  static const int DP_PENALTY;
  static const double MYERS_PENALTY;
  static const double ERROR_MAX_SCORE;

  // Utilities

  static const size_t GAP_MIN_DIFF;
  static const size_t GAP_MAX_DIFF;
  static const size_t OVERLAP_MIN_LEN;
  static const double STRICT_EQUAL_RATE;
  static const double FUZZY_EQUAL_RATE;
  static const double UNKNOWN_RATE;
};

#endif  // SRC_UTILS_CONFIG_H_
