#include "range.h"

#include <algorithm>
#include <cassert>
#include <string>

#include "config.h"
#include "logger.h"
#include "utils.h"

using std::min;
using std::string;
using std::to_string;

size_t Range::size() const {
  if (value_p_) return min(end_, value_p_->size()) - start_;
  return end_ - start_;
}

string Range::get() const {
  assert(value_p_);
  return value_p_->substr(start_, size());
}

string Range::Head(size_t start, size_t size) const {
  return ::Head(get(), start, size);
}

string Range::Stringify() const {
  auto start = to_string(start_);
  auto end = to_string(end_);
  switch (mode_) {
    case REVERSE:
      return end + " " + start;
    case COMPLEMENT:
      return "-" + start + " -" + end;
    case REVR_COMP:
      return "-" + end + " -" + start;
    case NORMAL:
    default:
      return start + " " + end;
  }
}

string Range::Stringify(const string& key) const {
  return (key.length() ? key + " " : "") + Stringify();
}

bool Range::Contains(const Range& that) const {
  return start_ <= that.start_ && end_ >= that.end_;
}

bool Range::operator<(const Range& that) const {
  return end_ < that.end_ || (end_ == that.end_ && start_ < that.start_);
}

bool Range::operator==(const Range& that) const {
  return start_ == that.start_ && end_ == that.end_;
}

bool Range::operator!=(const Range& that) const { return !(*this == that); }

bool FuzzyCompare(const Range& range1, const Range& range2) {
  return FuzzyCompare(range1.size(), range2.size()) &&
         FuzzyOverlap(range1, range2);
}

bool FuzzyOverlap(const Range& range1, const Range& range2) {
  return range1.end_ + Config::GAP_MAX_DIFF >= range2.start_ &&
         range2.end_ + Config::GAP_MAX_DIFF >= range1.start_;
}

bool StrictOverlap(const Range& range1, const Range& range2) {
  return range1.end_ + Config::GAP_MIN_DIFF >= range2.start_ &&
         range2.end_ + Config::GAP_MIN_DIFF >= range1.start_;
}

bool Verify(const Range& range1, const Range& range2) {
  return range1.Head(0, Config::HASH_SIZE) == range2.Head(0, Config::HASH_SIZE);
}
