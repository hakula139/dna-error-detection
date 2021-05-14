#ifndef SRC_DNA_H_
#define SRC_DNA_H_

#include <string>
#include <unordered_map>

#include "dna_delta.h"
#include "point.h"

class Dna {
 public:
  Dna() {}
  explicit Dna(const std::string& filename) { Import(filename); }

  bool Import(const std::string& filename);
  bool ImportEntry(const std::string& key, const std::string& value);
  bool Get(const std::string& key, std::string* value) const;
  void FindDelta(const Dna& sv, size_t chunk_size = 10000);
  bool PrintDelta(const std::string& filename) const;

 private:
  std::unordered_map<std::string, std::string> data_;
  DnaDelta ins_delta_{"INS"};
  DnaDelta del_delta_{"DEL"};
  DnaDelta dup_delta_{"DUP"};
  DnaDelta inv_delta_{"INV"};
  DnaMultiDelta tra_delta_{"TRA"};

  Point FindDeltaChunk(
      const std::string& key,
      const std::string* ref_p,
      size_t ref_start,
      size_t m,
      const std::string* sv_p,
      size_t sv_start,
      size_t n,
      bool reach_end = false);
};

#endif  // SRC_DNA_H_
