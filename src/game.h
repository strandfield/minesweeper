#ifndef MINESWEEPER_GAME_H
#define MINESWEEPER_GAME_H

#include "gamedata.h"

namespace minesweeper {

/**
 * \brief main game api
 */
class Game
{
private:
  GameData m_data;

public:
  explicit Game(const GameParams& params);
  explicit Game(GameData d);

  GameData& gameData();

  bool started() const;
  bool dead() const;
  bool won() const;
  bool finished() const;

  int mineLookup(int x, int y) const;

  void openSquare(int x, int y);
  void openAdjacentSquares(int x, int y);
  bool toggleMark(int x, int y);

  int countUncovered() const;
};

} // namespace minesweeper

#endif // MINESWEEPER_GAME_H
