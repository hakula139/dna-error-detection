#include "dna_delta.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "range.h"
#include "utils/config.h"
#include "utils/logger.h"
#include "utils/utils.h"

using std::make_pair;
using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::prev;
using std::string;
using std::to_string;

extern Logger logger;
extern Config config;

string Stringify(const string& key, const Range& range) {
  return key + " " + to_string(range.start_) + " " + to_string(range.end_);
}

string Stringify(
    const string& key1,
    const Range& range1,
    const string& key2,
    const Range& range2) {
  return Stringify(key1, range1) + " " + Stringify(key2, range2);
}

void DnaDelta::Print(ofstream& out_file) const {
  for (const auto& [key, ranges] : data_) {
    for (const auto& range : ranges) {
      out_file << type_ << " " << Stringify(key, range) << "\n";
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
  logger.Debug(
      "DnaDelta::Set", "Saved: " + type_ + " " + Stringify(key, range));
}

bool DnaDelta::Combine(Range* base_p, const Range* range_p) const {
  if (base_p->start_ == range_p->start_) {
    base_p->end_ += range_p->size();
    return true;
  }
  if (FuzzyCompare(range_p->start_, base_p->end_) ||
      FuzzyCompare(base_p->start_, range_p->end_)) {
    auto new_start = min(base_p->start_, range_p->start_);
    auto new_end = max(base_p->start_, range_p->end_);
    if (new_end > new_start + config.max_length) return false;
    base_p->start_ = new_start;
    base_p->end_ = new_end;
    return true;
  }
  return false;
}

void DnaMultiDelta::Print(ofstream& out_file) const {
  for (const auto& [key, ranges] : data_) {
    for (const auto& range : ranges) {
      out_file << type_ << " "
               << Stringify(key.first, range.first, key.second, range.second)
               << "\n";
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
    auto& ranges = data_[make_pair(key1, key2)];
    auto range = make_pair(range1, range2);
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
          "Saved: " + type_ + " " + Stringify(key1, range1, key2, range2));
    }
  };
  if (data_.count(make_pair(key2, key1))) {
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
