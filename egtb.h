#ifndef __EGTB_H__
#define __EGTB_H__
#include <string>
#include "defines.h"

/* Convert a combo to an array of PieceSet's in the order in which they should be placed on the board (and indexed) */
int comboToPieceSets(const char *combo, PieceSet *ps);

/* Encodes an EGTB board. This can be done in 31 bits for 5 or less pieces:
 * - 6 bits per piece encode the square on which every piece lies, in PieceSet order.
 * - 1 bit for side to move
 * - to encode the en passant square, if a pawn has just been pushed two spaces, put it on rank 1 or 8 instead of 4 or 5
 */
unsigned encodeEgtbBoard(PieceSet *ps, int nps, Board *b);

/* Decodes an EGTB board */
void decodeEgtbBoard(PieceSet *ps, int nps, Board *b, unsigned code);

/* Get the size of the table for the given piece set */
int getEgtbSize(PieceSet *ps, int numPieceSets);

/* Get the index of this position within its EGTB table. Assumes b is rotated into its canonical position. */
int getEgtbIndex(PieceSet *ps, int nps, Board *b);

/* Generate and write to file the endgame tablebase for the given combo */
void generateEgtb(const char *combo);

/* Queries the EGTB for this position. Takes care of canonicalization.
 * Returns the score shifted by 1. Returns INFTY on errors (missing EGTB file, more than EGTB_MEN pieces on the board etc.).
 * Clobbers b */
int egtbLookup(Board *b);

/* Takes a board and sets/returns four values:
 * - an array of move names listing all the legal moves
 * - an array of FEN-encoded boards listing the corresponding resulting positions
 * - an array of scores for the boards resulting after each of the above moves
 * - the number of such moves
 * - the score of the board b itself.
 */
int batchEgtbLookup(Board *b, string *moveNames, string *fens, int *scores, int *numMoves);

#endif
