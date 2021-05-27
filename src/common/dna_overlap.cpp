#include "dna_overlap.h"

#include <algorithm>
#include <fstream>
#include <utility>

using std::get;
using std::ofstream;
using std::sort;

void DnaOverlap::Sort() {
  auto compare = [](const Minimizer& m1, const Minimizer& m2) {
    return get<1>(m1) < get<1>(m2);
  };
  sort(data_.begin(), data_.end(), compare);
}

void DnaOverlap::Print(ofstream& out_file) const {
  for (const auto& [key_ref, range_ref, key_seg, range_seg] : data_) {
    out_file << range_ref.Stringify(key_ref) << " "
             << range_seg.Stringify(key_seg) << "\n";
  }
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
