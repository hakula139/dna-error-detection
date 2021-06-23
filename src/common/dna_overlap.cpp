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
  auto merge = [](Range& base, const Range& range) {
    assert(range.start_ < range.end_);
    if (base) {
      base.start_ = min(base.start_, range.start_);
      base.end_ = max(base.end_, range.end_);
    } else {
      base = range;
    }
    assert(base.start_ < base.end_);
    assert(base.value_p_);
  };

  for (auto&& [key_ref, entries] : data_) {
    unordered_map<string, tuple<Range, Range, size_t>> merged_overlaps;

    for (const auto& [range_ref, key_seg, range_seg] : entries) {
      auto&& [merged_ref, merged_seg, count] = merged_overlaps[key_seg];
      merge(merged_ref, range_ref);
      merge(merged_seg, range_seg);
      ++count;
    }

    entries.clear();
    for (const auto& [key_seg, entry] : merged_overlaps) {
      const auto& [merged_ref, merged_seg, count] = entry;

      auto used = count >= Config::MINIMIZER_MIN_COUNT &&
                  merged_ref.size() >= Config::MINIMIZER_MIN_LEN &&
                  merged_seg.size() >= Config::MINIMIZER_MIN_LEN;

      auto show_size = min(min(merged_ref.size(), merged_seg.size()), 100ul);
      auto show_ref = merged_ref.get().substr(0, show_size);
      auto show_seg = merged_seg.get().substr(0, show_size);

      if (used) {
        entries.emplace(merged_ref, key_seg, merged_seg);

        Logger::Debug("DnaOverlap::Merge " + key_ref, "Minimizer:");
        Logger::Debug("Minimizer count", to_string(count) + " \tused");
        Logger::Debug("", "REF: \t" + show_ref);
        Logger::Debug("", "SEG: \t" + show_seg);
      } else {
        Logger::Debug("DnaOverlap::Merge " + key_ref, "Minimizer:");
        Logger::Debug("Minimizer count", to_string(count) + " \tnot used");
        Logger::Debug("", "REF: \t" + show_ref);
        Logger::Debug("", "SEG: \t" + show_seg);
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
