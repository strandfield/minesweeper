#ifndef MINESWEEPER_GENERATOR_H
#define MINESWEEPER_GENERATOR_H

#include "gamedata.h"

#include <random>

namespace minesweeper {

class Perturbator;

/**
 * \brief generates solvable grids
 */
class Generator
{
private:
  std::random_device::result_type m_seed;
  std::default_random_engine m_rng;
  std::unique_ptr<Perturbator> m_owned_perturbator;
  Perturbator* m_perturbator;

public:
  Generator();
  explicit Generator(Perturbator& perturbator);
  ~Generator();

  Grid<bool> generate(const GameParams& params);

  std::random_device::result_type seed();
};

} // namespace minesweeper

#endif // MINESWEEPER_GENERATOR_H
