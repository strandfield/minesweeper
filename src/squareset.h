#ifndef MINESWEEPER_SQUARESET_H
#define MINESWEEPER_SQUARESET_H

#include "point.h"

#include <array>
#include <cstdint>

namespace minesweeper {

enum SquareSetMaskFlag
{
  NONE = 0,
  TOP_LEFT = 0x001,
  TOP = 0x002,
  TOP_RIGHT = 0x004,
  LEFT = 0x008,
  CENTER = 0x010,
  RIGHT = 0x020,
  BOTTOM_LEFT = 0x040,
  BOTTOM = 0x080,
  BOTTOM_RIGHT = 0x100,

  // useful combinations
  LEFT_COLUMN = TOP_LEFT | LEFT | BOTTOM_LEFT,
  CENTER_COLUMN = TOP | CENTER | BOTTOM,
  RIGHT_COLUMN = TOP_RIGHT | RIGHT | BOTTOM_RIGHT,
  TOP_ROW = TOP_LEFT | TOP | TOP_RIGHT,
  CENTER_ROW = LEFT | CENTER | RIGHT,
  BOTTOM_ROW = BOTTOM_LEFT | BOTTOM | BOTTOM_RIGHT,
};

inline std::array<SquareSetMaskFlag, 9> enumerateSquareSetMaskFlags()
{
  return {
    SquareSetMaskFlag::TOP_LEFT,
    SquareSetMaskFlag::TOP,
    SquareSetMaskFlag::TOP_RIGHT,
    SquareSetMaskFlag::LEFT,
    SquareSetMaskFlag::CENTER,
    SquareSetMaskFlag::RIGHT,
    SquareSetMaskFlag::BOTTOM_LEFT,
    SquareSetMaskFlag::BOTTOM,
    SquareSetMaskFlag::BOTTOM_RIGHT
  };
}

inline SquareSetMaskFlag pt2mf(int x, int y)
{
  return static_cast<SquareSetMaskFlag>(1 << (y * 3 + x));
}

inline Point mf2pt(SquareSetMaskFlag flag)
{
  switch (flag)
  {
  case SquareSetMaskFlag::TOP_LEFT:
    return Point(0, 0);
  case SquareSetMaskFlag::TOP:
    return Point(1, 0);
  case SquareSetMaskFlag::TOP_RIGHT:
    return Point(2, 0);
  case SquareSetMaskFlag::LEFT:
    return Point(0, 1);
  case SquareSetMaskFlag::CENTER:
    return Point(1, 1);
  case SquareSetMaskFlag::RIGHT:
    return Point(2, 1);
  case SquareSetMaskFlag::BOTTOM_LEFT:
    return Point(0, 2);
  case SquareSetMaskFlag::BOTTOM:
    return Point(1, 2);
  case SquareSetMaskFlag::BOTTOM_RIGHT:
    return Point(2, 2);
  case SquareSetMaskFlag::NONE:
  default:
    return Point(-1, -1);
  }
}

/**
 * \brief counts the number of bits set to 1 in a 2 bytes integer
 * @param word
 */
inline int popcount(uint16_t word)
{
  // efficient implementation of popcount() using bit tricks.
  // For reference:
  // https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
  // https://en.wikipedia.org/wiki/Hamming_weight
  word = (word & 0x5555) + ((word & 0xAAAA) >> 1);
  word = (word & 0x3333) + ((word & 0xCCCC) >> 2);
  word = (word & 0x0F0F) + ((word & 0xF0F0) >> 4);
  word = (word & 0x00FF) + ((word & 0xFF00) >> 8);
  return word;
}

class SquareSetMask
{
public:
  SquareSetMask(SquareSetMaskFlag val = NONE)
    : m_value(val)
  {
  }

  typedef unsigned int val_t;

  explicit SquareSetMask(val_t val)
    : m_value(val)
  {
  }

  val_t value() const
  {
    return m_value;
  }

  int count() const
  {
    return popcount(value());
  }

