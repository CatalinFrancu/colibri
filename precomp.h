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

/**
 * Holds information about every placement of a k-piece set. If the placement
 * with index i is canonical, then canonical64[k][i] holds its counter,
 * otherwise it holds -1 times the transformation that should be applied to
 * this placement to obtain the canonical one.  For example, there are 10
 * canonical 1-piece placements and 278 canonical 2-piece placements.
 */
extern int* canonical64[EGTB_MEN / 2 + 1];
extern int numCanonical64[EGTB_MEN / 2 + 1];

/**
 * Stores an 8-bit mask for each placement. trMask64[x] indicates which
 * transformations achieve the canonical placement in canonical64[x].
 */
extern byte* trMask64[EGTB_MEN / 2 + 1];

/* Same, but assuming the set to be placed consists of pawns (so only 48 of the squares are legal).
 * Here we need up to EGTB-1 pieces, for combinations like KvPPPP */
extern int* canonical48[EGTB_MEN];
extern int numCanonical48[EGTB_MEN];
extern byte* trMask48[EGTB_MEN];

/* given a mask with k bits set and an occupancy mask with o bits occupied, returns the rank of the
 * mask, i.e. a number between 0 and choose[64 - o][k] - 1 */
inline int rankCombination(u64 mask, u64 occupied) {
  int i = 0;
  int square;
  int result = 0;
  u64 dMask;
  while (mask) {
    square = ctz(mask);
    mask &= (dMask = mask - 1);
    square -= popCount((mask ^ dMask) & occupied);
    result += choose[square][++i];
  }
  return result;
}

/* Same as rankCombination(), but the board is clear */
inline int rankCombinationFree(u64 mask) {
  int i = 0;
  int square;
  int result = 0;
  while (mask) {
    GET_BIT_AND_CLEAR(mask, square);
    result += choose[square][++i];
  }
  return result;
}

/* given the rank of a combination of 64 choose k, construct the corresponding set (mask) */
u64 unrankCombination(int rank, int k, u64 occupied);

void precomputeAll();

#endif
