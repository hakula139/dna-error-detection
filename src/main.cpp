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

  Dna ref(config.path + config.ref_filename);

  // Create an index of reference data
  if (arg_flags['i']) {
    ref.CreateIndex();
    ref.PrintIndex(config.path + config.index_filename);
  }

  // Combine PacBio subsequences
  if (arg_flags['p']) {
    if (!ref.ImportIndex(config.path + config.index_filename)) {
      return EXIT_FAILURE;
    }
    Dna segments(config.path + config.seg_filename);
    segments.FindOverlaps(ref);
    segments.PrintOverlaps(config.path + config.overlaps_filename);
    Dna sv;
    sv.Print(config.path + config.sv_filename);
  }

  // Main process
  if (arg_flags['s']) {
    Dna sv(config.path + config.sv_filename);
    ref.FindDeltas(sv, config.chunk_size);
    ref.ProcessDeltas();
    ref.PrintDeltas(config.path + config.deltas_filename);
  }

  return EXIT_SUCCESS;
}
