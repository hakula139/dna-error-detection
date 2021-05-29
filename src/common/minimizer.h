#ifndef SRC_COMMON_MINIMIZER_H_
#define SRC_COMMON_MINIMIZER_H_

#include <string>

#include "range.h"

struct Minimizer {
  Minimizer() {}
  Minimizer(
      const Range& range_ref,
      const std::string& key_seg,
      const Range& range_seg)
      : range_ref_(range_ref), key_seg_(key_seg), range_seg_(range_seg) {}

  std::string Stringify() const;

  bool operator<(const Minimizer& that) const;

  Range range_ref_;
  std::string key_seg_;
  Range range_seg_;
};

#endif  // SRC_COMMON_MINIMIZER_H_
