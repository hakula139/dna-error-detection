#ifndef SRC_POINT_H_
#define SRC_POINT_H_

#include <string>

struct Point {
  Point() {}
  Point(int x, int y) : x_(x), y_(y) {}
  std::string Stringify() const;
  bool operator==(const Point& that) const;
  bool operator!=(const Point& that) const;

  int x_ = 0;
  int y_ = 0;
};

#endif  // SRC_POINT_H_
