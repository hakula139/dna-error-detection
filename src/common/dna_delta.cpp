#include "dna_delta.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "config.h"
#include "logger.h"
#include "minimizer.h"
#include "range.h"
#include "utils.h"

using std::accumulate;
using std::count;
using std::fill;
using std::max;
using std::min;
using std::move;
using std::next;
using std::ofstream;
using std::pair;
using std::prev;
using std::string;
using std::to_string;
using std::vector;

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
        Logger::Trace("DnaDelta::Set", "Merged:  \t" + delta_str(*prev_i));
        return true;
      }
    }
    return false;
  };

  if (value.range_ref_.size() > Config::DELTA_IGNORE_LEN) {
    if (deltas.empty() || !exist(value)) {
      deltas.emplace_back(value);
      Logger::Trace("DnaDelta::Set", "Saved:   \t" + delta_str(value));
    }
  } else {
    Logger::Trace("DnaDelta::Set", "Skipped: \t" + delta_str(value));
  }
}

void DnaDelta::Merge(
    const string& key_ref, const string& key_seg, const Range& range) {
  auto& deltas = data_[key_ref];
  vector<Minimizer> merged_deltas;

  for (const auto& delta : deltas) {
    auto merged = false;
    if (key_seg.empty() || delta.key_seg_ == key_seg) {
      if (range && !range.Contains(delta.range_ref_)) {
        continue;
      }
      for (auto&& merged_delta : merged_deltas) {
        if (Combine(&merged_delta, &delta, false)) {
          merged = true;
          break;
        }
      }
    }
    if (!merged) {
      merged_deltas.emplace_back(delta);
    }
  }

  deltas = merged_deltas;
}

void DnaDelta::Filter(const string& key_ref, const string& key_seg) {
  auto filter_ref = [&](vector<Minimizer>& deltas, const string& key_ref_i) {
    for (auto delta_i = deltas.end() - 1;
         delta_i >= deltas.begin() &&
         (delta_i->key_seg_ == key_seg || delta_i->key_seg_.empty());
         --delta_i) {
      auto&& [range_ref, key_seg_i, range_seg] = *delta_i;
      if (range_ref.size() < Config::DELTA_MIN_LEN ||
          range_ref.size() > Config::DELTA_MAX_LEN) {
        if (key_seg_i.empty()) {
          delete range_seg.value_p_;
        }
        delta_i = deltas.erase(delta_i);
      } else {
        Logger::Debug(
            "DnaDelta::Filter",
            "Saved:   \t" + type_ + " " + range_ref.Stringify(key_ref_i));
      }
    }
  };

  if (key_ref.empty()) {
    for (auto&& [key_ref_i, deltas] : data_) {
      filter_ref(deltas, key_ref_i);
    }
  } else {
    auto&& deltas = data_[key_ref];
    filter_ref(deltas, key_ref);
  }
}

double DnaDelta::GetDensity(
    const string& key, const Range& range, vector<Range>* delta_ranges_p) {
  auto& deltas = data_[key];
  auto& density = density_[key];

  auto start = density.begin() + range.start_;
  auto end = density.begin() + range.end_;

  fill(
      prev(start, min(range.start_, Config::DELTA_MAX_LEN)),
      next(end, Config::DELTA_MAX_LEN),
      0);

  auto min_start = range.start_;
  auto max_end = range.end_;
  for (const auto& delta : deltas) {
    if (!StrictOverlap(delta.range_ref_, range)) continue;
    const auto& delta_range = delta.range_ref_;

    ++density[delta_range.start_];
    --density[delta_range.end_];
    min_start = min(min_start, delta_range.start_);
    max_end = max(max_end, delta_range.end_);
  }

  for (auto i = min_start; i <= max_end; ++i) {
    density[i + 1] += density[i];
    density[i] = (density[i] > 0);
  }

  auto window_size = Config::DENSITY_WINDOW_SIZE;
  auto sum = accumulate(start, start + window_size - 1, 0.0);
  auto max_density = sum / window_size;
  Range delta_range;

  for (auto i = start + window_size - 1; i < end - window_size; ++i) {
    sum += (i >= start + window_size) ? *i - *(i - window_size) : *i;
    auto cur_density = sum / window_size;
    max_density = max(max_density, cur_density);

    if (cur_density > 1) {
      Logger::Warn("DnaDelta::GetDensity", to_string(cur_density) + " > 1");
    }
    if (cur_density >= Config::SIGNAL_RATE) {
      auto cur_start = i - density.begin() - window_size + 1ul;
      if (!delta_range.start_) {
        delta_range.start_ = cur_start;
      }
      delta_range.end_ = cur_start + window_size;
    } else if (
        cur_density < Config::SIGNAL_RATE - Config::NOISE_RATE && delta_range) {
      delta_ranges_p->emplace_back(move(delta_range));
      delta_range = Range{};
    }
  }
  if (delta_range) {
    delta_ranges_p->emplace_back(move(delta_range));
  }

  Logger::Debug(
      "DnaDelta::GetDensity",
      type_ + " " + range.Stringify(key) + " " + to_string(max_density));

  return max_density;
}

bool DnaDelta::Combine(
    Minimizer* base_p, const Minimizer* value_p, bool strict) const {
  auto&& [base_range_ref, base_key_seg, base_range_seg] = *base_p;
  const auto& [range_ref, key_seg, range_seg] = *value_p;

  if (strict && !StrictOverlap(base_range_ref, range_ref)) return false;
  // if (!strict && !FuzzyOverlap(base_range_ref, range_ref)) return false;

  auto new_ref_start = min(base_range_ref.start_, range_ref.start_);
  auto new_ref_end = max(base_range_ref.end_, range_ref.end_);
  if (new_ref_end > new_ref_start + Config::DELTA_ALLOW_LEN) return false;
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
        auto c = value_seg[i];
        if (c != 'N') {
          if (string("ATCG").find(c) == string::npos) {
            Logger::Warn("DnaDelta::Combine", "Invalid char: " + string(1, c));
          }
          (*new_value_seg_p)[j] = value_seg[i];
        }
      }
    };

    fill_in(base_range_ref.start_ - new_ref.start_, base_range_seg);
    fill_in(range_ref.start_ - new_ref.start_, range_seg);
    Logger::Trace("DnaDelta::Combine", "Created: " + *new_value_seg_p);

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
        NORMAL,
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
