#ifndef QTMINESWEEPER_H
#define QTMINESWEEPER_H

#include <QObject>

#include <QJsonValue>
#include <QSize>

#include <memory>

namespace minesweeper {
struct GameData;
class Game;
} // namespace minesweeper

/**
 * \brief Qt frontend for the minesweeper game
 */
class QMinesweeperGame : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GameState gameState READ gameState NOTIFY gameStateChanged)
    Q_PROPERTY(bool finished READ finished NOTIFY gameStateChanged)
    Q_PROPERTY(QSize gridSize READ gridSize NOTIFY gridSizeChanged)
    Q_PROPERTY(int mineCount READ mineCount NOTIFY mineCountChanged)
    Q_PROPERTY(QJsonValue grid READ grid NOTIFY gridChanged)
    Q_PROPERTY(QJsonValue mines READ mines NOTIFY minesChanged)
    Q_PROPERTY(int flagCount READ flagCount NOTIFY flagCountChanged)
    Q_PROPERTY(int uncoveredCount READ uncoveredCount NOTIFY uncoveredCountChanged)
    Q_PROPERTY(int secondsElapsed READ secondsElapsed NOTIFY secondsElapsedChanged)
public:
    explicit QMinesweeperGame(QObject* parent = nullptr);
    ~QMinesweeperGame();

    /**
     * \brief describes the state of the game
     */
    enum GameState { NotStarted = 0, Started, Won, Lost };
    Q_ENUM(GameState);

    GameState gameState() const;
    bool started() const;
    bool finished() const;

    QSize gridSize() const;
    int mineCount() const;

    QJsonValue grid() const;
    QJsonValue mines() const;

    int flagCount() const;
    int uncoveredCount() const;

    int secondsElapsed() const;

    Q_INVOKABLE void changeConfig(const QSize& gsize, int mcount);

    Q_INVOKABLE void open(int x, int y);
    Q_INVOKABLE void openNeighbors(int x, int y);
    Q_INVOKABLE void toggleMark(int x, int y);

    Q_INVOKABLE void restart();

Q_SIGNALS:
    void gameStateChanged();
    void gridSizeChanged();
    void mineCountChanged();
    void gridChanged();
    void minesChanged();
    void flagCountChanged();
    void uncoveredCountChanged();
    void secondsElapsedChanged();

protected:
    void timerEvent(QTimerEvent *ev) override;

protected:
    void setGameState(GameState gstate);
    void setGridSize(const QSize &gsize);
    void setMineCount(int n);
    void setFlagCount(int n);
    void setUncoveredCount(int n);
    void setSecondsElapsed(int secs);

private:
    void initGame(int sx, int sy);
    void updateGameState();

private:
    GameState m_state = GameState::NotStarted;
    QSize m_grid_size = QSize(9, 9);
    int m_mine_count = 10;
    std::unique_ptr<minesweeper::Game> m_game;
    int m_flag_count = 0;
    int m_uncovered_count = 0;
    int m_seconds_elapsed = 0;
    int m_timer_id = -1;
};

#endif // QTMINESWEEPER_H
