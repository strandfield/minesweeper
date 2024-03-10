#include "solver.h"

#include "game.h"
#include "perturbator.h"

#include <array>
#include <cassert>
#include <list>
#include <map>

namespace minesweeper {

class SetStoreElement : public SquareSet
{
public:
  int mines = 0;
  bool todo = false;
  SetStoreElement* prev = nullptr;
  SetStoreElement* next = nullptr;

public:
  SetStoreElement(int xmin, int ymin, SquareSetMask mask, int minecount = 0) :
      SquareSet(xmin, ymin, mask),
      mines(minecount)
  {

  }
};

/**
 * \brief a collection of localized square sets with a mine count
 *
 * This class maps each SquareSet it contains to a mine count and is
 * used extensively by the solver to perform its deductions.
 * For example, if a SquareSet containing 5 squares is given a mine count of 5,
 * then the solver can easily deduce that all squares in the set must be marked
 * with a mine-flag.
 *
 * This is kind of a hybrid structure: it acts both as a tree (std::map) containing
 * all the element and a linked list (the todo list) referencing elements not
 * yet processed by the solver.
 *
 * Internally, the SetStore stores the sets in normalized form to ensure
 * it contains no "duplicates".
 *
 * I chose to keep Tatham's terminology, "SetStore", even though it could
 * probably be improved to better reflect what this class does.
 */
class SetStore
{
public:
  using Container = std::map<SquareSet, SetStoreElement>;
  using Iterator = Container::iterator;
  using ConstIterator = Container::const_iterator;

  /**
   * \brief returns the number of elements in the set store
   */
  size_t size() const
  {
    return m_elements.size();
  }

  /**
   * \brief returns an iterator to the beginning of the set store
   */
  ConstIterator begin() const
  {
    return m_elements.begin();
  }

  /**
   * \brief returns the end iterator
   */
  ConstIterator end() const
  {
    return m_elements.end();
  }

  /**
   * \brief add an element to the set store
   * \param x      x coordinate of the square set
   * \param y      y coordinate of the square set
   * \param mask   mask of the square set
   * \param mines  number of mines in the resulting square set
   *
   * This function constructs a SquareSet from its parameters and, if the
   * store does not yet contain such set, inserts it in the tree.
   *
   * The mine count @a mines is attached to the set and is added to the
   * todo-list (if it was inserted in the tree).
   */
  void add(int x, int y, SquareSetMask mask, int mines)
  {
    SetStoreElement s{ x, y, mask, mines };
    s.normalize();

    auto p = m_elements.insert(std::make_pair((SquareSet)s, s));

    if (p.second) // inserting successful
    {
      // insert new element in todo list
      add_todo(p.first);
    }
  }

  /**
   * \brief adds an element from the set store to the todo list
   * \param it  iterator to the element
   *
   * If the element is already in the todo list, this does nothing.
   */
  void add_todo(Iterator it)
  {
    SetStoreElement& elem = it->second;

    if (elem.todo)
      return;

    if (m_todolist.tail != nullptr)
    {
      m_todolist.tail->next = &elem;
      elem.prev = m_todolist.tail;
    }
    else
    {
      m_todolist.head = &elem;
    }

    m_todolist.tail = &elem;
    m_todolist.size += 1;
    elem.todo = true;
  }

  /**
   * \brief returns whether the store contains an entry for a given set
   * \param s  the square set
   */
  bool contains(const SquareSet& s) const
  {
    return find(s) != m_elements.end();
  }

  /**
   * \brief find an element in the set store
   * \param s  the square set to look for
   * \return an iterator to the square set
   *
   * Note that the resulting iterator may reference a SquareSet
   * that is equivalent, yet not stricly identical to the provided SquareSet @a s.
   * This is because the SetStore stores the set in normalized form.
   */
  ConstIterator find(SquareSet s) const
  {
    s.normalize();
    return m_elements.find(s);
  }

