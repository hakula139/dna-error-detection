#include "dna_delta.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "config.h"
#include "logger.h"
#include "minimizer.h"
#include "range.h"
#include "utils.h"

using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::prev;
using std::string;

void DnaDelta::Print(ofstream& out_file) const {
  for (const auto& [key_ref, deltas] : data_) {
    for (const auto& [range_ref, key_seg, range_seg] : deltas) {
      out_file << type_ << " " << range_ref.Stringify(key_ref) << "\n";
    }
  }
}

void DnaDelta::Set(const string& key, const Minimizer& value) {
  auto& deltas = data_[key];
  auto exist = [&](const Minimizer& delta) {
    for (auto&& prev : deltas) {
      if (Combine(&prev, &delta)) return true;
    }
    return false;
  };
  if (!deltas.size() || !exist(value)) {
    deltas.emplace_back(value);
  }
  Logger::Debug(
      "DnaDelta::Set",
      "Saved: " + type_ + " " + value.range_ref_.Stringify(key));
}

bool DnaDelta::Combine(Minimizer* base_p, const Minimizer* value_p) const {
  auto&& [base_range_ref, base_key_seg, base_range_seg] = *base_p;
  const auto& [range_ref, key_seg, range_seg] = *value_p;

  if (base_range_ref.value_p_ == base_range_seg.value_p_ &&
      FuzzyOverlap(base_range_ref, range_ref)) {
    auto new_ref_start = min(base_range_ref.start_, range_ref.start_);
    auto new_ref_end = max(base_range_ref.end_, range_ref.end_);
    if (new_ref_end > new_ref_start + Config::DELTA_MAX_LEN) return false;
    auto new_seg_start = min(base_range_seg.start_, range_seg.start_);
    auto new_seg_end = max(base_range_seg.end_, range_seg.end_);

    *base_p = {
        {new_ref_start, new_ref_end, base_range_ref.value_p_},
        base_key_seg,
        {new_seg_start, new_seg_end, base_range_seg.value_p_},
    };
    return true;
  }
  return false;
}

void DnaMultiDelta::Print(ofstream& out_file) const {
  for (const auto& [key, ranges] : data_) {
    for (const auto& range : ranges) {
      out_file << type_ << " " << range.first.Stringify(key.first) << " "
               << range.second.Stringify(key.second) << "\n";
    }
  }
}

void DnaMultiDelta::Set(
    const string& key1,
    const Range& range1,
    const string& key2,
    const Range& range2) {
  auto save_range = [&](const string& key1,
                        const Range& range1,
                        const string& key2,
                        const Range& range2) {
    auto& ranges = data_[{key1, key2}];
    auto range = pair{range1, range2};
    auto exist = [&](const pair<Range, Range>& range) {
      for (auto&& prev : ranges) {
        if (Combine(&prev, &range)) return true;
      }
      return false;
    };
    if (!ranges.size() || !exist(range)) {
      ranges.emplace_back(range);
      Logger::Debug(
          "DnaDelta::Set",
          "Saved: " + type_ + " " + range1.Stringify(key1) + " " +
              range2.Stringify(key2));
    }
  };
  if (data_.count({key2, key1})) {
    save_range(key2, range2, key1, range1);
  } else {
    save_range(key1, range1, key2, range2);
  }
}

bool DnaMultiDelta::Combine(
    pair<Range, Range>* base_p, const pair<Range, Range>* range_p) const {
  return FuzzyCompare(base_p->first.start_, range_p->first.start_) &&
         FuzzyCompare(base_p->second.start_, range_p->second.start_);
}
