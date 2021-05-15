#include "dna_delta.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "range.h"
#include "utils/config.h"
#include "utils/logger.h"

using std::make_pair;
using std::max;
using std::min;
using std::ofstream;
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
  data_[key].push_back(range);
  logger.Debug(
      "DnaDelta::Set", "Saved: " + type_ + " " + Stringify(key, range));
}

void DnaDelta::Combine() {
  for (auto&& [key, ranges] : data_) {
    if (!ranges.size()) continue;
    for (auto i = ranges.begin() + 1; i < ranges.end();) {
      auto prev_i = prev(i);
      if (i->start_ == prev_i->start_) {
        prev_i->end_ += i->size();
        i = ranges.erase(i);
      } else if (
          (prev_i->size() < config.min_length ||
           i->size() < config.min_length) &&
          i->start_ <= prev_i->end_ + config.min_length &&
          prev_i->start_ <= i->end_ + config.min_length) {
        prev_i->start_ = min(prev_i->start_, i->start_);
        prev_i->end_ = max(prev_i->end_, i->end_);
        i = ranges.erase(i);
      } else {
        ++i;
      }
    }
  }
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
  data_[make_pair(key1, key2)].push_back(make_pair(range1, range2));
  logger.Debug(
      "DnaDelta::Set",
      "Saved: " + type_ + " " + Stringify(key1, range1, key2, range2));
}
