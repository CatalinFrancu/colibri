#ifndef __BITMANIP_H__
#define __BITMANIP_H__
#include "defines.h"

/* Returns the number of set bits */
inline byte popCount(u64 x) {
  return __builtin_popcountll(x);
}

/* Returns the index of the last set bit. When x only has one set bit, this works as a logarithm. */
inline byte ctz(u64 x) {
  return __builtin_ctzll(x);
}

/* Reverse the 8 bytes of x */
inline u64 reverseBytes(u64 x) {
  return __builtin_bswap64(x);
}

/* Flip and/or rotate a bitboard */
u64 rotate(u64 x, int orientation);

/* Print a bitboard in the form 00110100 | 01001101 | ... */
void printBitboard(const char *msg, u64 x);

#endif
