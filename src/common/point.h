#ifndef SRC_COMMON_POINT_H_
#define SRC_COMMON_POINT_H_

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

enum Direction {
  TOP = 1,
  BOTTOM = 1,
  LEFT = -1,
  RIGHT = -1,
  TOP_LEFT = 0,
  BOTTOM_RIGHT = 0,
};

#endif  // SRC_COMMON_POINT_H_
