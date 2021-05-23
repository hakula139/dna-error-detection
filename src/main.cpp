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

  Dna ref(config.path + "ref.fasta");

  // Create an index of reference data
  if (arg_flags['i']) {
    ref.CreateIndex();
    ref.PrintIndex(config.path + "index.txt");
  }

  // Combine PacBio subsequences
  if (arg_flags['p']) {
    if (!ref.ImportIndex(config.path + "index.txt")) {
      return EXIT_FAILURE;
    }
    Dna segments(config.path + "long.fasta");
    segments.FindOverlaps(ref);
    Dna sv;
    sv.Print(config.path + "sv.fasta");
  }

  // Main process
  if (arg_flags['s']) {
    Dna sv(config.path + "sv.fasta");
    ref.FindDeltas(sv, config.chunk_size);
    ref.ProcessDeltas();
    ref.PrintDeltas(config.path + "sv.bed");
  }

  return EXIT_SUCCESS;
}
