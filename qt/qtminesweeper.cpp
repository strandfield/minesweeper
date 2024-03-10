#include "qtminesweeper.h"

#include "minesweeper.h"

#include <QTimerEvent>

#include <QJsonArray>

QMinesweeperGame::QMinesweeperGame(QObject* parent)
  : QObject(parent)
{

}

QMinesweeperGame::~QMinesweeperGame() {}

/**
 * \brief returns the current state of the game
 */
QMinesweeperGame::GameState QMinesweeperGame::gameState() const
{
  return m_state;
}

/**
 * \brief returns whether the game has started
 */
bool QMinesweeperGame::started() const
{
  return gameState() == GameState::Started;
}

/**
 * \brief returns whether the game is finished
 */
bool QMinesweeperGame::finished() const
{
    return gameState() == GameState::Won || gameState() == GameState::Lost;
}

void QMinesweeperGame::setGameState(GameState gstate)
{
  if (m_state != gstate) {
    m_state = gstate;
    Q_EMIT gameStateChanged();
  }
}

/**
 * \brief returns the size of the grid
 *
 * The size of the grid can be changed before the game has started
 * with changeConfig().
 */
QSize QMinesweeperGame::gridSize() const
{
  return m_grid_size;
}

void QMinesweeperGame::setGridSize(const QSize& gsize)
{
  if (m_grid_size != gsize) {
    m_grid_size = gsize;
    Q_EMIT gridSizeChanged();
  }
}

/**
 * \brief returns the number of mines in the grid
 *
 * The number of mines can be changed before the game has started
 * with changeConfig().
 */
int QMinesweeperGame::mineCount() const
{
  return m_mine_count;
}

void QMinesweeperGame::setMineCount(int n)
{
  if (m_mine_count != n) {
    m_mine_count = n;
    Q_EMIT mineCountChanged();
  }
}

/**
 * \brief returns the number of squares marked with a flag
 */
int QMinesweeperGame::flagCount() const
{
  return m_flag_count;
}

void QMinesweeperGame::setFlagCount(int n)
{
  if (m_flag_count != n) {
    m_flag_count = n;
    Q_EMIT flagCountChanged();
  }
}

/**
 * \brief returns the number of squares that have been opened
 */
int QMinesweeperGame::uncoveredCount() const
{
  return m_uncovered_count;
}

void QMinesweeperGame::setUncoveredCount(int n)
{
  if (m_uncovered_count != n) {
    m_uncovered_count = n;
    Q_EMIT uncoveredCountChanged();
  }
}

/**
 * \brief returns the number of seconds that have elapsed since the game started
 */
int QMinesweeperGame::secondsElapsed() const
{
  return m_seconds_elapsed;
}

void QMinesweeperGame::setSecondsElapsed(int secs)
{
  if (secs != m_seconds_elapsed) {
    m_seconds_elapsed = secs;
    Q_EMIT secondsElapsedChanged();
  }
}

/**
 * \brief returns a representation in json of the knowledge grid
 *
 * The return value is in an array of integers describing the
 * player's knowledge of the grid, in row-major order.
 */
QJsonValue QMinesweeperGame::grid() const
{
  using It = std::vector<minesweeper::PlayerKnowledge>::const_iterator;

  auto to_js_array = [](It begin, It end) -> QJsonArray {
    QJsonArray result;
    while (begin != end) {
      result.append(static_cast<int>(*(begin++)));
    }
    return result;
  };

  if (m_game) {
    return to_js_array(m_game->gameData().grid.begin(), m_game->gameData().grid.end());
  } else {
    auto values = std::vector<minesweeper::PlayerKnowledge>(gridSize().width() * gridSize().height(),
                                                            minesweeper::PlayerKnowledge::Unknown);
    return to_js_array(values.begin(), values.end());
  }
}

/**
 * \brief returns a representation in json of the mines grid
 *
 * The return value is in an array of booleans describing the
 * location of the mines within the grid, in row-major order.
 * This information should be kept secret from the player.
 */
QJsonValue QMinesweeperGame::mines() const
{
  using It = std::vector<bool>::const_iterator;

  auto to_js_array = [](It begin, It end) -> QJsonArray {
    QJsonArray result;
    while (begin != end)
      result.append(bool(*(begin++)));
    return result;
  };

  if (m_game) {
    return to_js_array(m_game->gameData().mines.begin(), m_game->gameData().mines.end());
  } else {
    auto values = std::vector<bool>(gridSize().width() * gridSize().height(), false);
    return to_js_array(values.begin(), values.end());
  }
}

