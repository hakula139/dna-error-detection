#include <string>
#include <utility>
#include <vector>

#include "dna.h"
#include "logger.h"
#include "test.h"

using std::pair;
using std::string;
using std::to_string;
using std::vector;

void Test::HashTest() {
  auto tests = vector<pair<string, uint64_t>>{
      {"TACGGTGCGCACCGG", 318224559},
      {"ACGGCCGACCATTCG", 199960667},
      {"CCAGACGGCCGACCA", 684452648},
      {"ATCGGGGACGGCATA", 117387140},
      {"AACACGACCCCATGG", 36481567},
  };

  for (const auto& [str, expected] : tests) {
    uint64_t hash = 0;
    for (auto base : str) {
      hash = Dna::NextHash(hash, base);
    }
    Test::Expect(__func__, expected, hash);
  }

  Logger::Info(__func__, "Passed");
}
