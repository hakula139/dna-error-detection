#ifndef SRC_DNA_DELTA_H_
#define SRC_DNA_DELTA_H_

#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
  void Set(const std::string& key, const Range& value);
  void Combine();

  friend class Dna;

 private:
  std::unordered_map<std::string, std::vector<Range>> data_;
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

#endif  // SRC_DNA_DELTA_H_
