#include "range.h"

#include <algorithm>
#include <cmath>

#include "utils/config.h"

using std::abs;
using std::max;

extern Config config;

bool QuickCompare(const Range& range1, const Range& range2) {
  int len1 = range1.size();
  int len2 = range2.size();
  return abs(len1 - len2) < max(len1, len2) * (1 - config.fuzzy_match_rate);
}