  SquareSetMask& operator=(const SquareSetMask&) = default;
  SquareSetMask& operator|=(const SquareSetMask& other);

  operator bool() const
  {
    return m_value != 0;
  }

  bool operator!() const
  {
    return m_value == 0;
  }

  SquareSetMask operator~() const
  {
    val_t mask = (static_cast<val_t>(BOTTOM_RIGHT) << 1) - 1;
    return SquareSetMask(value() ^ mask);
  }

private:
  val_t m_value = 0;
};

inline SquareSetMask& SquareSetMask::operator|=(const SquareSetMask& other)
{
  m_value |= other.value();
  return *this;
}

inline SquareSetMask operator|(const SquareSetMask& lhs, const SquareSetMask& rhs)
{
  return SquareSetMask(lhs.value() | rhs.value());
}

inline SquareSetMask operator|(const SquareSetMaskFlag& lhs, const SquareSetMaskFlag& rhs)
{
  using val_t = SquareSetMask::val_t;
  return SquareSetMask(static_cast<val_t>(lhs) | static_cast<val_t>(rhs));
}

inline SquareSetMask operator|(const SquareSetMask& lhs, const SquareSetMaskFlag& rhs)
{
  using val_t = SquareSetMask::val_t;
  return SquareSetMask(lhs.value() | static_cast<val_t>(rhs));
}

inline SquareSetMask operator&(const SquareSetMask& lhs, const SquareSetMask& rhs)
{
  return SquareSetMask(lhs.value() & rhs.value());
}

inline SquareSetMask operator&(const SquareSetMaskFlag& lhs, const SquareSetMaskFlag& rhs)
{
  using val_t = SquareSetMask::val_t;
  return SquareSetMask(static_cast<val_t>(lhs) & static_cast<val_t>(rhs));
}

inline SquareSetMask operator&(const SquareSetMask& lhs, const SquareSetMaskFlag& rhs)
{
  using val_t = SquareSetMask::val_t;
  return SquareSetMask(lhs.value() & static_cast<val_t>(rhs));
}

inline SquareSetMask operator^(const SquareSetMask& lhs, const SquareSetMask& rhs)
{
  return SquareSetMask(lhs.value() ^ rhs.value());
}

inline SquareSetMask operator^(const SquareSetMaskFlag& lhs, const SquareSetMaskFlag& rhs)
{
  using val_t = SquareSetMask::val_t;
  return SquareSetMask(static_cast<val_t>(lhs) ^ static_cast<val_t>(rhs));
}

/**
 * \brief a set of square within a 3x3 region
 *
 * A SquareSet is a set of at most 9 squares, all located in a 3x3 region.
 * The 2D coordinates of the top-left square of the region are stored in \a x and
 * \a y.
 * The squares that actually belong to the set are specified by the \a mask.
 *
 * This class is used extensively by the solver to perform its deductions.
 *
 * A square set is said to be in normalized form if it has at least one
 * square in either its left column or its top row; or if it is empty.
 */
class SquareSet
{
public:
  int x, y; // the position of the TOP_LEFT square
  SquareSetMask mask;

public:
  SquareSet() = default;
  SquareSet(int xmin, int ymin, SquareSetMask mask);

  SquareSet(int x, int y, const SquareSet& other);

  void normalize();
  SquareSet normalized() const;

  int xmin() const;
  int ymin() const;

  int xmax() const;
  int ymax() const;
};

bool operator<(const SquareSet& lhs, const SquareSet& rhs);

SquareSet operator&(const SquareSet& lhs, const SquareSet& rhs);
SquareSet operator-(const SquareSet& lhs, const SquareSet& rhs);

template<typename F>
void foreachSquare(const SquareSet& set, F&& fn) {
  for (SquareSetMaskFlag flag : enumerateSquareSetMaskFlags()) {
    if (set.mask & flag) {
      Point relpos = mf2pt(flag);
      fn(set.x + relpos.x, set.y + relpos.y);
    }
  }
}

} // namespace minesweeper

#endif // MINESWEEPER_SQUARESET_H
