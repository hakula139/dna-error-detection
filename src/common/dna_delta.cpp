#include "dna_delta.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "config.h"
#include "logger.h"
#include "range.h"
#include "utils.h"

using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::prev;
using std::string;

extern Logger logger;
extern Config config;

void DnaDelta::Print(ofstream& out_file) const {
  for (const auto& [key, ranges] : data_) {
    for (const auto& range : ranges) {
      out_file << type_ << " " << range.Stringify(key) << "\n";
    }
  }
}

void DnaDelta::Set(const string& key, const Range& range) {
  auto& ranges = data_[key];
  auto exist = [&](const Range& range) {
    for (auto&& prev : ranges) {
      if (Combine(&prev, &range)) return true;
    }
    return false;
  };
  if (!ranges.size() || !exist(range)) {
    ranges.push_back(range);
  }
  logger.Debug("DnaDelta::Set", "Saved: " + type_ + " " + range.Stringify(key));
}

bool DnaDelta::Combine(Range* base_p, const Range* range_p) const {
  if (FuzzyCompare(*range_p, *base_p)) {
    auto new_start = min(base_p->start_, range_p->start_);
    auto new_end = max(base_p->end_, range_p->end_);
    if (new_end > new_start + config.delta_max_len) return false;
    base_p->start_ = new_start;
    base_p->end_ = new_end;
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
      ranges.push_back(range);
      logger.Debug(
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
