#ifndef __BOARD_H__
#define __BOARD_H__
#include <string>
#include "bitmanip.h"
using namespace std;

/* Prints a board */
void printBoard(Board *b);

/* Construct an empty board */
void emptyBoard(Board *b);

/* This is expensive -- only use for debugging */
bool equalBoard(Board *b1, Board *b2);

/* Returns true iff m1 and m2 are the same */
bool equalMove(Move m1, Move m2);

/* Returns the number of pieces on the board */
int getPieceCount(Board *b);

/* Rotate and/or flip the board as specified. Does not change the side to move. */
void transformBoard(Board *b, int tr);

/* Changes sides by flipping the board N-S and changing the color of all the pieces */
void changeSides(Board *b);

/* Changes sides if needed for an EGTB lookup (e.g. Black has more pieces than White) */
/* wp, bp: popcounts (saves recomputing them inside the function) */
void changeSidesIfNeeded(Board *b, int wp, int bp);

/* Two things need to happen before we refer to the EP index:
 * 1. Naturally, the EP bit must be set
 * 2. There needs to be a pawn that can execute the EP capture. We don't index boards where this isn't the case,
 * because the result is identical to the non-EP position. */
inline bool epCapturePossible(Board *b) {
  if (!b->bb[BB_EP]) {
    return false;
  }
  if (b->side == WHITE) {
    return b->bb[BB_WP] & RANK_5 & ((b->bb[BB_EP] >> 9) ^ (b->bb[BB_EP] >> 7));
  } else {
    return b->bb[BB_BP] & RANK_4 & ((b->bb[BB_EP] << 9) ^ (b->bb[BB_EP] << 7));
  }
}

/* Returns true iff m is a capture on b (in which case any legal move on b is a capture) */
bool isCapture(Board *b, Move m);

/**
 * Tranform the board as needed to bring it into its canonical position.
 *
 * @param bool dryRun If true, leave the board unchanged.
 * @return int The transformation performed to canonicalize the board (which
 *   can be TR_NONE if the board is already canonical).
*/
int canonicalizeBoard(PieceSet *ps, int nps, Board *b, bool dryRun);

/**
 * Constructs a board from a FEN string.
 * @return True on success, false on failure.
 */
bool fenToBoard(const char* fen, Board* b);

/* Get the FEN notation for a board */
string boardToFen(Board *b);

/* Get the algebraic notation for every move on this board. This is best done for all the moves at once, because there can be ambiguities.
  m is the array of all moves possible on the given board. */
void getAlgebraicNotation(Board *b, Move *m, int numMoves, string *san);

/* Returns a move's algebraic notation. This is *slow*. It works by generating
   all the moves on board b, then all those moves' names. */
string getMoveName(Board* b, Move m);

/**
 * Returns a move's long notation. This is useful when the board is
 * unavailable.
 */
string getLongMoveName(Move m);

/* Make move m on the board b, modifying b */
void makeMove(Board *b, Move m);

/* Make backward move m on the board b, modifying b. See the remarks for getAllMoves() (no un-captures, no un-promotions etc.) */
void makeBackwardMove(Board *b, Move m);

/**
 * Encodes a move on 16 bits.
 */
u16 encodeMove(Move m);

/**
 * Decodes a 16-bit number into a move.
 */
Move decodeMove(u16 x);

#endif
