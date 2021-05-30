#include "dna_overlap.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "minimizer.h"
#include "range.h"
#include "utils.h"

using std::greater;
using std::max;
using std::min;
using std::ofstream;
using std::priority_queue;
using std::string;
using std::tuple;
using std::unordered_map;
using std::vector;

size_t DnaOverlap::size() const {
  size_t size = 0;
  for (const auto& [key_ref, entries] : data_) {
    size += entries.size();
  }
  return size;
}

void DnaOverlap::Insert(const string& key_ref, const Minimizer& entry) {
  data_[key_ref].emplace(entry);
}

void DnaOverlap::Merge() {
  for (auto&& [key_ref, entries] : data_) {
    unordered_map<string, tuple<Range, Range, size_t>> merged_overlaps;
    auto merge = [](Range& base, const Range& range) {
      if (base == Range()) {
        base = range;
      } else if (FuzzyOverlap(base, range)) {
        base.start_ = min(base.start_, range.start_);
        base.end_ = max(base.end_, range.end_);
      }
    };
    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      auto&& [merged_ref, merged_seg, count] = merged_overlaps[key_seg];
      merge(merged_ref, range_ref);
      merge(merged_seg, range_seg);
      ++count;
    }

    entries.clear();
    for (const auto& [key_seg, entry] : merged_overlaps) {
      const auto& [merged_ref, merged_seg, count] = entry;
      if (count >= Config::MINIMIZER_MIN_COUNT &&
          merged_ref.size() >= Config::OVERLAP_MIN_LEN &&
          merged_seg.size() >= Config::OVERLAP_MIN_LEN) {
        entries.emplace(merged_ref, key_seg, merged_seg);
      }
    }
  }
}

void DnaOverlap::Print(ofstream& out_file) const {
  for (const auto& [key_ref, entries] : data_) {
    for (const auto& entry : entries) {
      out_file << entry.Stringify() << "\n";
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
