#include "range.h"

#include <algorithm>
#include <string>

#include "config.h"
#include "logger.h"
#include "utils.h"

using std::min;
using std::string;
using std::to_string;

string Range::get() const {
  auto size = min(this->size(), value_p_->size() - start_);
  return value_p_ ? value_p_->substr(start_, size) : "";
}

string Range::Head(size_t start, size_t size) const {
  return ::Head(get(), start, size);
}

string Range::Stringify() const {
  if (inverted_) {
    return to_string(end_) + " " + to_string(start_);
  } else {
    return to_string(start_) + " " + to_string(end_);
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
