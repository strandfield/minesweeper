
# Minesweeper

An implementation of the Minesweeper game in C++ supporting generation of grids of 
arbitrary sizes and with guaranteed solvability; ensuring that the player never 
needs to guess or, worse, lands on a mine at the very first click.

This implementation is heavily inspired by "Mines" from 
[Simon Tatham's Puzzle Collection](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/),
distributed under the MIT license. <br/>
Source code in this folder is also distributed under the MIT license.

`minesweeper.js` provides a `drawMinefield()` function that can be used
to render a grid in a HTML "canvas" element (i.e. [CanvasRenderingContext2D](https://developer.mozilla.org/fr/docs/Web/API/CanvasRenderingContext2D)).
