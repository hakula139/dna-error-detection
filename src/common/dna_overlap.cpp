#include "dna_overlap.h"

#include <algorithm>
#include <utility>

using std::get;
using std::sort;

void DnaOverlap::Sort() {
  auto compare = [](const Minimizer& m1, const Minimizer& m2) {
    return get<1>(m1) < get<1>(m2);
  };
  sort(data_.begin(), data_.end(), compare);
}

DnaOverlap& DnaOverlap::operator+=(const DnaOverlap& that) {
  data_.reserve(data_.size() + that.data_.size());
  data_.insert(data_.end(), that.data_.begin(), that.data_.end());
  return *this;
}

DnaOverlap& DnaOverlap::operator+=(const Minimizer& entry) {
  data_.push_back(entry);
  return *this;
}

DnaOverlap& DnaOverlap::operator=(const DnaOverlap& that) {
  data_ = that.data_;
  return *this;
}
