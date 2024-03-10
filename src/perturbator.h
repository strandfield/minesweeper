#ifndef MINESWEEPER_PERTURBATOR_H
#define MINESWEEPER_PERTURBATOR_H

#include "gamedata.h"
#include "squareset.h"

#include <random>
#include <vector>

namespace minesweeper {

using RandomEngine = std::default_random_engine;

/**
 * \brief raw struct describing a perturbation in the grid
 */
struct Perturbation
{
  int x, y; ///< coordinate of the perturbed square

  /**
   * \brief describes the change applied to the square
   *
   * The numerical value for each enum value reflects the delta of mine count
   * for the square and and the adjacent squares.
   */
  enum Change
  {
    ChangedToMine = +1,
    Cleared = -1,
  };

  Change delta; ///< the change applied to the square
};

inline Perturbation::Change opposite(Perturbation::Change c)
{
  return static_cast<Perturbation::Change>(-1 * static_cast<int>(c));
}

class Game;
class SetStore;

std::vector<Perturbation> mineperturb(Game& game,
                                      Grid<PlayerKnowledge>& grid,
                                      int setx,
                                      int sety,
                                      SquareSetMask mask,
                                      RandomEngine& rng,
                                      bool allow_big_perturbs = false);

/**
 * \brief base class for a grid perturbator
 *
 * A perturbator may be used by the solver while generating grid
 * to move mines in order to make the grid solvable.
 *
 * Roughly speaking, grid generation works as follows: mines are placed
 * randomly within the grid and the solver is run on the grid to ensure
 * it is solvable.
 * If the solver gets stuck, it calls the perturbator in an attempt to
 * make the grid solvable.
 */
class Perturbator
{
public:
  explicit Perturbator(RandomEngine& rng);
  virtual ~Perturbator();

  int useCount() const;
  void resetUseCount();

  std::vector<Perturbation> perturb(Game& ctx, int x, int y, SquareSetMask mask);
  std::vector<Perturbation> perturb(Game& ctx, const SetStore& ss);

  virtual void reset(int ntries);

protected:
  RandomEngine& rng() const;

  virtual std::vector<Perturbation> do_perturb(Game& ctx,
                                               int x,
                                               int y,
                                               SquareSetMask mask);

private:
  RandomEngine& m_rng;
  int m_use_count = 0;
};

} // namespace minesweeper

#endif // MINESWEEPER_PERTURBATOR_H
