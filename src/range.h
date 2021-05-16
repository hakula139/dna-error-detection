#ifndef SRC_RANGE_H_
#define SRC_RANGE_H_

#include <string>

struct Range {
  Range() {}
  Range(size_t start, size_t end, std::string value)
      : start_(start), end_(end), value_(value) {}
  size_t size() const { return end_ - start_; }

  size_t start_ = 0;
  size_t end_ = 0;
  std::string value_ = "";
};

bool FuzzyCompare(const Range& range1, const Range& range2);

#endif  // SRC_RANGE_H_
