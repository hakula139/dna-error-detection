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

using std::count;
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

  auto delta_str = [&](const Minimizer& delta) {
    return type_ + " " + delta.range_ref_.Stringify(key);
  };

  auto exist = [&](const Minimizer& delta) {
    for (auto prev_i = deltas.rbegin(); prev_i < deltas.rend(); ++prev_i) {
      // if (prev_i->key_seg_ != delta.key_seg_) break;
      if (Combine(&*prev_i, &delta)) {
        Logger::Debug("DnaDelta::Set", "Merged:  \t" + delta_str(*prev_i));
        return true;
      }
    }
    return false;
  };

  if (value.range_ref_.size() > Config::DELTA_IGNORE_LEN) {
    if (deltas.empty() || !exist(value)) {
      deltas.emplace_back(value);
      Logger::Debug("DnaDelta::Set", "Saved:   \t" + delta_str(value));
    }
  } else {
    Logger::Trace("DnaDelta::Set", "Skipped: \t" + delta_str(value));
  }
}

bool DnaDelta::Combine(Minimizer* base_p, const Minimizer* value_p) const {
  auto&& [base_range_ref, base_key_seg, base_range_seg] = *base_p;
  const auto& [range_ref, key_seg, range_seg] = *value_p;

  if (!StrictOverlap(base_range_ref, range_ref)) return false;

  auto new_ref_start = min(base_range_ref.start_, range_ref.start_);
  auto new_ref_end = max(base_range_ref.end_, range_ref.end_);
  if (new_ref_end > new_ref_start + Config::DELTA_MAX_LEN) return false;
  Range new_ref{new_ref_start, new_ref_end, base_range_ref.value_p_};

  if (base_key_seg == key_seg) {
    auto new_seg_start = min(base_range_seg.start_, range_seg.start_);
    auto new_seg_end = max(base_range_seg.end_, range_seg.end_);
    Range new_seg{new_seg_start, new_seg_end, base_range_seg.value_p_};

    *base_p = {new_ref, key_seg, new_seg};
  } else if (!base_range_ref.Contains(range_ref) || base_range_seg.unknown_) {
    // Create a new string in heap.
    auto new_value_seg_p = new string(new_ref.size(), 'N');

    auto fill_in = [new_value_seg_p](size_t start_pos, const Range& range) {
      auto value_seg = range.get();
      for (auto i = 0ul; i < range.size(); ++i) {
        auto j = start_pos + i;
        if (j >= new_value_seg_p->size()) break;
        (*new_value_seg_p)[j] = value_seg[i];
      }
    };

    fill_in(base_range_ref.start_ - new_ref.start_, base_range_seg);
    fill_in(range_ref.start_ - new_ref.start_, range_seg);
    Logger::Debug("DnaDelta::Combine", "Created: " + *new_value_seg_p);

    // Delete the old created string.
    if (base_key_seg.empty() && base_range_seg.value_p_) {
      delete base_range_seg.value_p_;
    }

    // Replace the original string.
    auto n_count = count(new_value_seg_p->begin(), new_value_seg_p->end(), 'N');
    auto unknown = n_count >= new_value_seg_p->size() * Config::UNKNOWN_RATE;
    Range new_seg{
        0,
        new_value_seg_p->size(),
        new_value_seg_p,
        false,
        unknown,
    };
    *base_p = {new_ref, "", new_seg};
  }
  return true;
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
