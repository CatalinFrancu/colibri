#ifndef __EGTB_H__
#define __EGTB_H__
#include <string>
#include "defines.h"
#include "lrucache.h"

extern LruCache egtbCache;

/* Initializes the endgame tables */
void initEgtb();

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

/* Get the size of the table for the given piece set, ignoring possible en passant situations */
int getEgtbSize(PieceSet *ps, int numPieceSets);

/* Get the size of the table for en passant situations. Returns 0 if one side has no pawns.
 * Considering the E-W symmetry, there are 7 placements for a WP and BP for the side to move, so 14 for both sides.
 * After the WP and BP are placed, 60 usable squares remain (44 for pawns). That is because the two squares behind
 * the pushed pawn must also remain empty. */
int getEpEgtbSize(PieceSet *ps, int numPieceSets);

/* Get the index of this position within its EGTB table. Assumes b is rotated into its canonical position. */
unsigned getEgtbIndex(PieceSet *ps, int nps, Board *b);

/* Get the index of this position within its EGTB table when the EP bit is set.
 * Assumes b is mirrored into its canonical position.
 * EP positions are appended after all the non-EP ones, so this function adds getEgtbSize() to its result. */
unsigned getEpEgtbIndex(PieceSet *ps, int nps, Board *b);

/* Generate and write to file the endgame tablebase for the given combo.
 * Returns true if it actually generated something, false if the file was already there. */
bool generateEgtb(const char *combo);

/**
 * Queries the EGTB for this position. Also handles two corner cases: (1) no
 * white or black pieces (game is won/lost now); (2) too many pieces. Takes
 * care of canonicalization. Returns the score shifted by 1. Returns INFTY on
 * errors (missing EGTB file etc.).
 * Clobbers b.
*/
int egtbLookup(Board *b);

/* Queries the EGTB for this position. The caller must pass some extra information (this information is identical over large numbers of queries
 * during EGTB generation / verification). Takes care of canonicalization, but assumes the sides are already correct.
 * Returns the score shifted by 1. Returns INFTY on errors (missing EGTB file, more than EGTB_MEN pieces on the board etc.).
 * Clobbers b. */
int egtbLookupWithInfo(Board *b, const char *combo, PieceSet *ps, int nps);

/* Takes a board and sets/returns four values:
 * - an array of move names listing all the legal moves
 * - an array of FEN-encoded boards listing the corresponding resulting positions
 * - an array of scores for the boards resulting after each of the above moves
 * - the number of such moves
 * - the score of the board b itself.
 */
int batchEgtbLookup(Board *b, string *moveNames, string *fens, int *scores, int *numMoves);

/* Verifies the correctness of an EGTB table:
 * - Generates all the possible positions, canonical and non-canonical
 * - Asserts that the value of each position is consistent with the values of its child positions.
 */
void verifyEgtb(const char *combo);

/* Compresses the combo.egt file into a combo.egt.xz and combo.idx. Does nothing if the table is already compressed. */
void compressEgtb(const char *combo);

/* Generates all EGTB where white has wc pieces and black has bc pieces */
void generateAllEgtb(int wc, int bc);

#endif
