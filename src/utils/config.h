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

  static constexpr char* PATH = "tests/test_2_demo";
  static constexpr char* REF_FILENAME = "ref.fasta";
  static constexpr char* SV_FILENAME = "sv.fasta";
  static constexpr char* SEG_FILENAME = "long.fasta";
  static constexpr char* INDEX_FILENAME = "index.txt";
  static constexpr char* OVERLAPS_FILENAME = "overlaps.txt";
  static constexpr char* DELTAS_FILENAME = "sv.bed";

  static constexpr int LOG_LEVEL = Config::Level::DEBUG;
  static constexpr size_t HASH_SIZE = 15;
  static constexpr size_t WINDOW_SIZE = 10;
  static constexpr size_t CHUNK_SIZE = 50000;
  static constexpr size_t SNAKE_MIN_LEN = 30;
  static constexpr size_t ERROR_MAX_LEN = 4;
  static constexpr size_t GAP_MIN_DIFF = 2;
  static constexpr size_t GAP_MAX_DIFF = 50;
  static constexpr size_t DELTA_MIN_LEN = 100;
  static constexpr size_t DELTA_MAX_LEN = 1000;
  static constexpr size_t DELTA_MAX_SIZE = 1000;
  static constexpr size_t OVERLAP_MIN_COUNT = 30;
  static constexpr size_t OVERLAP_MIN_LEN = 30;
  static constexpr size_t MINIMIZER_MIN_COUNT = 4;
  static constexpr size_t MINIMIZER_MIN_LEN = 100;
  static constexpr double MYERS_PENALTY = 0.25;
  static constexpr int DP_PENALTY = 2;
  static constexpr double STRICT_EQUAL_RATE = 0.5;
  static constexpr double FUZZY_EQUAL_RATE = 0.8;
};

#endif  // SRC_UTILS_CONFIG_H_
