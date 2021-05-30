#ifndef TESTS_UNIT_TEST_H_
#define TESTS_UNIT_TEST_H_

#include <string>

class Test {
 public:
  template <class T>
  static void Expect(
      const std::string& context, const T& expected, const T& got);

  static void HashTest();
};

#endif  // TESTS_UNIT_TEST_H_
