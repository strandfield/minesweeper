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

  GameData& gameData(); // TODO: make private
  const GameData& gameData() const;

  bool generated() const;
  void generate(int sx, int sy);

  bool dead() const;
  bool won() const;
  bool finished() const;

  const Grid<PlayerKnowledge>& grid() const;

  int mineLookup(int x, int y) const;

  void openSquare(int x, int y);
  void openAdjacentSquares(int x, int y);
  bool toggleMark(int x, int y);

  int countUncovered() const;
  int countFlags() const;
};

inline const GameData& Game::gameData() const
{
  return m_data;
}

} // namespace minesweeper

#endif // MINESWEEPER_GAME_H
