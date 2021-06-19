#ifndef SRC_COMMON_DNA_H_
#define SRC_COMMON_DNA_H_

#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dna_delta.h"
#include "dna_overlap.h"
#include "point.h"

class Dna {
 public:
  Dna() {}
  explicit Dna(const std::string& filename) { Import(filename); }

  bool Import(const std::string& filename);
  bool get(const std::string& key, std::string* value) const;
  size_t size() const { return data_.size(); }
  bool Print(const std::string& filename) const;

  bool ImportIndex(const std::string& filename);
  void CreateIndex();
  bool PrintIndex(const std::string& filename) const;

  bool ImportOverlaps(Dna* segments_p, const std::string& filename);
  bool FindOverlaps(const Dna& ref);
  bool PrintOverlaps(const std::string& filename) const;

  void CreateSvChain(const Dna& ref, const Dna& segments);

  void FindDeltas(const Dna& sv, size_t chunk_size = 10000);
  void FindDeltasFromSegments();
  void IgnoreSmallDeltas();
  void FindDupDeltas();
  void FindInvDeltas();
  void FindTraDeltas();
  void ProcessDeltas();
  bool PrintDeltas(const std::string& filename) const;

  friend class Test;

 protected:
  static uint64_t NextHash(uint64_t hash, char next_base);
  static std::string Invert(const std::string& chain);

  Point FindDeltasChunk(
      const std::string& key,
      const std::string& ref,
      size_t ref_start,
      size_t m,
      const std::string& sv,
      size_t sv_start,
      size_t n,
      bool reach_end = false);

 private:
  std::unordered_map<std::string, std::string> data_;

  std::unordered_multimap<uint64_t, std::pair<std::string, Range>> range_index_;

  DnaOverlap overlaps_;

  DnaDelta ins_deltas_{"INS"};
  DnaDelta del_deltas_{"DEL"};
  DnaDelta dup_deltas_{"DUP"};
  DnaDelta inv_deltas_{"INV"};
  DnaMultiDelta tra_deltas_{"TRA"};
};

#endif  // SRC_COMMON_DNA_H_
