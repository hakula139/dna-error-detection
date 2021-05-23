#ifndef SRC_COMMON_DNA_OVERLAP_H_
#define SRC_COMMON_DNA_OVERLAP_H_

#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "range.h"

class DnaOverlap {
 public:
  using Minimizer = std::tuple<std::string, Range, Range>;

  DnaOverlap() {}
  explicit DnaOverlap(const Minimizer& entry) : data_{entry} {}

  size_t size() const { return data_.size(); }
  void Sort();
  void Print(std::ofstream& out_file) const;
  DnaOverlap& operator+=(const DnaOverlap& that);
  DnaOverlap& operator+=(const Minimizer& entry);
  DnaOverlap& operator=(const DnaOverlap& that);

  friend class Dna;

 protected:
 private:
  std::vector<Minimizer> data_;
};

#endif  // SRC_COMMON_DNA_OVERLAP_H_
