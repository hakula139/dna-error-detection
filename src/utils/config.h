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

  inline static const std::string PATH = "tests/test_2_demo/";
  inline static const std::string REF_FILENAME = "ref.fasta";
  inline static const std::string SV_FILENAME = "sv.fasta";
  inline static const std::string SEG_FILENAME = "long.fasta";
  inline static const std::string INDEX_FILENAME = "index.txt";
  inline static const std::string OVERLAPS_FILENAME = "overlaps.txt";
  inline static const std::string DELTAS_FILENAME = "sv.bed";

  inline static const int LOG_LEVEL = Config::Level::DEBUG;
  inline static const size_t HASH_SIZE = 15;
  inline static const size_t WINDOW_SIZE = 10;
  inline static const size_t CHUNK_SIZE = 20000;
  inline static const size_t SNAKE_MIN_LEN = 10;
  inline static const size_t ERROR_MAX_LEN = 0;
  inline static const size_t GAP_MAX_DIFF = 100;
  inline static const size_t DELTA_MAX_LEN = 1000;
  inline static const size_t OVERLAP_MIN_LEN = 30;
  inline static const size_t MINIMIZER_MIN_COUNT = 10;
  inline static const size_t MINIMIZER_MIN_LEN = 150;
  inline static const int DP_PENALTY = 1;
  inline static const double STRICT_EQUAL_RATE = 0.5;
  inline static const double FUZZY_EQUAL_RATE = 0.8;
};

#endif  // SRC_UTILS_CONFIG_H_
