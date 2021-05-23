#include "config.h"

Config config{
    .log_level = Config::Level::INFO,
    .path = "tests/test_1/",
    .chunk_size = 50000,
    .tolerance = 30,
    .compare_diff = 100,
    .max_length = 1000,
    .strict_rate = 0.5,
    .fuzzy_rate = 0.8,
};
