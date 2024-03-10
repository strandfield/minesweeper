#include "squareset.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <list>
#include <map>
#include <vector>

namespace minesweeper {

SquareSetMask move_left(SquareSetMask mask)
{
  mask = mask & ~SquareSetMask(RIGHT_COLUMN);
  return SquareSetMask(mask.value() << 1);
}

SquareSetMask move_right(SquareSetMask mask)
{
  mask = mask & ~SquareSetMask(LEFT_COLUMN);
  return SquareSetMask(mask.value() >> 1);
}

SquareSetMask move_up(SquareSetMask mask)
{
  mask = mask & ~SquareSetMask(BOTTOM_ROW);
  return SquareSetMask(mask.value() << 3);
}

SquareSetMask move_down(SquareSetMask mask)
{
  mask = mask & ~SquareSetMask(TOP_ROW);
  return SquareSetMask(mask.value() >> 3);
}

SquareSet::SquareSet(int x_, int y_, SquareSetMask mask_)
    : x(x_)
    , y(y_)
    , mask(mask_)
{
}

SquareSet::SquareSet(int x_, int y_, const SquareSet& other) :
    x{ x_ },
    y{ y_ }
{
  if (std::abs(other.x - x) >= 3 || std::abs(other.y - y) >= 3) {
    return;
  }

  mask = other.mask;

  int ox = other.x;
  int oy = other.y;

  while (ox > x)
  {
    mask = move_left(mask);
    ox--;
  }
  while (ox < x)
  {
    mask = move_right(mask);
    ox++;
  }

  while (oy > y)
  {
    mask = move_up(mask);
    oy--;
  }
  while (oy < y)
  {
    mask = move_down(mask);
    oy++;
  }
}

void SquareSet::normalize()
{
  if (!mask)
    return;

  // ensures that there is at least one square in the top row
  // or left column

  while (!(mask & LEFT_COLUMN))
  {
    mask = move_right(mask);
    ++x;
  }

  while (!(mask & TOP_ROW))
  {
    mask = move_down(mask);
    ++y;
  }
}

SquareSet SquareSet::normalized() const
{
  SquareSet copy{ *this };
  copy.normalize();
  return copy;
}

int SquareSet::xmax() const
{
  if (mask & RIGHT_COLUMN) {
    return x + 2;
  } else if (mask & CENTER_COLUMN) {
    return x + 1;
  } else {
    return x;
  }
}

int SquareSet::ymax() const
{
  if (mask & BOTTOM_ROW) {
    return y + 2;
  } else if (mask & CENTER_ROW)
    return y + 1;
  else
    return y;
}

bool operator<(const SquareSet& lhs, const SquareSet& rhs)
{
  return std::make_tuple(lhs.x, lhs.y, lhs.mask.value()) < std::make_tuple(rhs.x, rhs.y, rhs.mask.value());
}

SquareSet operator&(const SquareSet& lhs, const SquareSet& rhs)
{
  SquareSet other{ lhs.x, lhs.y, rhs };
  return SquareSet{ lhs.x, lhs.y, lhs.mask & other.mask };
}

SquareSet operator-(const SquareSet& lhs, const SquareSet& rhs)
{
  SquareSet other{ lhs.x, lhs.y, rhs };
  return SquareSet{ lhs.x, lhs.y, lhs.mask & ~other.mask };
}

} // namespace minesweeper
