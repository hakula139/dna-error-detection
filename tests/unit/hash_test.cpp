#include <cstdlib>
#include <iostream>
#include <string>

#include "dna.h"
#include "logger.h"
#include "test.h"

using std::cerr;
using std::string;
using std::to_string;

template <class T>
void Test::Expect(const string& context, const T& expected, const T& got) {
  if (expected != got) {
    Logger::Error(
        context, "expected " + to_string(expected) + ", got " + to_string(got));
    exit(EXIT_SUCCESS);
  }
}

void Test::HashTest() {
  string str = "GCTANATCG";
  uint64_t hash = 0;
  for (auto base : str) {
    hash = Dna::NextHash(hash, base);
  }

  uint64_t expected = 233499;
  Test::Expect(__func__, expected, hash);

  Logger::Info(__func__, "Passed");
}
