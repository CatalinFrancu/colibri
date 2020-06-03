#ifndef __BITMANIP_H__
#define __BITMANIP_H__
#include <stdio.h>
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

/* Reverse the files of a bitboard, efectively reversing the bits inside each byte. */
inline u64 mirrorEW(u64 x) {
  x = ((x >> 1) & 0x5555555555555555ull) | ((x & 0x5555555555555555ull) << 1);
  x = ((x >> 2) & 0x3333333333333333ull) | ((x & 0x3333333333333333ull) << 2);
  x = ((x >> 4) & 0x0f0f0f0f0f0f0f0full) | ((x & 0x0f0f0f0f0f0f0f0full) << 4);
  return x;
}

/* Flip a bitboard about the a1-h8 diagonal */
inline u64 flipDiagA1H8(u64 x) {
  register u64 t;
  t  = 0x0f0f0f0f00000000ull & (x ^ (x << 28));
  x ^= t ^ (t >> 28) ;
  t  = 0x3333000033330000ull & (x ^ (x << 14));
  x ^= t ^ (t >> 14) ;
  t  = 0x5500550055005500ull & (x ^ (x <<  7));
  x ^= t ^ (t >>  7) ;
  return x;
}

/* Flip a bitboard about the a8-h1 antidiagonal */
inline u64 flipDiagA8H1(u64 x) {
  register u64 t;
  t  = x ^ (x << 36) ;
  x ^= 0xf0f0f0f00f0f0f0full & (t ^ (x >> 36));
  t  = 0xcccc0000cccc0000ull & (x ^ (x << 18));
  x ^= t ^ (t >> 18) ;
  t  = 0xaa00aa00aa00aa00ull & (x ^ (x <<  9));
  x ^= t ^ (t >>  9) ;
  return x;
}

/* Flip and/or rotate a bitboard */
u64 transform(u64 x, int tr);

/* Print a bitboard in the form 00110100 | 01001101 | ... */
void printBitboard(const char *msg, u64 x);

#endif
