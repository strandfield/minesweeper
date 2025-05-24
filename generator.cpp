#include "generator.h"

#include "game.h"
#include "perturbator.h"
#include "solver.h"

#include <cassert>

namespace minesweeper {

class BuiltinPerturbator : public Perturbator
{
private:
  bool m_allow_big_perturbs = false;

public:
  using Perturbator::Perturbator;

  void reset(int ntries) final
  {
    Perturbator::reset(ntries);
    m_allow_big_perturbs = ntries > 100;
  }

  std::vector<Perturbation> do_perturb(Game& ctx,
                                       int x,
                                       int y,
                                       SquareSetMask mask) final
  {
    return mineperturb(ctx, ctx.gameData().grid, x, y, mask, rng(), m_allow_big_perturbs);
  }
};

static bool minegen(Grid<bool>& ret, int n, int x, int y, bool unique, RandomEngine& rng, Perturbator* perturbator)
{
  int w = ret.width(), h = ret.height();
  bool success = false;
  int ntries = 0;

  do
  {
    ntries++;

    std::fill(ret.begin(), ret.end(), false);

    // Place 'n' mines randomly, but not within one square of (x,y).
    {
      std::vector<size_t> minespos; // possible locations for the mines

      // compute the list of possible locations for the mines
      for (int i(0); i < h; ++i) {
        for (int j(0); j < w; ++j) {
          if (std::abs(i - y) > 1 || std::abs(j - x) > 1) {
            minespos.push_back(pt2idx(ret, j, i));
          }
        }
      }

      // keep 'n' random squares
      std::shuffle(minespos.begin(), minespos.end(), rng);
      minespos.resize(n);

      // place the mines
      for (size_t i : minespos)
        ret[i] = true;
    }

    /*
         * Now set up a results grid to run the solver in, and a
         * context for the solver to open squares. Then run the solver
         * repeatedly; if the number of perturb steps ever goes up or
         * it ever returns -1, give up completely.
         *
         * We bypass this bit if we're not after a unique grid.
         */
    if (unique) { // we are after an unique grid.
      // We setup a fake game on which we will run the solver repeatedly to verify
      // that the grid is indeed solvable.

      GameData gamedata;
      gamedata.grid = Grid<PlayerKnowledge>(w, h, PlayerKnowledge::Unknown);
      gamedata.mines = ret;
      gamedata.params.sx = x;
      gamedata.params.sy = y;
      Game game{ std::move(gamedata) };

      int nbperturbs = -1;

      for (;;) {
        // reset the knowledge grid
        std::fill(game.gameData().grid.begin(), game.gameData().grid.end(), PlayerKnowledge::Unknown);
        game.gameData().grid.set(x, y, static_cast<PlayerKnowledge>(game.mineLookup(x, y)));

        // the random generation never places mines near the starting point
        assert(game.gameData().grid.at(x, y) == PlayerKnowledge::Mine0);

        perturbator->reset(ntries);
        // We give the solver a perturbator so that the grid can be modified
        // if it gets stuck.
        Solver solver{ perturbator };
        bool solved = solver.solve(game); // try to solve the game

        if (!solved || (nbperturbs >= 0 && perturbator->useCount() >= nbperturbs)) {
          // we couldn't solve the grid, or had to increase the number of perturbations
          // to do so.
          // give up with this grid, restart generation completely from scratch.
          success = false;
          break;
        }

        nbperturbs = perturbator->useCount();

        if (nbperturbs == 0) { // we solved the grid without using the perturbator
          // this grid is good!
          ret = game.gameData().mines;
          success = true;
          break;
        }
      }
    } else {
      success = true;
    }

  } while (!success);

  return success;
}

static Grid<bool> minegen(int w, int h, int n, int x, int y, bool unique, RandomEngine& rng, Perturbator* perturbator)
{
  auto ret = Grid<bool>(w, h);
  minegen(ret, n, x, y, unique, rng, perturbator);
  return ret;
}

Generator::Generator()
    : m_rng{},
    m_owned_perturbator{ std::make_unique<BuiltinPerturbator>(m_rng) },
    m_perturbator{ m_owned_perturbator.get() }
{

}

Generator:: Generator(Perturbator& perturbator)
  : m_perturbator{&perturbator}
{

}

Generator::~Generator()
{

}

Grid<bool> Generator::generate(const GameParams& params)
{
  std::random_device::result_type s = 0;

  if(params.seed != 0) {
    s = params.seed;
  } else {
    if (m_seed == 0) {
      seed();
    }
    s = m_seed;
  }

  m_rng.seed(s);

  return minegen(params.width, params.height, params.minecount, params.sx, params.sy, params.unique, m_rng, m_perturbator);
}

std::random_device::result_type Generator::seed()
{
  std::random_device rd;
  m_seed = rd.entropy() > 0 ? rd() : std::rand();
  return m_seed;
}

} // namespace minesweeper
