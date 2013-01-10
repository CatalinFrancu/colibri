#ifndef __BITMANIP_H__
#define __BITMANIP_H__
#include "defines.h"

/* Returns the number of set bits */
inline int popCount(u64 x) {
  x -= (x >> 1) & 0x5555555555555555;
  x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
  x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
  return (x * 0x0101010101010101) >> 56;
}

/* Returns the index of the last set bit. When x only has one set bit, this works as a logarithm. */
inline int ctz(u64 x) {
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
