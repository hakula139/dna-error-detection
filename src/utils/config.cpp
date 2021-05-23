#include "config.h"

Config config{
    .log_level = Config::Level::INFO,
    .path = "tests/test_2_demo/",
    .hash_size = 30,
    .window_size = 300,
    .chunk_size = 50000,
    .snake_min_len = 30,
    .gap_max_diff = 100,
    .delta_max_len = 1000,
    .strict_equal_rate = 0.5,
    .fuzzy_equal_rate = 0.8,
};
