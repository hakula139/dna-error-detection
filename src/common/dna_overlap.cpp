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
#include "logger.h"
#include "minimizer.h"
#include "range.h"
#include "utils.h"

using std::greater;
using std::max;
using std::min;
using std::ofstream;
using std::priority_queue;
using std::string;
using std::to_string;
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
  auto merge = [](Range& base, const Range& range) {
    base.start_ = min(base.start_, range.start_);
    base.end_ = max(base.end_, range.end_);
  };

  for (auto&& [key_ref, entries] : data_) {
    unordered_map<string, vector<tuple<Range, Range, size_t>>> merged_overlaps;

    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      auto key = (range_seg.inverted_ ? "-" : "+") + key_seg;
      auto&& merged_overlaps_seg = merged_overlaps[key];

      auto merged = false;
      for (auto&& [merged_ref, merged_seg, count] : merged_overlaps_seg) {
        if (FuzzyOverlap(merged_ref, range_ref)) {
          merge(merged_ref, range_ref);
          merge(merged_seg, range_seg);
          merged = true;
          ++count;
          break;
        }
      }

      if (!merged) {
        merged_overlaps_seg.emplace_back(range_ref, range_seg, 1);
      }
    }

    entries.clear();
    for (const auto& [key_seg, entry] : merged_overlaps) {
      for (const auto& [merged_ref, merged_seg, count] : entry) {
        auto used = merged_ref.size() >= Config::MINIMIZER_MIN_LEN &&
                    merged_seg.size() >= Config::MINIMIZER_MIN_LEN;

        if (used) entries.emplace(merged_ref, key_seg.substr(1), merged_seg);

        Logger::Trace("DnaOverlap::Merge", key_ref + ": \tMinimizer:");
        Logger::Trace(
            "Minimizer count",
            to_string(count) + (used ? " \tused" : " \tnot used"));
        Logger::Trace("", "REF: \t" + merged_ref.get());
        Logger::Trace("", "SEG: \t" + merged_seg.get());
      }
    }
  }
}

void DnaOverlap::Print(ofstream& out_file) const {
  for (const auto& [key_ref, entries] : data_) {
    for (const auto& entry : entries) {
      out_file << entry.Stringify(key_ref) << "\n";
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
