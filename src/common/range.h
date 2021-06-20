#ifndef SRC_COMMON_RANGE_H_
#define SRC_COMMON_RANGE_H_

#include <string>

struct Range {
  Range() {}
  Range(
      size_t start,
      size_t end,
      const std::string* value_p,
      bool inverted = false)
      : start_(start), end_(end), value_p_(value_p), inverted_(inverted) {}

  size_t size() const { return end_ - start_; }
  std::string get() const;
  std::string Stringify() const;
  std::string Stringify(const std::string& key) const;

  bool operator<(const Range& that) const;
  bool operator==(const Range& that) const;
  bool operator!=(const Range& that) const;

  size_t start_ = 0;
  size_t end_ = 0;
  const std::string* value_p_ = nullptr;
  bool inverted_ = false;
};

bool FuzzyCompare(const Range& range1, const Range& range2);
bool FuzzyOverlap(const Range& range1, const Range& range2);
bool StrictOverlap(const Range& range1, const Range& range2);

#endif  // SRC_COMMON_RANGE_H_
