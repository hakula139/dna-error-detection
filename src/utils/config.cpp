#include "config.h"

Config config{
    .path = "tests/test_2_demo/",
    .ref_filename = "ref.fasta",
    .sv_filename = "sv.fasta",
    .seg_filename = "long.fasta",
    .index_filename = "index.txt",
    .overlaps_filename = "overlaps.txt",
    .deltas_filename = "sv.bed",

    .log_level = Config::Level::DEBUG,
    .hash_size = 20,
    .window_size = 200,
    .chunk_size = 50000,
    .snake_min_len = 30,
    .gap_max_diff = 100,
    .delta_max_len = 1000,
    .strict_equal_rate = 0.3,
    .fuzzy_equal_rate = 0.8,
};
