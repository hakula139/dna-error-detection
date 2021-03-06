#ifndef SRC_COMMON_RANGE_H_
#define SRC_COMMON_RANGE_H_

#include <string>

#include "config.h"

enum Mode {
  NORMAL,
  REVERSE,
  COMPLEMENT,
  REVR_COMP,
};

struct Range {
  Range() {}
  Range(
      size_t start,
      size_t end,
      const std::string* value_p,
      Mode mode = NORMAL,
      bool unknown = false)
      : start_(start),
        end_(end),
        value_p_(value_p),
        mode_(mode),
        unknown_(unknown) {}

  operator bool() const { return start_ || end_; }

  size_t size() const;
  std::string get() const;
  std::string Head(size_t start = 0, size_t size = Config::DISPLAY_SIZE) const;
  std::string Stringify() const;
  std::string Stringify(const std::string& key) const;
  bool Contains(const Range& that) const;

  bool operator<(const Range& that) const;
  bool operator==(const Range& that) const;
  bool operator!=(const Range& that) const;

  size_t start_ = 0;
  size_t end_ = 0;
  const std::string* value_p_ = nullptr;
  Mode mode_ = NORMAL;
  bool unknown_ = false;
};

bool FuzzyCompare(const Range& range1, const Range& range2);
bool FuzzyOverlap(const Range& range1, const Range& range2);
bool StrictOverlap(const Range& range1, const Range& range2);

bool Verify(const Range& range1, const Range& range2);

#endif  // SRC_COMMON_RANGE_H_
