#include "game.h"

#include "generator.h"

#include <cassert>

namespace minesweeper {

Game::Game(const GameParams& params)
{
  m_data.params = params;
}

Game::Game(GameData d) :
    m_data(std::move(d))
{

}

GameData& Game::gameData()
{
  return m_data;
}

bool Game::started() const
{
  return !m_data.mines.empty();
}

bool Game::dead() const
{
  return m_data.dead;
}

bool Game::won() const
{
  return m_data.won;
}

bool Game::finished() const
{
  return won() || dead();
}

enum OpenSquareResult
{
  KilledByMine = -1,
  OK = 0
};


int open_square(Game& thegame, int x, int y)
{
  GameData& game = thegame.gameData();
  int w = game.params.width, h = game.params.height;

  assert(!game.mines.empty());

  if (game.mines.at(x, y)) { // It's a mine.
    // You lose.
    game.dead = true;
    game.grid.set(x, y, PlayerKnowledge::MineHit);
    return KilledByMine;
  }

  // It's safe.
  // Count the number of mines in the adjacent squares and update
  // the grid.
  int nbmines = thegame.mineLookup(x, y);
  game.grid.set(x, y, static_cast<PlayerKnowledge>(nbmines));

  // If the opened square has a mine count of zero, we want to
  // open automatically all adjacent squares; and we want to do
  // so recursively.
  // We use a 'todo' list of squares which neighbors we have to open.
  std::vector<size_t> todo;

  if (nbmines == 0) {
    todo.push_back(pt2idx(game.grid, x, y));
  }

  // Open all adjacent squares of squares with a mine count of zero.
  while (!todo.empty()) {
    size_t i = todo.back();
    todo.pop_back();

    auto& grid = game.grid;

    int xx = idx2pt(grid, i).x;
    int yy = idx2pt(grid, i).y;

    for (int dx = -1; dx <= +1; dx++) {
      for (int dy = -1; dy <= +1; dy++) {
        if (grid.contains(xx + dx, yy + dy)
            && grid.at(xx + dx, yy + dy) == PlayerKnowledge::Unknown) {
          nbmines = thegame.mineLookup(xx + dx, yy + dy);
          assert(nbmines != -1); // check that the square is not a mine

          grid.set(xx + dx, yy + dy, static_cast<PlayerKnowledge>(nbmines));

          if (nbmines == 0) { // we just opened a square with a mine count of zero
            // add it to the todo list so that its neighbors will
            // also be opened.
            todo.push_back(pt2idx(grid, xx + dx, yy + dy));
          }
        }
      }
    }
  }

  // Finally, we are going to check if the player has opened all
  // the empty squares, in which case it's a WIN!

  // Can't win if you are already dead.
  if (game.dead) {
    return OK;
  }

  // We count the number of covered squares and the number of mines.
  int nmines = 0, ncovered = 0;
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      if (game.grid.at(xx, yy) < 0) {
        ncovered++;
      }
      if (game.mines.at(xx, yy)) {
        nmines++;
      }
    }
  }
  assert(ncovered >= nmines);

  // If the numbers match, it means the player has opened all empty
  // squares.
  // In such case, we mark all remaining unknown squares as mines and
  // mark the game as being won!
  if (ncovered == nmines)
  {
    for (int yy = 0; yy < h; yy++) {
      for (int xx = 0; xx < w; xx++) {
        if (game.grid.at(xx, yy) < 0) {
          game.grid.set(xx, yy, PlayerKnowledge::MarkedAsMine);
        }
      }
    }
    game.won = true;
  }

  return OK;
}

/**
 * \brief computes the number of mines surrounding a square
 * \param x  x-coordinate of the square
 * \param y  y-coordinate of the square
 * \return the mine count (an integer ranging from 0 to 8)
 * \pre the coordinates must specify a square within the grid
 *
 * If the square is in fact a mine, this function returns -1.
 */
int Game::mineLookup(int x, int y) const
{
  if (m_data.mines.at(x, y)) {
    return -1; // it's a mine!
  }

  int n = 0;

  for (int i = -1; i <= +1; ++i) {
    for (int j = -1; j <= +1; ++j) {
      if (!m_data.mines.contains(x + i, y + j) || (i == 0 && j == 0)) {
        continue;
      }
      if (m_data.mines.at(x + i, y + j)) {
        ++n;
      }
    }
  }

  return n;
}

