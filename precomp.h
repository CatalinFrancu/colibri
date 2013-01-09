#ifndef __PRECOMP_H__
#define __PRECOMP_H__
#include "bitmanip.h"
#include "board.h"

// choose[n][k] just holds {n choose k}
extern int choose[65][EGTB_MEN];

/* kingAttacks[sq] is a bitboard with the 8 (or less) king moves from sq set */
extern u64 kingAttacks[64];

/* knightAttacks[sq] is a bitboard with the 8 (or less) knight moves from sq set */
extern u64 knightAttacks[64];

/* isoMove[pos][occ] is an 8-bit configuration indicating what moves a piece has if it lies on position "pos" and the row
 * occupancy is "occ".  "iso" stands for "isotropic", since they do not depend on any direction. */
extern byte isoMove[8][64];

/* fileConfiguration[byte] unwraps the binary representation of byte on the "a" file of a bitboard.
 * The most significant bit ends up at a1 and the least significant bit ends up at a8. */
extern u64 fileConfiguration[256];

/* diagonal[sq] is a bitboard with bits set along the diagonal to which sq belongs. Likewise for antidiagonal[sq]. */
extern u64 diagonal[64];
extern u64 antidiagonal[64];

/* canonical64[k] holds information about every placement of a k-piece set.
 * If the placement with index i is canonical, then canonical64[k][i] holds its counter, otherwise it holds -1 times
 * the transformation that should be applied to this placement to obtain the canonical one.
 * For example, there are 10 canonical 1-piece placements and 278 canonical 2-piece placements. */
extern int* canonical64[EGTB_MEN / 2 + 1];
extern int numCanonical64[EGTB_MEN / 2 + 1];

/* Same, but assuming the set to be placed consists of pawns (so only 48 of the squares are legal).
 * Here we need up to EGTB-1 pieces, for combinations like KvPPPP */
extern int* canonical48[EGTB_MEN];
extern int numCanonical48[EGTB_MEN];

/* Egtb files are numbered sequentially starting from 0. There are 2,646 files for 5-men EGTB.
 * egtbFileSegment[i][j] gives the starting index of file numbers for configurations with i White pieces and j Black pieces.
 * Several key points:
 * - there are (n + 5) choose n ways of selecting n men, for example there are 56 ways of selecting 3 men, from KKK, KKQ, ... down to PPP;
 * - for i White pieces versus j Black pieces, where i > j, there are ((i + 5) choose i) * ((j + 5) choose i) files
 * - for i White pieces versus j Black pieces, there are sum of k from k = 1 to ((i + 5) choose i) files. */
extern int egtbFileSegment[EGTB_MEN][EGTB_MEN / 2 + 1];

/* given a mask with k bits set and an occupancy mask with o bits occupied, returns the rank of the
 * mask, i.e. a number between 0 and choose[64 - o][k] - 1 */
inline int rankCombination(u64 mask, u64 occupied) {
  int i = 0;
  byte square;
  int result = 0;
  while (mask) {
    GET_BIT_AND_CLEAR(mask, square);
    square -= popCount(((1ull << square) - 1) & occupied);
    result += choose[square][++i];
  }
  return result;
}

/* given the rank of a combination of 64 choose k, construct the corresponding set (mask) */
u64 unrankCombination(int rank, int k, u64 occupied);

void precomputeAll();

#endif
