#include "progress.h"

#include <string>

#include "logger.h"

using std::to_string;

void Progress::Set(size_t cur) {
  cur_ = cur < total_ ? cur : total_;
  Print(cur_ == total_);
}

void Progress::Print(bool endl) const {
  Logger::Info(name_, to_string(cur_) + " / " + to_string(total_) + "\r", endl);
}

Progress& Progress::operator++() {
  Set(cur_ + 1);
  return *this;
}

Progress& Progress::operator+=(size_t step) {
  Set(cur_ + step);
  return *this;
}
