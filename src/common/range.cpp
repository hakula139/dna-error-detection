#include "range.h"

#include <string>

#include "config.h"
#include "utils.h"

using std::string;
using std::to_string;

string Range::Stringify() const {
  return to_string(start_) + " " + to_string(end_);
}

string Range::Stringify(const string& key) const {
  return (key.length() ? key + " " : "") + Stringify();
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
