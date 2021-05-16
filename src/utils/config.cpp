#include "config.h"

Config config{
    .log_level = Config::Level::INFO,
    .path = "tests/demo/",
    .tolerance = 50,
    .compare_diff = 100,
    .strict_rate = 0.5,
    .fuzzy_rate = 0.8,
};
