#ifndef MINESWEEPER_GRID_H
#define MINESWEEPER_GRID_H

#include "point.h"

#include <vector>

namespace minesweeper {

class GridGeom
{
private:
  int m_width = 0;
  int m_height = 0;

public:
  GridGeom() = default;

  GridGeom(int w, int h) :
      m_width{ w },
      m_height{ h }
  {

  }

  int width() const;
  int height() const;

  int area() const;
};

inline int GridGeom::width() const
{
  return m_width;
}

inline int GridGeom::height() const
{
  return m_height;
}

inline int GridGeom::area() const
{
  return width() * height();
}

inline size_t pt2idx(const GridGeom& geom, int x, int y)
{
  return static_cast<size_t>(y * geom.width() + x);
}

inline Point idx2pt(const GridGeom& geom, size_t index)
{
  auto idx = static_cast<int>(index);
  return Point{ idx % geom.width(), idx / geom.width() };
}


template<typename T>
class Grid
{
public:
  typedef T element_type;
  using Container = std::vector<T>;

  Grid() = default;
  Grid(const Grid<T>&) = default;

  Grid(int w, int h, const T& value = T())
  {
    resize(w, h, value);
  }

  Grid(const GridGeom& geom, const T& value = T())
      : Grid(geom.width(), geom.height(), value)
  {

  }

  const GridGeom& geom() const
  {
    return m_geom;
  }

  int width() const
  {
    return geom().width();
  }

  int height() const
  {
    return geom().height();
  }

  size_t size() const
  {
    return m_data.size();
  }

  bool empty() const
  {
    return m_data.empty();
  }

  void resize(int w, int h, const T& value = T())
  {
    m_geom = GridGeom{ w, h };
    m_data.resize(w * h, value);
  }

  bool contains(size_t index) const
  {
    return index < size();
  }

  bool contains(int x, int y) const
  {
    return x >= 0 && x < width() && y >= 0 && y < height();
  }

  bool contains(const Point& pos) const
  {
    return contains(pos.x, pos.y);
  }

  typename Container::const_reference at(size_t index) const
  {
    return m_data.at(index);
  }

  typename Container::const_reference at(const Point& pos) const
  {
    return m_data.at(pos.y * width() + pos.x);
  }

  typename Container::const_reference at(int x, int y) const
  {
    return m_data.at(y * width() + x);
  }

  void set(int x, int y, T val)
  {
    m_data[y * width() + x] = std::move(val);
  }

  typename Container::reference operator[](size_t index)
  {
    return m_data[index];
  }

  typename Container::const_reference operator[](size_t index) const
  {
    return m_data[index];
  }

  typename Container::reference operator[](const Point& pos)
  {
    return m_data[pos.y * width() + pos.x];
  }

  typename Container::const_reference operator[](const Point& pos) const
  {
    return m_data[pos.y * width() + pos.x];
  }

  typename Container::const_iterator begin() const
  {
    return m_data.begin();
  }

  typename Container::const_iterator end() const
  {
    return m_data.end();
  }

  typename Container::iterator begin()
  {
    return m_data.begin();
  }

  typename Container::iterator end()
  {
    return m_data.end();
  }

  Grid<T>& operator=(const Grid<T>&) = default;

private:
  GridGeom m_geom;
  Container m_data;
};

template<typename T>
size_t pt2idx(const Grid<T>& grid, const Point& pos)
{
  return pt2idx(grid.geom(), pos.x, pos.y);
}

template<typename T>
size_t pt2idx(const Grid<T>& grid, int x, int y)
{
  return pt2idx(grid.geom(), x, y);
}

template<typename T>
Point idx2pt(const Grid<T>& grid, size_t index)
{
  return idx2pt(grid.geom(), index);
}

} // namespace minesweeper

#endif // MINESWEEPER_GRID_H
