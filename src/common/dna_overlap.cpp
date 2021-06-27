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
    auto new_base = base;
    if (base) {
      assert(base.mode_ == range.mode_);
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
    using MergedOverlap = tuple<Range, Range, size_t>;
    unordered_map<string, vector<MergedOverlap>> merged_overlaps;

    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      auto&& merged_overlaps_seg = merged_overlaps[key_seg];
      auto merged = false;

      for (auto&& [merged_ref, merged_seg, count] : merged_overlaps_seg) {
        auto new_merged_ref = merge(merged_ref, range_ref);
        auto new_merged_seg = merge(merged_seg, range_seg);
        auto delta_ref = new_merged_ref.size() - merged_ref.size();
        auto delta_seg = new_merged_seg.size() - merged_seg.size();

        if (FuzzyCompare(delta_ref, delta_seg, Config::MINIMIZER_MAX_DIFF) &&
            Verify(new_merged_ref, new_merged_seg)) {
          merged_ref = new_merged_ref;
          merged_seg = new_merged_seg;
          ++count;
          merged = true;
          break;
        }
      }

      if (!merged) {
        merged_overlaps_seg.emplace_back(range_ref, range_seg, 1);
      }
    }

    entries.clear();
    for (const auto& [key_seg, merged_overlaps_seg] : merged_overlaps) {
      for (const auto& [merged_ref, merged_seg, count] : merged_overlaps_seg) {
        auto used = count >= Config::MINIMIZER_MIN_COUNT &&
                    merged_ref.size() >= Config::MINIMIZER_MIN_LEN &&
                    merged_seg.size() >= Config::MINIMIZER_MIN_LEN;

        auto log_title = string("Mode: ") + to_string(merged_seg.mode_);

        if (used) {
          entries.emplace(merged_ref, key_seg, merged_seg);

          Logger::Debug("DnaOverlap::Merge " + key_ref, log_title);
          Logger::Debug("Minimizer count", to_string(count) + " used");
          Logger::Trace("", "REF: \t" + merged_ref.Head());
          Logger::Trace("", "SEG: \t" + merged_seg.Head());
        } else {
          Logger::Trace("DnaOverlap::Merge " + key_ref, log_title);
          Logger::Trace("Minimizer count", to_string(count) + " not used");
          Logger::Trace("", "REF: \t" + merged_ref.Head());
          Logger::Trace("", "SEG: \t" + merged_seg.Head());
        }
      }
    }
  }
}

void DnaOverlap::SelectChain() {
  for (auto&& [key_ref, entries] : data_) {
    unordered_map<string, double> coverages;
    auto max_coverage = 0.0;
    string max_key;

    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      auto pos = key_seg.find('_');
      assert(pos != string::npos);
      auto key = key_seg.substr(0, pos);
      coverages[key] = 0.0;
    }

    for (auto&& [key, _coverage] : coverages) {
      auto coverage = CheckCoverage(key_ref, key);
      if (coverage > max_coverage) {
        max_coverage = coverage;
        max_key = key;
      }
    }
    Logger::Info("DnaOverlap::SelectChain " + key_ref, "Select " + max_key);

    for (auto entry_i = entries.begin(); entry_i != entries.end();) {
      if (entry_i->key_seg_.compare(0, max_key.size(), max_key)) {
        entry_i = entries.erase(entry_i);
      } else {
        ++entry_i;
      }
    }
  }
}

double DnaOverlap::CheckCoverage(
    const string& key_ref, const string& key_sv) const {
  const auto& entries = data_.at(key_ref);
  if (!entries.size()) return 0.0;

  auto ref_size = entries.begin()->range_ref_.value_p_->size();
  vector<int> covered(ref_size + 1);
  for (const auto& [range_ref, key_seg, range_seg] : entries) {
    if (key_sv.size() && key_seg.compare(0, key_sv.size(), key_sv)) continue;

    auto start_padding = range_seg.start_;
    auto end_padding = range_seg.value_p_->size() - range_seg.end_;
    if (range_seg.mode_ == REVERSE || range_seg.mode_ == REVR_COMP) {
      swap(start_padding, end_padding);
    }

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

  Logger::Debug(
      "DnaOverlap::CheckCoverage " + key_ref,
      (key_sv.size() ? key_sv : "Total") +
          " cover rate: " + to_string(covered_rate * 100) + " %");

  return covered_rate;
}

void DnaOverlap::CheckCoverage() const {
  for (const auto& [key_ref, entries] : data_) {
    CheckCoverage(key_ref);
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

bool DnaOverlap::operator<(const DnaOverlap& that) const {
  return size() < that.size();
}
