#include "dna.h"
#include "utils/config.h"

extern Config config;

int main() {
  Dna ref(config.path + "ref.fasta");
  Dna sv(config.path + "sv.fasta");
  ref.FindDeltas(sv, config.chunk_size);
  ref.ProcessDeltas();
  ref.PrintDeltas(config.path + "sv.bed");
  return 0;
}
