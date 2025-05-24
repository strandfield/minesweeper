#ifndef MINESWEEPER_SOLVER_H
#define MINESWEEPER_SOLVER_H

#include "gamedata.h"

namespace minesweeper {

class Game;

class Perturbator;
class SetStore;
class SquareSet;

size_t setstore_size(const SetStore& ss);
SquareSet setstore_at(const SetStore& ss, size_t i);

/**
 * \brief the solver class
 */
class Solver
{
public:
  explicit Solver(Perturbator* perturbator = nullptr);

  bool solve(Game& game);

private:
  Perturbator* m_perturbator = nullptr;
};


} // namespace minesweeper

#endif // MINESWEEPER_SOLVER_H
