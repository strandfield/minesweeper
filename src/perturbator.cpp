#include "perturbator.h"

#include "game.h"
#include "solver.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <tuple>

namespace minesweeper {

/**
 * \brief a square outside of the input set that can be used by the Perturbator
 *
 * Each square is given a classification that quantifies how much we want to
 * use it for the perturbations.
 */
struct PerturbSquare
{
  /**
   * \brief describes the class of the square
   *
   * A lower value means a higher preference at using the square.
   */
  enum Type
  {
    /**
     * \brief an unknown square on the boundary of known squares
     */
    NearKnownSquare = 1,

    /**
     * \brief an unknown square beyond the boundary of known squares
     */
    InUnknownRegion = 2,

    /**
     * \brief a known square
     */
    KnownSquare = 3,
  };

  Type type; ///< the classification for the square
  int x, y; ///< coordinates of the square
};

/**
 * \brief build the list of squares that can be used by the perturbator
 * \param ctx   the game
 * \param grid  the grid
 * \param setx  x-coordinate of the input set
 * \param sety  y-coordinate of the input set
 * \param mask  mask of the input set
 * \param rng  random number generator
 * \return a sorted list of usable squares
 *
 * If \a mask is empty, the input set is interpreted as all unknown squares in the grid.
 * None of the squares from the input list can be part of the list built by this function.
 *
 * The squares within one square of the starting position are also excluded from the list
 * because the generator is required to create grids without any mines near the starting position.
 */
static std::vector<PerturbSquare> build_squarelist(const GameData& ctx, const Grid<PlayerKnowledge>& grid, int setx,
                                                           int sety, SquareSetMask mask, RandomEngine& rng)
{
  std::vector<PerturbSquare> sqlist;
  sqlist.reserve(grid.size());
  for (size_t i(0); i < grid.size(); ++i)
  {
    int x = idx2pt(grid, i).x, y = idx2pt(grid, i).y;

    // When generating a grid, we don't want to have any mines near the starting position,
    // so if the current square is too close to it, we don't put it on the list at all.
    if (std::abs(y - ctx.params.sy) <= 1 && std::abs(x - ctx.params.sx) <= 1)
      continue;

    // We also don't put on the list squares that belong to the input set!
    if ((!mask && grid[i] == PlayerKnowledge::Unknown)
        || (x >= setx && x < setx + 3 && y >= sety && y < sety + 3
            && (mask & static_cast<SquareSetMaskFlag>(1 << ((y - sety) * 3 + (x - setx))))))
      continue;

    PerturbSquare sq;
    sq.x = x;
    sq.y = y;

    // Now we have to find the classification for this square.
    if (grid[i] != PlayerKnowledge::Unknown)
    {
      sq.type = PerturbSquare::KnownSquare;
    }
    else
    {
      // The square is unknown.
      // We now want to distinguish between a square near a known square
      // and a square that is fully in the unknown regions.
      sq.type = PerturbSquare::InUnknownRegion;

      for (int dy = -1; dy <= +1; dy++) {
        for (int dx = -1; dx <= +1; dx++) {
          if (grid.contains(x + dx, y + dy)
              && grid.at(x + dx, y + dy) != PlayerKnowledge::Unknown) {
            sq.type = PerturbSquare::NearKnownSquare;
            break;
          }
        }
      }
    }

    sqlist.push_back(sq);
  }

  // group the squares by their classification
  std::sort(sqlist.begin(), sqlist.end(), [](const PerturbSquare& lhs, const PerturbSquare& rhs){
    return std::make_tuple(lhs.type, lhs.y, lhs.x) < std::make_tuple(rhs.type, rhs.y, rhs.x);
  });

  // then shuffle within each group
  for (auto it = sqlist.begin(); it != sqlist.end();)
  {
    auto end = std::upper_bound(it,
                                sqlist.end(),
                                *it,
                                [](const PerturbSquare& lhs, const PerturbSquare& rhs)
                                { return lhs.type < rhs.type; });

    std::shuffle(it, end, rng);

    it = end;
  }

  return sqlist;
}

static std::pair<size_t, size_t> count_full_and_empty(const SquareSet& s, const Grid<bool>& mines)
{
  size_t nfull = 0, nempty = 0;

  // ?TODO: use foreachSquare
  for (int dy = 0; dy < 3; dy++) {
    for (int dx = 0; dx < 3; dx++) {
      if (s.mask & pt2mf(dx, dy)) {
        assert(s.x + dx < mines.width());
        assert(s.y + dy < mines.height());
        if (mines.at(s.x + dx, s.y + dy))
          nfull++;
        else
          nempty++;
      }
    }
  }

  return std::make_pair(nfull, nempty);
}

static std::pair<size_t, size_t> count_full_and_empty_among_unknown_squares(const Grid<PlayerKnowledge>& grid, const Grid<bool>& mines)
{
  size_t nfull = 0, nempty = 0;

  for (size_t i(0); i < grid.size(); ++i)
  {
    if (grid.at(i) == PlayerKnowledge::Unknown)
    {
      if (mines.at(i))
        ++nfull;
      else
        ++nempty;
    }
  }

  return std::make_pair(nfull, nempty);
}

static std::vector<size_t> build_fill_list(const Grid<PlayerKnowledge>& grid, const Grid<bool>& mines, const SquareSet& inputset, size_t size, RandomEngine& rng)
{
  assert(size != 0);

  std::vector<size_t> filllist;
  filllist.reserve(grid.size());

  if (inputset.mask)
  {
    for (int dy = 0; dy < 3; dy++) {
      for (int dx = 0; dx < 3; dx++) {
        if (inputset.mask & pt2mf(dx, dy)) {
          assert(inputset.x + dx <= grid.width());
          assert(inputset.y + dy <= grid.height());
          if (!mines.at(inputset.x + dx, inputset.y + dy)) {
            filllist.push_back(pt2idx(grid, inputset.x + dx, inputset.y + dy));
          }
        }
      }
    }
  }
  else
  {
    for (size_t i(0); i < grid.size(); ++i) {
      if (grid.at(i) == PlayerKnowledge::Unknown) {
        if (!mines.at(i)) {
          filllist.push_back(i);
        }
      }
    }
  }

  // ensure we have found enough squares
  assert(filllist.size() >= size);
  // in the context in which this function is called,
  // we must in fact have more squares then requested
  assert(filllist.size() > size);

  std::shuffle(filllist.begin(), filllist.end(), rng);
  filllist.resize(size);

  return filllist;
}


static void apply_changes(Grid<PlayerKnowledge>& grid, Grid<bool>& mines, const std::vector<Perturbation>& perturbations)
{
  for (const Perturbation& p : perturbations)
  {
    int x = p.x;
    int y = p.y;
    Perturbation::Change delta = p.delta;

    // check that the perturbation is not nonsense.
    // i.e.. clearing a square that has no mine or adding a mine
    // to a square that already has one.
    assert((delta == Perturbation::Cleared) ^ (mines.at(x, y) == false));

    // make the change!
    mines.set(x, y, delta == Perturbation::ChangedToMine);

    // update the grid, that is, neighboring squares that are no longer
    // unknown.
    for (int dy = -1; dy <= +1; dy++) {
      for (int dx = -1; dx <= +1; dx++) {
        if (grid.contains(x + dx, y + dy)
            && grid.at(x + dx, y + dy) != PlayerKnowledge::Unknown) {
          if (dx == 0 && dy == 0) {
            // the square we just changed is marked as known.
            // this is something we try to avoid but that we may have to
            // do if nothing better is possible.
            if (delta == Perturbation::ChangedToMine) {
              // the square was empty and is now a mine.
              grid.set(x, y, PlayerKnowledge::MarkedAsMine);
            } else {
              // the square had a mine and is now empty.
              // we need to compute its "number".
              int minecount = 0;
              for (int dy2 = -1; dy2 <= +1; dy2++) {
                for (int dx2 = -1; dx2 <= +1; dx2++) {
                  if (grid.contains(x + dx2, y + dy2)
                      && mines.at(x + dx2, y + dy2)) {
                    minecount++;
                  }
                }
              }
              grid.set(x, y, static_cast<PlayerKnowledge>(minecount));
            }
          } else {
            // update the "number" of a neighbor square.
            if (grid.at(x + dx, y + dy) >= Mine0) {
              int minecount = static_cast<int>(grid.at(x + dx, y + dy));
              minecount += static_cast<int>(delta); // add or subtract 1 depending on the change
              grid.set(x + dx, y + dy, static_cast<PlayerKnowledge>(minecount));
            }
          }
        }
      }
    }
  }
}

/**
 * \brief builtin perturbation algorithm
 * \param game                the game
 * \param grid                the grid
 * \param setx                x-coordinate of the input set
 * \param sety                y-coordinate of the input set
 * \param mask                mask of the input set
 * \param rng                 random number generator
 * \param allow_big_perturbs  whether big perturbations are allowed, defaults to false
 *
 * If \a allow_big_perturbs is true, \a mask can be empty and the input
 * set is then all the unknown squares in the grid.
 * Otherwise \a mask must not be empty for this function to attempt
 * perturbing the grid.
 *
 * The general outline of the algorithm is due to Simon Tatham and is as follows:
 * - we count the number of mines and empty square in the input set
 * - we build a list of squares outside of the input that we can use for exchanging mines
 * - we search enough empty or full squares in the outside set to make the input set
 *   either empty or full of mines
 * - if we fail at that, we nonetheless try to fill the input set with as much mine as possible
 * - we build the vector of Perturbation describing the changes that we applied.
 *
 * Quoting Tatham:
 * > Allowing [big] perturbation [...] appears to make it
 * > guaranteeably possible to generate a workable grid for any mine
 * > density, but they tend to be a bit boring, with mines packed
 * > densely into far corners of the grid and the remainder being
 * > less dense than one might like. Therefore, to improve overall
 * > grid quality I disable this feature for the first few
 * > [grid generation] attempts, and fall back to it after no useful
 * > grid has been generated.
 */
std::vector<Perturbation> mineperturb(Game& game,
                                             Grid<PlayerKnowledge>& grid,
                                             int setx,
                                             int sety,
                                             SquareSetMask mask,
                                             RandomEngine& rng,
                                             bool allow_big_perturbs)
{

  if (!mask && !allow_big_perturbs)
    return {};

  GameData& ctx = game.gameData();

  // Compute the number of full (with a mine) and empty squares in the input set.
  // Depending on the mask, the input set is either the SquareSet or all unknown squares
  // in the grid.
  size_t nfull, nempty;
  std::tie(nfull, nempty) = mask ? count_full_and_empty(SquareSet(setx, sety, mask), ctx.mines) :
                                count_full_and_empty_among_unknown_squares(grid, ctx.mines);

  // Build a list of squares that are not in the input set and that we can
  // therefore use to swap mines with squares from the input set.
  // This 'sqlist' is sorted and has squares to use preferably at the beginning.
  std::vector<PerturbSquare> sqlist = build_squarelist(ctx, grid, setx, sety, mask, rng);

  // Find in 'sqlist' either 'nfull' empty squares (in which we would put mines)
  // or 'nempty' full squares (in which we would take mines).
  // Remember that the idea is to swap mines between the input set and PerturbSquare in 'sqlist'.
  std::vector<PerturbSquare*> tofill, toempty;
  tofill.reserve(std::min(sqlist.size(), (size_t)nfull));
  toempty.reserve(std::min(sqlist.size(), (size_t)nempty));
  for (PerturbSquare& sq : sqlist)
  {
    if (ctx.mines.at(sq.x, sq.y))
      toempty.push_back(&sq);
    else
      tofill.push_back(&sq);

    if (tofill.size() == nfull || toempty.size() == nempty)
      break;
  }

  // If we haven't found enough empty squares or full squares outside of the input set to either
  // completely empty or fill the input set, we'll have to settle for doing only a partial job.
  // In this case, we choose to fill the input set as much as possible and we therefore
  // need to build a list of empty squares in the input set.
  std::vector<size_t> filllist;
  if (tofill.size() != nfull && toempty.size() != nempty) {
    filllist = build_fill_list(grid, ctx.mines, SquareSet(setx, sety, mask), toempty.size(), rng);
  }

  // We now need to decide what to do (depending on what we *can* do).
  // a) move all mines in the input set to squares in the outside set
  // b) fill all (or at least some) empty squares in the input set with mines
  //    from the outside set.

  std::vector<PerturbSquare*> todo; // squares outside of the input set that we will change
  Perturbation::Change change; // what will happen to the squares in 'todo'
  if (tofill.size() == nfull) // if we have enough empty square to fill, we do that
  {
    todo = std::move(tofill);
    change = Perturbation::ChangedToMine;
    toempty.clear(); // no longer needed
  }
  else // otherwise we will put mines from outside the input set to the input set
  {
    // note that we also end up here if we've constructed a 'filllist'.
    todo = std::move(toempty);
    change = Perturbation::Cleared;
    tofill.clear();  // no longer needed
  }

  // 'toempty' and 'tofill' have served their purpose and are no longer needed
  assert(toempty.empty() && tofill.empty());

  std::vector<Perturbation> ret;
  ret.reserve(2 * todo.size()); // each change in the todo has a counterpart in the input set
  for (PerturbSquare* sq : todo)
  {
    ret.emplace_back();
    ret.back().x = sq->x;
    ret.back().y = sq->y;
    ret.back().delta = change;
  }

  // we will now compute the changes for the input set
  change = opposite(change);

  // if we have a non-empty 'filllist', that is what we will use
  if (!filllist.empty())
  {
    assert(change == Perturbation::ChangedToMine);

    // the input set is only going to be partially changed,
    // i.e. we are not going to either completely empty or fill it.
    // the squares that are going to be changed are listed in the 'filllist'.
    for (size_t i : filllist)
    {
      ret.emplace_back();
      ret.back().x = idx2pt(grid, i).x;
      ret.back().y = idx2pt(grid, i).y;
      ret.back().delta = change;
    }
  }
  else
  {
    // otherwise we consider the entirety of the input set.

    // a lambda function that computes the change that would be applied to a square
    // depending on whether it currently contains a mine.
    auto change_for = [](bool square_is_mined) {
      return square_is_mined ? Perturbation::Cleared : Perturbation::ChangedToMine;
    };

    if (mask) {
      for (int dy = 0; dy < 3; dy++) {
        for (int dx = 0; dx < 3; dx++) {
          if (mask & pt2mf(dx, dy)) {
            Perturbation::Change c = change_for(ctx.mines.at(setx + dx, sety + dy));
            if (change == c) {
              ret.emplace_back();
              ret.back().x = setx + dx;
              ret.back().y = sety + dy;
              ret.back().delta = change;
            }
          }
        }
      }
    } else {
      for (size_t i(0); i < grid.size(); ++i) {
        if (grid.at(i) == PlayerKnowledge::Unknown) {
          Perturbation::Change c = change_for(ctx.mines.at(i));
          if (change == c) {
            ret.emplace_back();
            ret.back().x = idx2pt(grid, i).x;
            ret.back().y = idx2pt(grid, i).y;
            ret.back().delta = change;
          }
        }
      }
    }
  }

  // check that we got the expected number of perturbations
  assert(ret.size() == 2 * todo.size());

  // Now is the time to actually apply the changes.

  // check we are not modifying a square near the starting point
  assert(std::none_of(ret.begin(), ret.end(), [&ctx](const Perturbation& p) {
    return std::abs(p.x - ctx.params.sx) <= 1 && std::abs(p.y - ctx.params.sy) <= 1;
  }));


  // Apply changes!
  apply_changes(grid, ctx.mines, ret);

  return ret;
}

/**
 * \brief construct a perturbator
 * \param rng  a random engine
 */
Perturbator::Perturbator(RandomEngine& rng)
    : m_rng(rng)
    , m_use_count(0)
{
}

Perturbator::~Perturbator()
{

}

/**
 * \brief returns the number of times the perturbator was used
 *
 * This value is incremented by one for every call to perturb().
 */
int Perturbator::useCount() const
{
  return m_use_count;
}

/**
 * \brief resets the use count to zero
 */
void Perturbator::resetUseCount()
{
  m_use_count = 0;
}

/**
 * \brief runs the perturbator on a set of squares
 * \param ctx   the game
 * \param x     x coordinate of the square set
 * \param y     y coordinate of the square set
 * \param mask  mask of the square set
 * \return a vector describing the perturbations that have been applied
 *
 * This function returns an empty vector if it failed to apply any perturbation.
 */
std::vector<Perturbation> Perturbator::perturb(Game& ctx, int x, int y, SquareSetMask mask)
{
  ++m_use_count;
  return do_perturb(ctx, x, y, mask);
}

/**
 * \brief runs the perturbator on a set of a set store
 * \param ctx  the game
 * \param ss   a set store
 * \return a vector describing the perturbations that have been applied
 *
 * This function selects a square set randomly from \a ss and calls
 * the other overload of perturb() with it.
 *
 * If the set store is empty, all the unknown squares in the grid are used
 * as the input set.
 *
 * This function is used by the Generator class.
 */
std::vector<Perturbation> Perturbator::perturb(Game& ctx, const SetStore& ss)
{
  size_t sssize = setstore_size(ss);

  if(sssize == 0)
  {
    return perturb(ctx, -1, -1, SquareSetMask());
  }
  else
  {
    size_t i = std::uniform_int_distribution<size_t>(0, sssize - 1)(rng());
    SquareSet s = setstore_at(ss, i);
    return perturb(ctx, s.x, s.y, s.mask);
  }
}

/**
 * \brief resets the perturbator
 * \param ntries  number of unsuccessful attempts by the Generator at generating a grid
 *
 * This function is used by the Generator class between attempts at solving a
 * randomly generated grid.
 * We want the Perturbator to be in a clean state when trying to solve a new
 * randomly generated grid.
 *
 * The default implementation simply resets the useCount().
 * Subclasses that overrides this function should call the base version
 * or manually reset the use count.
 */
void Perturbator::reset(int /* ntries */)
{
  resetUseCount();
}

RandomEngine& Perturbator::rng() const
{
  return m_rng;
}

/**
 * \brief virtual function that actually does the perturbation
 * \param ctx   the game
 * \param setx  x coordinate of the input set
 * \param sety  y coordinate of the input set
 * \param mask  mask of the input set
 *
 * The default implementation simply calls mineperturb().
 */
std::vector<Perturbation> Perturbator::do_perturb(Game& ctx, int setx, int sety, SquareSetMask mask)
{
  return mineperturb(ctx, ctx.gameData().grid, setx, sety, mask, rng());
}

} // namespace minesweeper