  /**
   * \brief removes an element from the store
   * \param it  iterator referencing the element
   *
   * This also removes the element from the todo-list (if present).
   */
  void erase(ConstIterator it)
  {
    if (it != end())
    {
      const SetStoreElement& elem = it->second;

      if (elem.todo)
      {
        // remove elmenet from the todo list
        if (elem.prev)
        {
          elem.prev->next = elem.next;
        }
        else
        {
          m_todolist.head = elem.next;
        }

        if (elem.next)
        {
          elem.next->prev = elem.prev;
        }
        else
        {
          m_todolist.tail = elem.prev;
        }

        m_todolist.size -= 1;
      }

      m_elements.erase(it);
    }
  }

  /**
   * \brief returns the next element of the todo-list
   *
   * If the todo list is empty, nullptr is returned;
   * otherwise, the element is removed from the todo-list and
   * returned as a raw pointer.
   *
   * Ownership of the element remains to the SetStore, so do not \c delete the element manually.
   */
  SetStoreElement* next_todo()
  {
    if (m_todolist.size == 0)
      return nullptr;

    SetStoreElement* ret = m_todolist.head;
    m_todolist.head = ret->next;
    if (ret->next)
    {
      ret->next->prev = nullptr;
    }
    else
    {
      m_todolist.tail = nullptr;
    }

    m_todolist.size -= 1;
    ret->todo = false;
    ret->next = nullptr;
    ret->prev = nullptr;
    return ret;
  }

  // TODO: add overload that takes a single square (just x,y)
  /**
   * \brief returns iterators to the elements that overlap with a given SquareSet
   * \param x     x coordinate of the square set
   * \param y     y coordinate of the square set
   * \param mask  mask of the square set
   */
  std::vector<Iterator> overlap(int x, int y, SquareSetMask mask)
  {
    SquareSet input{x, y, mask};

    std::vector<Iterator> result;

    for (int xx = x - 2; xx < x + 3; ++xx) {
      for (int yy = y - 2; yy < y + 3; ++yy) {
        SquareSet searchkey{ xx, yy, SquareSetMask() };

        auto it = m_elements.lower_bound(searchkey);

        while (it != end() && it->first.x == xx && it->first.y == yy)
        {
          // It is geometrically possible that the set reference by 'it'
          // overlaps with the input one.
          // We need to compute the intersection to know if they actually
          // overlap.
          if ((it->second & input).mask) {
            // At least one square in common: there is an overlap!
            result.push_back(it);
          }

          ++it;
        }
      }
    }

    return result;
  }

private:
  Container m_elements;

  /**
   * \brief bookkeeping structure for the todo-list linked list
   */
  struct
  {
    /**
     * \brief the head of the todo list
     */
    SetStoreElement* head = nullptr;

    /**
     * \brief the tail of the todo list
     */
    SetStoreElement* tail = nullptr;

