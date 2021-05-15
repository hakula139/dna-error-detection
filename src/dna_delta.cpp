#include "dna_delta.h"

#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "utils/logger.h"

using std::make_pair;
using std::ofstream;
using std::prev;
using std::string;
using std::to_string;

extern Logger logger;

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
  for (const auto& [key, value] : data_) {
    for (const auto& range : value) {
      out_file << type_ << " " << Stringify(key, range) << "\n";
    }
  }
}

void DnaDelta::Set(const string& key, const Range& value) {
  data_[key].push_back(value);
  logger.Debug(
      "DnaDelta::Set", "Saved: " + type_ + " " + Stringify(key, value));
}

void DnaDelta::Combine() {
  if (type_ == "INS" || type_ == "DEL") {
    for (auto&& [key, value] : data_) {
      if (!value.size()) continue;
      for (auto range_i = value.begin() + 1; range_i < value.end();) {
        if (range_i->start_ == prev(range_i)->start_) {
          prev(range_i)->end_ += range_i->end_ - range_i->start_;
          range_i = value.erase(range_i);
        } else {
          ++range_i;
        }
      }
    }
  }
}

void DnaMultiDelta::Print(ofstream& out_file) const {
  for (const auto& [key, value] : data_) {
    for (const auto& range : value) {
      out_file << type_ << " "
               << Stringify(key.first, range.first, key.second, range.second)
               << "\n";
    }
  }
}

void DnaMultiDelta::Set(
    const string& key1,
    const Range& value1,
    const string& key2,
    const Range& value2) {
  data_[make_pair(key1, key2)].push_back(make_pair(value1, value2));
  logger.Debug(
      "DnaDelta::Set",
      "Saved: " + type_ + " " + Stringify(key1, value1, key2, value2));
}
