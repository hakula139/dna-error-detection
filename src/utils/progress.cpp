#include "progress.h"

#include <string>

#include "logger.h"

using std::to_string;

Progress& Progress::operator++() {
  if (cur_ < total_) ++cur_;

  if (cur_ % 100 == 0) {
    Print();
  } else if (cur_ == total_) {
    Print(true);
  }
  return *this;
}

void Progress::Print(bool endl) const {
  Logger::Info(name_, to_string(cur_) + " / " + to_string(total_) + "\r", endl);
}
