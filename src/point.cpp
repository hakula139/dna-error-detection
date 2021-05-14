#include "point.h"

#include <string>

using std::string;
using std::to_string;

string Point::Stringify() const {
  return "(" + to_string(x_) + ", " + to_string(y_) + ")";
}

bool Point::operator==(const Point& that) const {
  return x_ == that.x_ && y_ == that.y_;
}

bool Point::operator!=(const Point& that) const { return !(*this == that); }
