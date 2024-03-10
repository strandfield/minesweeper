#ifndef MINESWEEPER_KNOWLEDGE_H
#define MINESWEEPER_KNOWLEDGE_H

namespace minesweeper {

/**
 * \brief describes the knowledge the player has concerning a square
 */
enum PlayerKnowledge
{
  Unknown = -2,
  MarkedAsMine = -1,
  Mine0 = 0,
  Mine1,
  Mine2,
  Mine3,
  Mine4,
  Mine5,
  Mine6,
  Mine7,
  Mine8,
  MineRevealed = 64,
  MineHit,
  MineIncorrect
};

} // namespace minesweeper

#endif // MINESWEEPER_KNOWLEDGE_H
