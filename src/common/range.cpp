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
  return key.length() ? key + " " + Stringify() : Stringify();
}

bool Range::operator<(const Range& that) const {
  return start_ < that.start_ || (start_ == that.start_ && end_ < that.end_);
}

bool FuzzyCompare(const Range& range1, const Range& range2) {
  return FuzzyCompare(range1.size(), range2.size()) &&
         range1.end_ + Config::GAP_MAX_DIFF >= range2.start_ &&
         range2.end_ + Config::GAP_MAX_DIFF >= range1.start_;
}
