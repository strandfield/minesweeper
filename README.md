
# Minesweeper

An implementation of the Minesweeper game in C++ supporting generation of grids of 
arbitrary sizes and with guaranteed solvability; ensuring that the player never 
needs to guess or, worse, lands on a mine at the very first click.

The source code of the generator, written in pure C++, is available in the `src`
directory.

The `qt` and `qml` directories provide the sources for a Qt/QML user interface for 
the game that can be built in QtCreator with `minesweeper.pro`.
