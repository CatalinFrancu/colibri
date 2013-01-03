#ifndef __BOARD_H__
#define __BOARD_H__
#include <string>
#include "bitmanip.h"
using namespace std;

/* Prints a board */
void printBoard(Board *b);

/* Construct an empty board */
void emptyBoard(Board *b);

/* Returns the number of pieces on the board */
int getPieceCount(Board *b);

/* Rotate the board as specified. Does not change the side to move. */
void rotateBoard(Board *b, int orientation);

/* Construct a board from a FEN notation */
Board fenToBoard(const char *fen);

/* Get the algebraic notation for every move on this board. This is best done for all the moves at once, because there can be ambiguities.
  m is the array of all moves possible on the given board. */
void getAlgebraicNotation(Board *b, Move *m, int numMoves, string *san);

/* Make move m on the board b, modifying b. */
void makeMove(Board* b, Move m);

/* Statically evaluates a board:
 * - if one side has no pieces left, it wins
 * - otherwise this is a draw from the static eval's point of view. */
int evalBoard(Board *b);

#endif
