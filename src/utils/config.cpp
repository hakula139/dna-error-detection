#include "config.h"

Config config{
    .log_level = Config::Level::INFO,
    .path = "tests/demo/",
    .tolerance = 20,
    .min_length = 50,
    .strict_rate = 0.5,
    .fuzzy_rate = 0.8,
    .cover_rate = 0.9,
};
