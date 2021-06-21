#include "config.h"

const char* Config::PATH = "tests/test_2_demo";
const char* Config::REF_FILENAME = "ref.fasta";
const char* Config::SV_FILENAME = "sv.fasta";
const char* Config::SEG_FILENAME = "long.fasta";
const char* Config::INDEX_FILENAME = "index.txt";
const char* Config::OVERLAPS_FILENAME = "overlaps.txt";
const char* Config::DELTAS_FILENAME = "sv.bed";

const int Config::LOG_LEVEL = Config::Level::DEBUG;

const size_t Config::HASH_SIZE = 15;
const size_t Config::WINDOW_SIZE = 10;
const size_t Config::CHUNK_SIZE = 50000;
const size_t Config::SNAKE_MIN_LEN = 0;
const size_t Config::GAP_MIN_DIFF = 10;
const size_t Config::GAP_MAX_DIFF = 50;
const size_t Config::DELTA_IGNORE_LEN = 0;
const size_t Config::DELTA_MIN_LEN = 50;
const size_t Config::DELTA_MAX_LEN = 1000;
const size_t Config::OVERLAP_MIN_COUNT = 30;
const size_t Config::OVERLAP_MIN_LEN = 30;
const size_t Config::MINIMIZER_MIN_COUNT = 5;
const size_t Config::MINIMIZER_MIN_LEN = 100;
const double Config::ERROR_MAX_SCORE = 3.0;
const double Config::MYERS_PENALTY = 0.25;
const int Config::DP_PENALTY = 2;
const double Config::STRICT_EQUAL_RATE = 0.5;
const double Config::FUZZY_EQUAL_RATE = 0.8;
