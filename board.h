#ifndef __BOARD_H__
#define __BOARD_H__
#include <string>
#include "bitmanip.h"
using namespace std;

/* Construct a board from a FEN notation */
Board fenToBoard(const char *fen);

/* Get the algebraic notation for every move on this board. This is best done for all the moves at once, because there can be ambiguities.
  m is the array of all moves possible on the given board. */
void getAlgebraicNotation(Board *b, Move *m, int numMoves, string *san);

/* Make move m on the board b, modifying b. */
void makeMove(Board* b, Move m);

#endif
