#include <cstdlib>
#include <unordered_map>

#include "config.h"
#include "dna.h"
#include "utils.h"

extern Config config;

using std::unordered_map;

int main(int argc, char** argv) {
  unordered_map<char, bool> arg_flags;
  if (!ReadArgs(&arg_flags, argc, argv)) {
    ShowManual();
    return EXIT_FAILURE;
  }

  Dna ref(config.path + "ref.fasta");
  if (arg_flags['i']) {
    // Create an index of reference data
    ref.CreateIndex();
  }
  if (arg_flags['p']) {
    // Combine PacBio subsequences
    Dna reads(config.path + "long.fasta");
  }
  if (arg_flags['s']) {
    // Main process
    Dna sv(config.path + "sv.fasta");
    ref.FindDeltas(sv, config.chunk_size);
    ref.ProcessDeltas();
    ref.PrintDeltas(config.path + "sv.bed");
  }
  return EXIT_SUCCESS;
}
