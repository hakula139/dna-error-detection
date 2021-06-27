#include "config.h"

// File input / Output

const char* Config::PATH = "tests/test_2_demo";
const char* Config::REF_FILENAME = "ref.fasta";
const char* Config::SV_FILENAME = "sv.fasta";
const char* Config::SEG_FILENAME = "long.fasta";
const char* Config::INDEX_FILENAME = "index.txt";
const char* Config::OVERLAPS_FILENAME = "overlaps.txt";
const char* Config::DELTAS_FILENAME = "sv.bed";

// Logging

const char* Config::LOG_PATH = "logs";
const char* Config::LOG_FILENAME = "output.log";
const char* Config::ERROR_LOG_FILENAME = "error.log";
const Config::Level Config::LOG_LEVEL = Config::Level::DEBUG;
const size_t Config::DISPLAY_SIZE = 100;

// Indexing

const size_t Config::HASH_SIZE = 15;
const size_t Config::WINDOW_SIZE = 10;
const size_t Config::CHUNK_SIZE = 50000;

// Finding minimizers

const size_t Config::OVERLAP_MIN_COUNT = 30;
const size_t Config::MINIMIZER_MIN_COUNT = 4;
const size_t Config::MINIMIZER_MIN_LEN = 500;
const size_t Config::MINIMIZER_MAX_DIFF = 1200;

// Finding deltas

const size_t Config::DENSITY_WINDOW_SIZE = 40;
const double Config::NOISE_RATE = 0.10;
const double Config::SIGNAL_RATE = 0.55;
const size_t Config::DELTA_IGNORE_LEN = 1;
const size_t Config::DELTA_MIN_LEN = 100;
const size_t Config::DELTA_MAX_LEN = 1000;
const size_t Config::PADDING_SIZE = 60000;
const size_t Config::SNAKE_MIN_LEN = 3;
const int Config::DP_PENALTY = 2;
const double Config::MYERS_PENALTY = 0.25;
const double Config::ERROR_MAX_SCORE = 0.0;

// Utilities

const size_t Config::GAP_MIN_DIFF = 1;
const size_t Config::GAP_MAX_DIFF = 30;
const size_t Config::OVERLAP_MIN_LEN = 30;
const double Config::STRICT_EQUAL_RATE = 0.4;
const double Config::FUZZY_EQUAL_RATE = 0.6;
const double Config::UNKNOWN_RATE = 0.1;
