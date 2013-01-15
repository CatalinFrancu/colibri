#include <stddef.h>
#include <stdio.h>
#include "bitmanip.h"

u64 rotate(u64 x, int orientation) {
  switch (orientation) {
    case ORI_NORMAL: return x;
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
