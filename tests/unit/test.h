#ifndef TESTS_UNIT_TEST_H_
#define TESTS_UNIT_TEST_H_

#include <cstdlib>
#include <string>

#include "logger.h"

class Test {
 public:
  template <class T>
  static void Expect(
      const std::string& context, const T& expected, const T& got) {
    if (expected != got) {
      Logger::Error(
          context,
          "expected " + std::to_string(expected) + ", got " +
              std::to_string(got));
      exit(EXIT_SUCCESS);
    }
  }

  static void HashTest();
};

#endif  // TESTS_UNIT_TEST_H_
