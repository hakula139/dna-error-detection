#ifndef SRC_DNA_H_
#define SRC_DNA_H_

#include <string>
#include <tuple>
#include <unordered_map>

class Dna {
 public:
  using ConstStrIt = std::string::const_iterator;

  Dna() {}
  explicit Dna(const std::string& filename) { Import(filename); }

  bool Import(const std::string& filename);
  bool ImportEntry(const std::string& key, const std::string& value);
  bool Get(const std::string& key, std::string* value) const;
  void FindDelta(const Dna& sv, size_t chunk_size = 10000);

 private:
  std::unordered_map<std::string, std::string> data_;
  std::unordered_map<std::string, std::string> delta_;

  std::tuple<ConstStrIt, ConstStrIt> FindDeltaChunk(
      ConstStrIt value_ref_i, ConstStrIt value_sv_i, size_t chunk_size);
};

#endif  // SRC_DNA_H_
