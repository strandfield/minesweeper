#ifndef MINESWEEPER_POINT_H
#define MINESWEEPER_POINT_H

namespace minesweeper {

/**
 * \brief a two-dimensional point with integer coordinates
 */
class Point
{
public:
  int x = 0;
  int y = 0;

public:
  Point() = default;
  Point(const Point&) = default;
  ~Point() = default;

  Point(int x_, int y_)
      : x{ x_ }
      , y{ y_ }
  {
  }

  Point& operator=(const Point&) = default;
};

inline bool operator==(const Point& lhs, const Point& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const Point& lhs, const Point& rhs)
{
  return !(lhs == rhs);
}

inline bool operator<(const Point& lhs, const Point& rhs)
{
  return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
}

} // namespace minesweeper

#endif // MINESWEEPER_POINT_H
