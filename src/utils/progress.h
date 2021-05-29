#ifndef SRC_UTILS_PROGRESS_H_
#define SRC_UTILS_PROGRESS_H_

#include <string>

class Progress {
 public:
  Progress(const std::string& name, size_t total, size_t cur = 0)
      : name_(name), total_(total), cur_(cur) {}

  void Print(bool endl = false) const;
  Progress& operator++();

 private:
  std::string name_;
  size_t total_;
  size_t cur_;
};

#endif  // SRC_UTILS_PROGRESS_H_
