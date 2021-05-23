#include "range.h"

#include <string>

#include "config.h"
#include "utils.h"

extern Config config;

using std::string;
using std::to_string;

string Range::Stringify() const {
  return to_string(start_) + " " + to_string(end_);
}

string Range::Stringify(const string& key) const {
  return key.length() ? key + " " + Stringify() : Stringify();
}

bool FuzzyCompare(const Range& range1, const Range& range2) {
  return FuzzyCompare(range1.size(), range2.size()) &&
         range1.end_ + config.compare_diff >= range2.start_ &&
         range2.end_ + config.compare_diff >= range1.start_;
}