/**
 * \brief change the configuration of the game
 * \param gsize   new size of the grid
 * \param mcount  new number of mines in the grid
 *
 * If the game has already started, this does nothing.
 */
void QMinesweeperGame::changeConfig(const QSize& gsize, int mcount)
{
  if (gameState() != GameState::NotStarted)
    restart();

  m_grid_size = gsize;
  Q_EMIT gridSizeChanged();

  m_mine_count = mcount;
  Q_EMIT mineCountChanged();

  Q_EMIT gridChanged();
}

/**
 * \brief open a square
 * \param x  x-coordinate of the square
 * \param y  y-coordinate of the square
 *
 * If the game has not started yet, the grid is generated before the
 * square is opened, and the game timer starts.
 *
 * The number of uncovered squares is updated as well as the game
 * state if the player wins or loses.
 *
 * \sa uncoveredCount(), gameState() and openNeighbors().
 */
void QMinesweeperGame::open(int x, int y)
{
  if (x < 0 || x >= gridSize().width() || y < 0 || y >= gridSize().height())
    return;

  if (gameState() == GameState::NotStarted) {
    initGame(x, y);
  } else {
    m_game->openSquare(x, y);
    Q_EMIT gridChanged();
    updateGameState();
  }

  setUncoveredCount(m_game->countUncovered());
}

/**
 * \brief open the adjacent squares of an already opened square
 * \param x  x-coordinate of the square
 * \param y  y-coordinate of the square
 *
 * If the game hasn't started yet, this does nothing.
 * If there are not enough flags on the adjacent squares to account
 * for the mine count of the square, this also does nothing.
 *
 * Otherwise, this is roughly the same as calling open() on each
 * adjacent, non-opened and non-marked square of the input square.
 */
void QMinesweeperGame::openNeighbors(int x, int y)
{
  if (!started())
    return;

  if (!m_game->gameData().grid.contains(x, y))
    return;

  m_game->openAdjacentSquares(x, y);
  Q_EMIT gridChanged();
  updateGameState();
  setUncoveredCount(m_game->countUncovered());
}

/**
 * \brief toggles the flag on a square
 * \param x  x-coordinate of the square
 * \param y  y-coordinate of the square
 *
 * If the target square has already been opened, this does nothing.
 */
void QMinesweeperGame::toggleMark(int x, int y)
{
  if (m_game && m_game->toggleMark(x, y)) {
    Q_EMIT gridChanged();

    int delta = m_game->gameData().grid.at(x, y) == minesweeper::PlayerKnowledge::MarkedAsMine ? 1
                                                                                               : -1;
    setFlagCount(flagCount() + delta);
  }
}

/**
 * \brief ends the current game so that a new one can be started
 *
 * This function should probably be renamed "reset" as it does not
 * actually starts a new game.
 */
void QMinesweeperGame::restart()
{
  if (m_timer_id != -1) {
    killTimer(m_timer_id);
    m_timer_id = -1;
  }

  m_game.reset();
  setGameState(NotStarted);
  Q_EMIT minesChanged();
  Q_EMIT gridChanged();

  setFlagCount(0);
  setUncoveredCount(0);
  setSecondsElapsed(0);
}

void QMinesweeperGame::timerEvent(QTimerEvent* ev)
{
  if (ev->timerId() == m_timer_id) {
    setSecondsElapsed(secondsElapsed() + 1);
    ev->accept();
  } else {
    QObject::timerEvent(ev);
  }
}

void QMinesweeperGame::initGame(int sx, int sy)
{
  minesweeper::GameParams params;
  params.width = gridSize().width();
  params.height = gridSize().height();
  params.minecount = mineCount();
  params.sx = sx;
  params.sy = sy;

  m_game = std::make_unique<minesweeper::Game>(params);
  m_game->openSquare(sx, sy);

  qDebug() << m_game->gameData().seed;

  setGameState(GameState::Started);
  Q_EMIT minesChanged();
  Q_EMIT gridChanged();

  m_timer_id = startTimer(1000);
}

void QMinesweeperGame::updateGameState()
{
  if (!m_game) {
    setGameState(GameState::NotStarted);
  } else {
    if (m_game->gameData().dead)
      setGameState(GameState::Lost);
    else if (m_game->gameData().won)
      setGameState(GameState::Won);
    else
      setGameState(GameState::Started);

    if (gameState() == GameState::Lost || gameState() == GameState::Won) {
      if (m_timer_id != -1) {
        killTimer(m_timer_id);
        m_timer_id = -1;
      }

      if (gameState() == GameState::Won) {
        setFlagCount(mineCount());
        setUncoveredCount(gridSize().width() * gridSize().height() - mineCount());
      }
    }
  }
}
