#include "dna_delta.h"

#include <fstream>
#include <string>
#include <utility>

#include "utils/logger.h"

using std::make_pair;
using std::ofstream;
using std::string;
using std::to_string;

extern Logger logger;

string Stringify(const string& key, const Range& range) {
  return key + " " + to_string(range.start_) + " " + to_string(range.end_);
}

void DnaDelta::Print(ofstream& out_file) const {
  for (const auto& [key, value] : data_) {
    for (const auto& range : value) {
      out_file << type_ << " ";
      out_file << Stringify(key, range) << "\n";
    }
  }
}

void DnaDelta::Set(const string& key, const Range& value) {
  data_[key].push_back(value);
}

void DnaMultiDelta::Print(ofstream& out_file) const {
  for (const auto& [key, value] : data_) {
    for (const auto& range : value) {
      out_file << type_ << " ";
      out_file << Stringify(key.first, range.first) << " "
               << Stringify(key.second, range.second) << "\n";
    }
  }
}

void DnaMultiDelta::Set(
    const string& key1,
    const Range& value1,
    const string& key2,
    const Range& value2) {
  data_[make_pair(key1, key2)].push_back(make_pair(value1, value2));
}
