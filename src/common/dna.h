#ifndef SRC_COMMON_DNA_H_
#define SRC_COMMON_DNA_H_

#include <string>
#include <unordered_map>
#include <utility>

#include "dna_delta.h"
#include "point.h"

class Dna {
 public:
  Dna() {}
  explicit Dna(const std::string& filename) { Import(filename); }

  bool Import(const std::string& filename);
  bool ImportEntry(const std::string& key, const std::string& value);
  bool Get(const std::string& key, std::string* value) const;

  void CreateIndex();
  bool PrintIndex(const std::string& filename) const;

  void FindDeltas(const Dna& sv, size_t chunk_size = 10000);
  void FindDupDeltas();
  void FindInvDeltas();
  void FindTraDeltas();
  void ProcessDeltas();
  bool PrintDeltas(const std::string& filename) const;

 protected:
  Point FindDeltasChunk(
      const std::string& key,
      const std::string* ref_p,
      size_t ref_start,
      size_t m,
      const std::string* sv_p,
      size_t sv_start,
      size_t n,
      bool reach_end = false);

 private:
  std::unordered_map<std::string, std::string> data_;
  std::unordered_map<uint64_t, std::pair<std::string, Range>> range_index_;

  DnaDelta ins_deltas_{"INS"};
  DnaDelta del_deltas_{"DEL"};
  DnaDelta dup_deltas_{"DUP"};
  DnaDelta inv_deltas_{"INV"};
  DnaMultiDelta tra_deltas_{"TRA"};
};

#endif  // SRC_COMMON_DNA_H_
