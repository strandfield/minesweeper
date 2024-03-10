#ifndef MINESWEEPER_GAMEDATA_H
#define MINESWEEPER_GAMEDATA_H

#include "grid.h"
#include "knowledge.h"

#include <random>

namespace minesweeper {

/**
 * \brief raw struct describing grid generation parameters
 */
struct GameParams
{
  int width;
  int height;
  int minecount;
  bool unique = true;
  int sx = -1, sy = -1;
  std::random_device::result_type seed = 0;
};

struct GameData
{
  GameParams params;
  std::random_device::result_type seed;
  // ?TODO: create GameState struct ?
  Grid<bool> mines;
  Grid<PlayerKnowledge> grid;
  bool dead = false;
  bool won = false;
};

} // namespace minesweeper

#endif // MINESWEEPER_GAMEDATA_H
