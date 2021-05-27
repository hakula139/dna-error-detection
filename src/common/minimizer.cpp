#include "minimizer.h"

bool Minimizer::operator<(const Minimizer& that) const {
  return range_ref_ < that.range_ref_;
}
