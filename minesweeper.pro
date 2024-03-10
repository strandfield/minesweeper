TEMPLATE = app
TARGET = minesweeper

QT += core quick qml svg
INCLUDEPATH += $$PWD/src

HEADERS += \
    qt/qtminesweeper.h \
    src/point.h \
    src/grid.h \
    src/squareset.h \
    src/knowledge.h \
    src/gamedata.h \
    src/solver.h \
    src/perturbator.h \
    src/generator.h \
    src/game.h \
    src/minesweeper.h
SOURCES += qt/main.cpp \
    qt/qtminesweeper.cpp \
    src/squareset.cpp \
    src/solver.cpp \
    src/perturbator.cpp \
    src/generator.cpp \
    src/game.cpp \
    src/minesweeper.cpp
RESOURCES += minesweeper.qrc

QML_IMPORT_PATH = $$PWD/qml
