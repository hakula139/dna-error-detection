#include "range.h"

#include "utils/utils.h"

bool QuickCompare(const Range& range1, const Range& range2) {
  return QuickCompare(range1.size(), range2.size());
}