/**
 * \brief opens a square
 * \param x  x-coordinate of the square
 * \param y  y-coordinate of the square
 *
 * This function generates the grid the first time it is called
 * so that we can be certain that the player won't open a mine
 * on its first click.
 *
 * If the square is a mine, the game is lost.
 * If the square contains no mine and its mine count is zero,
 * all adjacent squares are opened recursively.
 *
 * When all non-mined squares have been opened, the game is won.
 */
void Game::openSquare(int x, int y)
{
  if (m_data.mines.empty()) { // if the the grid has not been generated yet...
    // ... generate one
    m_data.params.sx = x;
    m_data.params.sy = y;

    Generator generator;
    m_data.seed = generator.seed();
    m_data.mines = generator.generate(m_data.params);

    if (m_data.grid.size() != m_data.mines.size()) {
      m_data.grid = Grid<PlayerKnowledge>(m_data.mines.geom(), PlayerKnowledge::Unknown);
    }
  }

  open_square(*this, x, y);
}

/**
 * \brief open all squares adjacent to a given square
 * \param x  x-coordinate of the square
 * \param y  y-coordinate of the square
 *
 * If the square is marked as a mine, or outside the grid, this function does nothing.
 *
 * Also, if the number of squares marked as mined in the adjacent squares is not the
 * same as the square's mine count, this does nothing.
 *
 * If there is a mine among the squares that are going to be opened (because
 * the user incorrectly marked as square as mined), only that square is
 * opened and the game is lost.
 *
 * \sa openSquare()
 */
void Game::openAdjacentSquares(int x, int y)
{
  const auto& grid = m_data.grid;
  const auto& mines = m_data.mines;

  if (!grid.contains(x, y) || grid.at(x, y) == PlayerKnowledge::MarkedAsMine) {
    return;
  }

  // count the mine markers in adjacent squares
  int n = 0;
  for (int dy = -1; dy <= +1; dy++) {
    for (int dx = -1; dx <= +1; dx++) {
      if (grid.contains(x + dx, y + dy)
          && grid.at(x + dx, y + dy) == PlayerKnowledge::MarkedAsMine) {
        ++n;
      }
    }
  }

  // if the number of markers matches with the displayed mine count
  // we start uncovering the squares.
  if (n == static_cast<int>(grid.at(x, y)))
  {
    // check if there is a mine among the squares
    for (int dy = -1; dy <= +1; dy++) {
      for (int dx = -1; dx <= +1; dx++) {
        if (mines.contains(x + dx, y + dy)) {
          if (grid.at(x + dx, y + dy) != PlayerKnowledge::MarkedAsMine
              && mines.at(x + dx, y + dy)) {
            // if so, opens the incorrect square and let the player lose.
            open_square(*this, x + dx, y + dy);
            return;
          }
        }
      }
    }

    // otherwise, the squares are all safe so we can open them all.
    for (int dy = -1; dy <= +1; dy++) {
      for (int dx = -1; dx <= +1; dx++) {
        if (grid.contains(x + dx, y + dy)
            && grid.at(x + dx, y + dy) == PlayerKnowledge::Unknown) {
          open_square(*this, x + dx, y + dy);
        }
      }
    }
  }
}

/**
 * \brief toggles the mark on a square
 * \param x  x-coordinate of the square
 * \param y  y-coordinate of the square
 * \return whether a mark was actually toggled
 *
 * If the coordinates do not identify a square within the grid, or
 * if the square has already been opened, this does nothing.
 */
bool Game::toggleMark(int x, int y)
{
  if (!m_data.grid.contains(x, y))
    return false;

  if (m_data.grid.at(x, y) == PlayerKnowledge::MarkedAsMine) {
    m_data.grid.set(x, y, PlayerKnowledge::Unknown);
  } else if (m_data.grid.at(x, y) == PlayerKnowledge::Unknown) {
    m_data.grid.set(x, y, PlayerKnowledge::MarkedAsMine);
  }

  return m_data.grid.at(x, y) == PlayerKnowledge::MarkedAsMine
         || m_data.grid.at(x, y) == PlayerKnowledge::Unknown;
}

/**
 * \brief counts the number of squared that have been opened
 */
int Game::countUncovered() const
{
  return std::count_if(m_data.grid.begin(), m_data.grid.end(), [](PlayerKnowledge k) {
    return static_cast<int>(k) >= 0 && static_cast<int>(k) <= 8;
  });
}

} // namespace minesweeper
