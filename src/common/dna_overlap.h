#ifndef SRC_COMMON_DNA_OVERLAP_H_
#define SRC_COMMON_DNA_OVERLAP_H_

#include <fstream>
#include <set>
#include <string>
#include <unordered_map>

#include "minimizer.h"

class DnaOverlap {
 public:
  DnaOverlap() {}

  size_t size() const;
  void Insert(const std::string& key_ref, const Minimizer& entry);
  void Merge();
  void SelectChain();
  double CheckCoverage(
      const std::string& key_ref, const std::string& key_sv = "") const;
  void CheckCoverage() const;
  void Print(std::ofstream& out_file) const;

  DnaOverlap& operator+=(const DnaOverlap& that);
  bool operator<(const DnaOverlap& that) const;

  friend class Dna;

 private:
  std::unordered_map<std::string, std::set<Minimizer>> data_;
};

#endif  // SRC_COMMON_DNA_OVERLAP_H_
