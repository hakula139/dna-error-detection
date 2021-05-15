#include "config.h"

Config config{
    .log_level = Config::Level::INFO,
    .path = "tests/demo/",
    .tolerance = 20,
    .fuzzy_rate = 0.8,
};
