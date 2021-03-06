#ifndef SRC_COMMON_DNA_DELTA_H_
#define SRC_COMMON_DNA_DELTA_H_

#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "minimizer.h"
#include "range.h"

class DnaDeltaBase {
 public:
  explicit DnaDeltaBase(const std::string& type) : type_(type) {}

  virtual void Print(std::ofstream& out_file) const = 0;

 protected:
  std::string type_;
};

class DnaDelta : public DnaDeltaBase {
 public:
  explicit DnaDelta(const std::string& type) : DnaDeltaBase{type} {}

  void Print(std::ofstream& out_file) const override;
  void Set(const std::string& key, const Minimizer& value);
  void Merge(
      const std::string& key_ref,
      const std::string& key_seg = "",
      const Range& range = {});
  void Filter(const std::string& key_ref, const std::string& key_seg);
  double GetDensity(
      const std::string& key,
      const Range& range,
      std::vector<Range>* delta_ranges_p);

  friend class Dna;

 protected:
  bool Combine(
      Minimizer* base_p, const Minimizer* value_p, bool strict = true) const;

 private:
  std::unordered_map<std::string, std::vector<Minimizer>> data_;
  std::unordered_map<std::string, std::vector<int>> density_;
};

class DnaMultiDelta : public DnaDeltaBase {
 public:
  explicit DnaMultiDelta(const std::string& type) : DnaDeltaBase{type} {}

  void Print(std::ofstream& out_file) const override;
  void Set(
      const std::string& key1,
      const Range& range1,
      const std::string& key2,
      const Range& range2);

 protected:
  bool Combine(
      std::pair<Range, Range>* base_p,
      const std::pair<Range, Range>* range_p) const;

 private:
  struct PairHash {
   public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& p) const {
      return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
    }
  };

  std::unordered_map<
      std::pair<std::string, std::string>,
      std::vector<std::pair<Range, Range>>,
      PairHash>
      data_;
};

#endif  // SRC_COMMON_DNA_DELTA_H_
