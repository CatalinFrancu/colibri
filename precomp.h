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
extern int* canonical64[EGTB_MEN / 2];
extern int numCanonical64[EGTB_MEN / 2];

/* Same, but assuming the set to be placed consists of pawns (so only 48 of the squares are legal) */
extern int* canonical48[EGTB_MEN / 2];
extern int numCanonical48[EGTB_MEN / 2];

/* given a mask with k bits set and an occupancy mask with o bits occupied, returns the rank of the
 * mask, i.e. a number between 0 and choose[64 - o][k] - 1 */
int rankCombination(u64 mask, u64 occupied);

/* given the rank of a combination of 64 choose k, construct the corresponding set (mask) */
u64 unrankCombination(int rank, int k);

void precomputeAll();

#endif
