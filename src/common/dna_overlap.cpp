#include "dna_overlap.h"

#include <fstream>
#include <string>

using std::ofstream;
using std::string;

void DnaOverlap::Insert(const string& key_ref, const Minimizer& entry) {
  data_[key_ref].insert(entry);
}

void DnaOverlap::Print(ofstream& out_file) const {
  for (const auto& [key_ref, entries] : data_) {
    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      out_file << range_ref.Stringify(key_ref) << " "
               << range_seg.Stringify(key_seg) << "\n";
    }
  }
}

DnaOverlap& DnaOverlap::operator+=(const DnaOverlap& that) {
  for (const auto& [key_ref, entries] : that.data_) {
    for (const auto& entry : entries) {
      Insert(key_ref, entry);
    }
  }
  return *this;
}
