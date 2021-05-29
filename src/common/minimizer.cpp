#include "minimizer.h"

#include <string>

using std::string;

string Minimizer::Stringify() const {
  return range_ref_.Stringify() + " " + range_seg_.Stringify(key_seg_);
}

bool Minimizer::operator<(const Minimizer& that) const {
  return range_ref_ < that.range_ref_;
}
