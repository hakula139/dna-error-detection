#include "progress.h"

#include <iostream>
#include <string>

#include "logger.h"

using std::cout;
using std::flush;
using std::to_string;

Progress& Progress::operator++() {
  if (cur_ < total_) ++cur_;

  if (cur_ % 100 == 0 || cur_ == total_) Print();
  return *this;
}

void Progress::Print() const {
  Logger::Debug(
      name_, to_string(cur_) + " / " + to_string(total_) + "\r", false);
  cout << flush;
}
