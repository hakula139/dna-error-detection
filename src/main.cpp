#include <cstdlib>
#include <iostream>
#include <unordered_map>

#include "config.h"
#include "dna.h"
#include "logger.h"
#include "utils.h"

using std::ios;
using std::unordered_map;

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);

  unordered_map<char, bool> arg_flags;
  if (!ReadArgs(&arg_flags, argc, argv)) {
    ShowManual();
    return EXIT_FAILURE;
  }

  Dna ref, sv, segments;

  // Read data
  if (!ref.Import(Config::PATH + Config::REF_FILENAME)) {
    return EXIT_FAILURE;
  }

  // Create an index of reference data
  if (arg_flags['i']) {
    ref.CreateIndex();
    ref.PrintIndex(Config::PATH + Config::INDEX_FILENAME);
  }

  // Merge PacBio subsequences
  if (arg_flags['m']) {
    if (!ref.ImportIndex(Config::PATH + Config::INDEX_FILENAME)) {
      return EXIT_FAILURE;
    }
    if (!segments.Import(Config::PATH + Config::SEG_FILENAME)) {
      return EXIT_FAILURE;
    }
    segments.FindOverlaps(ref);
    segments.PrintOverlaps(Config::PATH + Config::OVERLAPS_FILENAME);
  }

  // Main process
  if (arg_flags['s']) {
    if (!sv.size() && !sv.Import(Config::PATH + Config::SV_FILENAME)) {
      if (!segments.size() &&
          !segments.Import(Config::PATH + Config::SEG_FILENAME)) {
        return EXIT_FAILURE;
      }
      if (!ref.ImportOverlaps(
              &segments, Config::PATH + Config::OVERLAPS_FILENAME)) {
        return EXIT_FAILURE;
      }
      ref.FindDeltasFromSegments();
    } else {
      ref.FindDeltas(sv, Config::CHUNK_SIZE);
    }
    ref.ProcessDeltas();
    ref.PrintDeltas(Config::PATH + Config::DELTAS_FILENAME);
  }

  return EXIT_SUCCESS;
}
