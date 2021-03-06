#ifndef SRC_UTILS_PROGRESS_H_
#define SRC_UTILS_PROGRESS_H_

#include <string>

class Progress {
 public:
  Progress(
      const std::string& name, size_t total, size_t step = 1, size_t cur = 0)
      : name_(name), total_(total), step_(step > 1 ? step : 1), cur_(cur) {}

  void Set(size_t cur);
  void Print(bool endl = false) const;

  Progress& operator++();
  Progress& operator+=(size_t step);

 private:
  std::string name_;
  size_t total_;
  size_t step_;
  size_t cur_;
};

#endif  // SRC_UTILS_PROGRESS_H_
