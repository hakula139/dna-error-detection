#include "dna.h"

#include <exception>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>

#include "utils/logger.h"

using std::get;
using std::getline;
using std::ifstream;
using std::string;
using std::tuple;
using std::unordered_map;
using ConstStrIt = std::string::const_iterator;

extern Logger logger;

bool Dna::Import(const string& filename) {
  ifstream in_file(filename);
  if (!in_file) {
    logger.Error("Dna::Import", "input file " + filename + " not found\n");
    return false;
  }

  for (string key; getline(in_file, key);) {
    string value;
    getline(in_file, value);
    ImportEntry(key.substr(1), value);
  }

  in_file.close();
  return true;
}

bool Dna::ImportEntry(const string& key, const string& value) {
  data_[key] = value;
  return true;
}

bool Dna::Get(const string& key, string* value) const {
  try {
    *value = data_.at(key);
    return true;
  } catch (std::out_of_range error) {
    *value = "";
    return false;
  }
}

void Dna::FindDelta(const Dna& sv, size_t chunk_size) {
  for (const auto& [key, value_ref] : data_) {
    string value_sv;
    if (!sv.Get(key, &value_sv)) {
      logger.Warn("Dna::FindDelta", "key " + key + " not found in sv chain");
      continue;
    }

    ConstStrIt i = value_ref.begin();
    ConstStrIt j = value_sv.begin();
    while (i < value_ref.end() || j < value_sv.end()) {
      auto result = FindDeltaChunk(i, j, chunk_size);
      i = get<0>(result);
      j = get<1>(result);
    }
  }
}

tuple<ConstStrIt, ConstStrIt> Dna::FindDeltaChunk(
    ConstStrIt value_ref_i, ConstStrIt value_sv_i, size_t chunk_size) {
  // TODO: implement Myers' diff algorithm
  return {value_ref_i, value_sv_i};
}