    /**
     * \brief size of the todo list
     */
    size_t size = 0;
  } m_todolist;
};

/**
 * \brief returns the number of elements in a SetStore
 * \param ss  the set store
 */
size_t setstore_size(const SetStore& ss)
{
  return ss.size();
}

/**
 * \brief returns the SquareSet of an element of a SetStore
 * \param ss  the set store
 * \param i   the index of the element
 */
SquareSet setstore_at(const SetStore& ss, size_t i)
{
  auto it = std::next(ss.begin(), i);
  return it->first;
}

Solver::Solver(Perturbator* perturbator)
    : m_perturbator(perturbator)
{
}

using SquareTodo = std::list<size_t>;

static SquareTodo build_square_todolist(const Grid<PlayerKnowledge>& grid)
{
  SquareTodo list;

  for (size_t i(0); i < grid.size(); ++i) {
    if (grid.at(i) != PlayerKnowledge::Unknown) {
      list.push_back(i);
    }
  }

  return list;
}

static void mark_known_squares(Game& thegame,
                          SquareTodo& std,
                          int x,
                          int y,
                          SquareSetMask mask,
                          bool mine)
{
  GameData& game = thegame.gameData();
  auto& grid = game.grid;

  foreachSquare(SquareSet{x, y, mask}, [&grid, mine, &thegame, &std](int xx, int yy) {
    // it is possible that the square is already known because
    // it was deduced as such when processing an element earlier in
    // the todo-list.
    if (grid.at(xx, yy) != PlayerKnowledge::Unknown) {
      // in which case we do not mark it again
      return;
    }

    if (mine)
    {
      grid.set(xx, yy, PlayerKnowledge::MarkedAsMine);
    }
    else
    {
      grid.set(xx, yy, static_cast<PlayerKnowledge>(thegame.mineLookup(xx, yy)));
    }

    std.push_back(pt2idx(grid, xx, yy));
  });
}

static void mark_known_square(Game& thegame,
                               SquareTodo& std,
                               int x,
                               int y,
                               bool mine)
{
  mark_known_squares(thegame, std, x, y, SquareSetMaskFlag::TOP_LEFT, mine);
}

static void process_newly_known_squares(SquareTodo& squareTodo, Grid<PlayerKnowledge>& grid, SetStore& ss)
{
  for (size_t i : squareTodo)
  {
    Point pos = idx2pt(grid, i);
    int& x = pos.x;
    int& y = pos.y;

    if (grid[i] >= PlayerKnowledge::Mine0)
    {
      int mines = static_cast<int>(grid[i]);

      struct Neighbor
      {
        SquareSetMaskFlag flag;
        Point deltapos;
      };

      SquareSetMask mask = SquareSetMaskFlag::NONE;

      static const std::array<Neighbor, 8> neighbors
          = { Neighbor{ SquareSetMaskFlag::TOP_LEFT, { -1, -1 } },
             Neighbor{ SquareSetMaskFlag::TOP, { 0, -1 } },
             Neighbor{ SquareSetMaskFlag::TOP_RIGHT, { 1, -1 } },
             Neighbor{ SquareSetMaskFlag::LEFT, { -1, 0 } },
             Neighbor{ SquareSetMaskFlag::RIGHT, { 1, 0 } },
             Neighbor{ SquareSetMaskFlag::BOTTOM_LEFT, { -1, 1 } },
             Neighbor{ SquareSetMaskFlag::BOTTOM, { 0, 1 } },
             Neighbor{ SquareSetMaskFlag::BOTTOM_RIGHT, { 1, 1 } } };

      for (const Neighbor& neighbor : neighbors)
      {
        Point neighborpos = Point(x + neighbor.deltapos.x, y + neighbor.deltapos.y);

        if (!grid.contains(neighborpos))
          continue;

        if (grid[neighborpos] == PlayerKnowledge::MarkedAsMine)
          --mines;
        else if (grid[neighborpos] == PlayerKnowledge::Unknown)
          mask |= neighbor.flag;
      }

      if (mask)
      {
        ss.add(x - 1, y - 1, mask, mines);
      }
    }

    // we are going to remove this known square from all existing square sets
    // in the set store.
    // in practice, this means removing such square sets and replacing them
    // with a new one.
    {
      std::vector<SetStore::Iterator> list
          = ss.overlap(x, y, SquareSetMaskFlag::TOP_LEFT);

      for (auto it : list)
      {
        const SetStoreElement& s = it->second;

        SquareSetMask newmask
            = (s - SquareSet(x, y, SquareSetMaskFlag::TOP_LEFT)).mask;

        int newmines = s.mines - (grid[i] == PlayerKnowledge::MarkedAsMine);

        if (newmask)
        {
          ss.add(s.x, s.y, newmask, newmines);
        }

        ss.erase(it);
      }
    }
  }

  squareTodo.clear();
}

void process_next_todo(const SetStoreElement& s, Game& thegame, SquareTodo& stodo, SetStore& ss)
{
  // check the trivial cases of zero mines or mine-count equals square-count
  if (s.mines == 0 || s.mines == s.mask.count()) {
    // all the squares in the set can be marked as known
    mark_known_squares(thegame, stodo, s.x, s.y, s.mask, (s.mines != 0));

    // because all of its squares are now known, the set will eventually be
    // removed from the set store, we can stop now.
    // TODO: check if we could not simply remove it now
    return;
  }

  // find all sets that overlap with the our current set.
  std::vector<SetStore::Iterator> list = ss.overlap(s.x, s.y, s.mask);

  for (auto setstoreiterator : list) {
    const SetStoreElement& s2 = setstoreiterator->second;

    // Find the non-overlapping parts of (s2 - s) and (s - s2).
    // Tatham refers to these as 'wings' so we will keep the terminology.
    SquareSetMask swing = (s - s2).mask;
    SquareSetMask s2wing = (s2 - s).mask;
    int swc = swing.count();
    int s2wc = s2wing.count();

    // Quoting Tatham:
    // > If one set has more mines than the other, and
    // > the number of extra mines is equal to the
    // > cardinality of that set's wing, then we can mark
    // > every square in the wing as a known mine, and
    // > every square in the other wing as known clear.
    if (swc == s.mines - s2.mines || s2wc == s2.mines - s.mines) {
      mark_known_squares(thegame, stodo, s.x, s.y, swing, (swc == s.mines - s2.mines));
      mark_known_squares(thegame, stodo, s2.x, s2.y, s2wing, (s2wc == s2.mines - s.mines));
      continue;
    }

    // > Failing that, see if one set is a subset of the
    // > other. If so, we can divide up the mine count of
    // > the larger set between the smaller set and its
    // > complement, even if neither smaller set ends up
    // > being immediately clearable.
    if (swc == 0 && s2wc != 0)
    {
      // s is a subset of s2
      assert(s2.mines > s.mines);
      ss.add(s2.x, s2.y, s2wing, s2.mines - s.mines);
    }
    else if (s2wc == 0 && swc != 0)
    {
      // s2 is a subset of s
      assert(s.mines > s2.mines);
      ss.add(s.x, s.y, swing, s.mines - s2.mines);
    }
  }
}

static bool attempt_global_deduction(int n, int squaresleft, int minesleft, Game& thegame, Grid<PlayerKnowledge>& grid, SquareTodo& stodo, SetStore& ss)
{
  // simple case: no mines left, or as many mines as there are squares.
  if (minesleft == 0 || minesleft == squaresleft) {
    for (size_t i(0); i < grid.size(); ++i) {
      if (grid[i] == PlayerKnowledge::Unknown){
        Point pos = idx2pt(grid, i);
        mark_known_square(
            thegame, stodo, pos.x, pos.y, minesleft != 0);
      }
    }

    return true;
  }

  // list all the sets in the setstore
  std::vector<const SetStoreElement*> sets;
  sets.reserve(ss.size());
  for (auto it = ss.begin(); it != ss.end(); ++it)
    sets.push_back(&(it->second));

  // We have to find a suitable subset of the SetStore on which a
  // deduction can be made.
  // Since we do not know in advance which sets are going to be used,
  // we pretty much have to enumerate all the subsets until we can find
  // one that is okay.
  // Such problem is usually "easily" solved by a recursive function.
  // Here however, Tatham chose to implement a virtual "on-stack" recursion.
  // This makes the algorithm harder to understand but is nonetheless
  // quite interesting. So we will keep it largely that way.
  // What follows is therefore a C++ implementation of Simon Tatham's
  // global deduction algorithm, with some of the original comments
  // kept for reference.

  // > Failing that, we have to do some _real_ work.
  // > Ideally what we do here is to try every single
  // > combination of the currently available sets, in an
  // > attempt to find a disjoint union (i.e. a set of
  // > squares with a known mine count between them) such
  // > that the remaining unknown squares _not_ contained
  // > in that union either contain no mines or are all
  // > mines.
  // >
  // > Actually enumerating all 2^n possibilities will get
  // > a bit slow for large n, so I artificially cap this
  // > recursion at n=10 to avoid too much pain.
  constexpr int setused_size = 10;
  bool setused[setused_size];
  int nsets = ss.size();
  if(nsets > setused_size) return false;

  // > Doing this with actual recursive function calls
  // > would get fiddly because a load of local
  // > variables from this function would have to be
  // > passed down through the recursion. So instead
  // > I'm going to use a virtual recursion within this
  // > function. The way this works is:
  // >
  // >  - we have an array `setused', such that setused[n]
  // >    is true if set n is currently in the union we
  // >    are considering.
  // >
  // >  - we have a value `cursor' which indicates how
  // >    much of `setused' we have so far filled in.
  // >    It's conceptually the recursion depth.
  // >
  // > We begin by setting `cursor' to zero. Then:
  // >
  // >  - if cursor can advance, we advance it by one. We
  // >    set the value in `setused' that it went past to
  // >    true if that set is disjoint from anything else
  // >    currently in `setused', or to false otherwise.
  // >
  // >  - If cursor cannot advance because it has
  // >    reached the end of the setused list, then we
  // >    have a maximal disjoint union. Check to see
  // >    whether its mine count has any useful
  // >    properties. If so, mark all the squares not
  // >    in the union as known and terminate.
  // >
  // >  - If cursor has reached the end of setused and the
  // >    algorithm _hasn't_ terminated, back cursor up to
  // >    the nearest true entry, reset it to false, and
  // >    advance cursor just past it.
  // >
  // >  - If we attempt to back up to the nearest 1 and
  // >    there isn't one at all, then we have gone
  // >    through all disjoint unions of sets in the
  // >    list and none of them has been helpful, so we
  // >    give up.
  for(int cursor = 0;;)
  {
    if (cursor < nsets)
    {
      bool ok = true;

      // See if any existing used set overlaps with the one at the cursor.
      for (int i = 0; i < cursor; i++)
      {
        if (setused[i] && (*sets[cursor] & *sets[i]).mask)
        {
          ok = false;
          break;
        }
      }

      // we add the set to the union if it does not overlap with any
      // set already in the union.
      setused[cursor] = ok;

      if (ok)
      {
        // if the set was added to the union, we need to adjust
        // minesleft and squaresleft.
        minesleft -= sets[cursor]->mines;
        squaresleft -= sets[cursor]->mask.count();
      }

      ++cursor; // go on to the next set
    }
    else
    {
      // we have reached the end. all sets in the store have been taken
      // into account and are either in the union or not.
      // now see if we got anything interesting.
      if (squaresleft > 0 && (minesleft == 0 || minesleft == squaresleft))
      {
        // > We have! There is at least one
        // > square not contained within the set
        // > union we've just found, and we can
        // > deduce that either all such squares
        // > are mines or all are not (depending
        // > on whether minesleft==0). So now all
        // > we have to do is actually go through
        // > the grid, find those squares, and
        // > mark them.
        for (size_t i(0); i < grid.size(); ++i)
        {
          if (grid[i] == PlayerKnowledge::Unknown)
          {
            bool outside = true; // is the square outside the union?
            int y = idx2pt(grid, i).y;
            int x = idx2pt(grid, i).x;

            for (int j(0); j < nsets; ++j)
            {
              if (setused[j]
                  && (*sets[j] & SquareSet(x, y, SquareSetMaskFlag::TOP_LEFT)).mask)
              {
                outside = false;
                break;
              }
            }

            if (outside) {
              mark_known_square(thegame, stodo, x, y, minesleft != 0);
            }
          }
        }

        return true;
      }

      // > If we reach here, then this union hasn't
      // > done us any good, so move on to the
      // > next. Backtrack cursor to the nearest 1,
      // > change it to a 0 and continue.
      while (--cursor >= 0 && !setused[cursor])
        ;

      if (cursor >= 0)
      {
        assert(setused[cursor]);

        // > We're removing this set from our
        // > union, so re-increment minesleft and
        // > squaresleft.
        minesleft += sets[cursor]->mines;
        squaresleft += sets[cursor]->mask.count();

        setused[cursor++] = false;
      }
      else
      {
        // > We've backtracked all the way to the
        // > start without finding a single 1,
        // > which means that our virtual
        // > recursion is complete and nothing
        // > helped.
        break;
      }
    }
  }

  return false;
}

static bool perturb_grid(Perturbator& perturbator, Game& thegame, Grid<PlayerKnowledge>& grid, SquareTodo& stodo, SetStore& ss)
{
  std::vector<Perturbation> perturbations = perturbator.perturb(thegame, ss);

  if (perturbations.empty()) { // the perturbator did nothing
    return false;
  }

  // the perturbator changed the grid in some way.
  // we use the perturbations set to update the internal
  // data of the solver.

  for (const Perturbation& p : perturbations) {
    if (p.delta == Perturbation::Cleared
        && grid.at(p.x, p.y) != PlayerKnowledge::Unknown) {
      // the square was known to be a mine and is now known
      // to be empty.
      // we add it to the list of newly known square.
      stodo.push_back(pt2idx(grid, p.x, p.y));
    }

    // we then look at all sets which overlap with the changed square
    // and update their mine count.
    // we also add them back to the todo list as the deductions we can
    // make based on them may have changed too.

    std::vector<SetStore::Iterator> list
        = ss.overlap(p.x, p.y, SquareSetMaskFlag::TOP_LEFT);

    for (auto it : list) {
      it->second.mines += static_cast<int>(p.delta);
      ss.add_todo(it);
    }
  }

  return true;
}

bool Solver::solve(Game& thegame)
{
  GameData& game = thegame.gameData();
  int n = std::count(game.mines.begin(), game.mines.end(), true); // TODO: add option in the solver to ignore total mine count
  auto& grid = game.grid;
  SquareTodo stodo = build_square_todolist(grid); // a list of newly known squares
  SetStore ss;

  for (;;) {
    process_newly_known_squares(stodo, grid, ss);

    // attempt deductions from the next element in the SetStore todo-list
    SetStoreElement* next_todo = ss.next_todo();
    if (next_todo) {
      process_next_todo(*next_todo, thegame, stodo, ss);
      continue;
    } else {
      // the todo list is empty...

      // scan the grid to see how many unknown squares are left
      int squaresleft = std::count(grid.begin(), grid.end(), PlayerKnowledge::Unknown);
      int minesleft = n - std::count(grid.begin(), grid.end(), PlayerKnowledge::MarkedAsMine);

      if (squaresleft == 0) { // if there are no unknown squares left, we are finished!
        assert(minesleft == 0); // there shouldn't be any mines left too
        break;
      }

      if (n >= 0) {
        // The todo list is empty and we still have unknown squares left.
        // We will have to attempt global deductions based on the total
        // mine count.
        // We only resort to this and everything else failed because this
        // is computationally expensive.
        bool success = attempt_global_deduction(n, squaresleft, minesleft, thegame, grid, stodo, ss);

        if(success) {
          continue;
        }
      }
    }

    // if we reach that point, that means the solver is stuck.
    // our last chance is to modify the grid in order to make
    // it workable.
    // we only do this if a perturbator was provided though,
    // which is typically the case when we are actually trying
    // to generate a solvable grid.
    if (m_perturbator) {
      bool success = perturb_grid(*m_perturbator, thegame, grid, stodo, ss);

      if (success)
        continue;
      else
        break;
    }

    // nothing worked, we have to give up.
    break;
  }

  // if the solver succeeded, there are no unknown squares left in the grid
  return std::find(grid.begin(), grid.end(), PlayerKnowledge::Unknown) == grid.end();
}

} // namespace minesweeper
