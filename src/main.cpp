#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <unordered_map>

#include "config.h"
#include "dna.h"
#include "logger.h"
#include "utils.h"

namespace fs = std::filesystem;

using std::ios;
using std::unordered_map;

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);

  unordered_map<char, bool> arg_flags;
  if (!ReadArgs(&arg_flags, argc, argv)) {
    ShowManual();
    return EXIT_FAILURE;
  }

  fs::path path(Config::PATH);
  fs::path ref_filename(Config::REF_FILENAME);
  fs::path sv_filename(Config::SV_FILENAME);
  fs::path seg_filename(Config::SEG_FILENAME);
  fs::path index_filename(Config::INDEX_FILENAME);
  fs::path overlaps_filename(Config::OVERLAPS_FILENAME);
  fs::path deltas_filename(Config::DELTAS_FILENAME);

  Dna ref, sv, segments;

  // Read data
  if (!ref.Import(path / ref_filename)) {
    return EXIT_FAILURE;
  }

  // Create an index of reference data
  if (arg_flags['i']) {
    ref.CreateIndex();
    ref.PrintIndex(path / index_filename);
  }

  // Merge PacBio subsequences
  if (arg_flags['m']) {
    if (!ref.ImportIndex(path / index_filename)) {
      return EXIT_FAILURE;
    }
    if (!segments.Import(path / seg_filename)) {
      return EXIT_FAILURE;
    }
    segments.FindOverlaps(ref);
    segments.PrintOverlaps(path / overlaps_filename);
  }

  // Main process
  if (arg_flags['s']) {
    if (!sv.size() && !sv.Import(path / sv_filename)) {
      if (!segments.size() && !segments.Import(path / seg_filename)) {
        return EXIT_FAILURE;
      }
      if (!ref.ImportOverlaps(&segments, path / overlaps_filename)) {
        return EXIT_FAILURE;
      }
      ref.FindDeltasFromSegments();
    } else {
      ref.FindDeltas(sv, Config::CHUNK_SIZE);
    }
    ref.ProcessDeltas();
    ref.PrintDeltas(path / deltas_filename);
  }

  return EXIT_SUCCESS;
}
