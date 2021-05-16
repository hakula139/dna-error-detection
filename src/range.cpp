#include "range.h"

#include "utils/utils.h"

bool FuzzyCompare(const Range& range1, const Range& range2) {
  return QuickCompare(range1, range2) &&
         (FuzzyCompare(range1.start_, range2.start_) ||
          FuzzyCompare(range1.end_, range2.start_) ||
          FuzzyCompare(range1.start_, range2.end_));
}

bool QuickCompare(const Range& range1, const Range& range2) {
  return FuzzyCompare(range1.size(), range2.size());
}
