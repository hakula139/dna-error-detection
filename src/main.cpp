#include <cstdlib>
#include <unordered_map>

#include "config.h"
#include "dna.h"
#include "logger.h"
#include "utils.h"

extern Config config;
extern Logger logger;

using std::unordered_map;

int main(int argc, char** argv) {
  unordered_map<char, bool> arg_flags;
  if (!ReadArgs(&arg_flags, argc, argv)) {
    ShowManual();
    return EXIT_FAILURE;
  }

  Dna ref, sv, segments;

  // Read reference data
  if (!ref.Import(config.path + config.ref_filename)) {
    return EXIT_FAILURE;
  }

  // Create an index of reference data
  if (arg_flags['i']) {
    ref.CreateIndex();
    ref.PrintIndex(config.path + config.index_filename);
  }

  // Merge PacBio subsequences
  if (arg_flags['m']) {
    if (!ref.ImportIndex(config.path + config.index_filename)) {
      return EXIT_FAILURE;
    }
    if (!segments.ImportOverlaps(config.path + config.overlaps_filename)) {
      if (!segments.Import(config.path + config.seg_filename)) {
        return EXIT_FAILURE;
      }
      segments.FindOverlaps(ref);
      segments.PrintOverlaps(config.path + config.overlaps_filename);
    }

    sv.CreateSvChain(ref, segments);
    sv.Print(config.path + config.sv_filename);
  }

  // Main process
  if (arg_flags['s']) {
    if (!sv.size() || !sv.Import(config.path + config.sv_filename)) {
      return EXIT_FAILURE;
    }
    ref.FindDeltas(sv, config.chunk_size);
    ref.ProcessDeltas();
    ref.PrintDeltas(config.path + config.deltas_filename);
  }

  return EXIT_SUCCESS;
}
