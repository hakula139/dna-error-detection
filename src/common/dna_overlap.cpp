#include "dna_overlap.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
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
using std::swap;
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
  auto merge = [](const Range& base, const Range& range) {
    assert(range.start_ < range.end_);
    Range new_base = base;
    if (base) {
      new_base.start_ = min(base.start_, range.start_);
      new_base.end_ = max(base.end_, range.end_);
    } else {
      new_base = range;
    }
    assert(new_base.start_ < new_base.end_);
    assert(new_base.value_p_);
    return new_base;
  };

  for (auto&& [key_ref, entries] : data_) {
    unordered_map<string, tuple<Range, Range, size_t>> merged_overlaps;

    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      auto&& [merged_ref, merged_seg, count] = merged_overlaps[key_seg];
      auto new_merged_ref = merge(merged_ref, range_ref);
      auto new_merged_seg = merge(merged_seg, range_seg);
      auto delta_ref = new_merged_ref.size() - merged_ref.size();
      auto delta_seg = new_merged_seg.size() - merged_seg.size();

      if (FuzzyCompare(delta_ref, delta_seg, Config::OVERLAP_MAX_DIFF)) {
        merged_ref = new_merged_ref;
        merged_seg = new_merged_seg;
        ++count;
      } else {
        Logger::Trace(
            "DnaOverlap::Merge " + key_seg,
            "+" + to_string(delta_ref) + " +" + to_string(delta_seg) +
                ": deltas not matched, not merged");
      }
    }

    entries.clear();
    for (const auto& [key_seg, entry] : merged_overlaps) {
      const auto& [merged_ref, merged_seg, count] = entry;

      auto used = count >= Config::MINIMIZER_MIN_COUNT &&
                  merged_ref.size() >= Config::MINIMIZER_MIN_LEN &&
                  merged_seg.size() >= Config::MINIMIZER_MIN_LEN;

      auto log_title = string("Minimizer: ") +
                       (merged_seg.inverted_ ? "inverted" : "not inverted");

      if (used) {
        entries.emplace(merged_ref, key_seg, merged_seg);

        Logger::Debug("DnaOverlap::Merge " + key_ref, log_title);
        Logger::Debug("Minimizer count", to_string(count) + " \tused");
        Logger::Trace("", "REF: \t" + merged_ref.Head());
        Logger::Trace("", "SEG: \t" + merged_seg.Head());
      } else {
        Logger::Debug("DnaOverlap::Merge " + key_ref, log_title);
        Logger::Debug("Minimizer count", to_string(count) + " \tnot used");
        Logger::Trace("", "REF: \t" + merged_ref.Head());
        Logger::Trace("", "SEG: \t" + merged_seg.Head());
      }
    }
  }
}

void DnaOverlap::CheckCoverage() const {
  for (const auto& [key_ref, entries] : data_) {
    if (!entries.size()) continue;

    auto ref_size = entries.begin()->range_ref_.value_p_->size();
    vector<int> covered(ref_size + 1);
    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      auto start_padding = range_seg.start_;
      auto end_padding = range_seg.value_p_->size() - range_seg.end_;
      if (range_seg.inverted_) swap(start_padding, end_padding);

      Range cover_range{
          max(range_ref.start_, range_seg.start_) - start_padding,
          range_ref.end_ + end_padding,
          nullptr,
      };
      ++covered[cover_range.start_];
      --covered[cover_range.end_];
    }

    auto covered_rate = 0.0;
    for (auto i = 0ul; i < ref_size; ++i) {
      if (covered[i] > 0) ++covered_rate;
      covered[i + 1] += covered[i];
    }
    covered_rate /= ref_size;

    Logger::Info(
        "DnaOverlap::CheckCoverage " + key_ref,
        "Minimizer cover rate: " + to_string(covered_rate * 100) + " %");
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
