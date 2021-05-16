#include "range.h"

#include "utils/config.h"
#include "utils/utils.h"

extern Config config;

bool FuzzyCompare(const Range& range1, const Range& range2) {
  return FuzzyCompare(range1.size(), range2.size()) &&
         range1.end_ + config.compare_diff >= range2.start_ &&
         range2.end_ + config.compare_diff >= range1.start_;
}
