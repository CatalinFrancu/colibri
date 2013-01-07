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

/* Changes sides by flipping the board N-S and changing the color of all the pieces */
void changeSides(Board *b);

/* Two things need to happen before we refer to the EP index:
 * 1. Naturally, the EP bit must be set
 * 2. There needs to be a pawn that can execute the EP capture. We don't index boards where this isn't the case,
 * because the result is identical to the non-EP position. */
bool epCapturePossible(Board *b);

/* Rotate the board as needed to bring it into canonical orientation */
void canonicalizeBoard(PieceSet *ps, int nps, Board *b);

/* Construct a board from a FEN notation */
Board fenToBoard(const char *fen);

/* Get the FEN notation for a board */
string boardToFen(Board *b);

/* Get the algebraic notation for every move on this board. This is best done for all the moves at once, because there can be ambiguities.
  m is the array of all moves possible on the given board. */
void getAlgebraicNotation(Board *b, Move *m, int numMoves, string *san);

/* Make move m on the board b, modifying b */
void makeMove(Board* b, Move m);

/* Make backward move m on the board b, modifying b. See the remarks for getAllMoves() (no un-captures, no un-promotions etc.) */
void makeBackwardMove(Board *b, Move m);

/* Statically evaluates a board:
 * - if one side has no pieces left, it wins
 * - otherwise this is a draw from the static eval's point of view. */
int evalBoard(Board *b);

#endif
