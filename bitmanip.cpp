#include <stddef.h>
#include <stdio.h>
#include "bitmanip.h"

byte popCount(u64 x) {
  return __builtin_popcountll(x);
}

byte ctz(u64 x) {
  return __builtin_ctzll(x);
}

u64 reverseBytes(u64 x) {
  return __builtin_bswap64(x);
}

u64 mirrorEW(u64 x) {
   x = ((x >> 1) & 0x5555555555555555ull) | ((x & 0x5555555555555555ull) << 1);
   x = ((x >> 2) & 0x3333333333333333ull) | ((x & 0x3333333333333333ull) << 2);
   x = ((x >> 4) & 0x0f0f0f0f0f0f0f0full) | ((x & 0x0f0f0f0f0f0f0f0full) << 4);
   return x;
}

u64 flipDiagA1H8(u64 x) {
   u64 t;
   t  = 0x0f0f0f0f00000000ull & (x ^ (x << 28));
   x ^= t ^ (t >> 28) ;
   t  = 0x3333000033330000ull & (x ^ (x << 14));
   x ^= t ^ (t >> 14) ;
   t  = 0x5500550055005500ull & (x ^ (x <<  7));
   x ^= t ^ (t >>  7) ;
   return x;
}

u64 flipDiagA8H1(u64 x) {
   u64 t;
   t  = x ^ (x << 36) ;
   x ^= 0xf0f0f0f00f0f0f0full & (t ^ (x >> 36));
   t  = 0xcccc0000cccc0000ull & (x ^ (x << 18));
   x ^= t ^ (t >> 18) ;
   t  = 0xaa00aa00aa00aa00ull & (x ^ (x <<  9));
   x ^= t ^ (t >>  9) ;
   return x;
}


u64 rotate(u64 x, int orientation) {
  switch (orientation) {
    case ORI_ROT_CCW: return flipDiagA1H8(reverseBytes(x));
    case ORI_ROT_180: return reverseBytes(mirrorEW(x));
    case ORI_ROT_CW: return reverseBytes(flipDiagA1H8(x));
    case ORI_FLIP_NS: return reverseBytes(x);
    case ORI_FLIP_DIAG: return flipDiagA1H8(x);
    case ORI_FLIP_EW: return mirrorEW(x);
    case ORI_FLIP_ANTIDIAG: return flipDiagA8H1(x);
    default: return x;
  }
}

void printBitboard(const char *msg, u64 x) {
  printf("%s: h8 ", msg);
  for (int i = 63; i >= 0; i--) {
    printf("%d", (x & (1ull << i)) ? 1 : 0);
    if (i && (i % 8 == 0)) {
      printf(" | ");
    }
  }
  printf(" a1\n");
}
