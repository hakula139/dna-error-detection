#ifndef SRC_RANGE_H_
#define SRC_RANGE_H_

#include <string>

struct Range {
  Range() {}
  Range(size_t start, size_t end, std::string value)
      : start_(start), end_(end), value_(value) {}

  size_t start_ = 0;
  size_t end_ = 0;
  std::string value_ = "";
};

#endif  // SRC_RANGE_H_
