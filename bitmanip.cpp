#include <stdio.h>
#include <string>
#include "bitmanip.h"
#include "logging.h"

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
  string s = string(msg) + ": h8 ";
  for (int i = 63; i >= 0; i--) {
    s += ((x & (1ull << i)) ? '1' : '0');
    if (i && (i % 8 == 0)) {
      s += " | ";
    }
  }
  s += " a1";
  log(LOG_DEBUG, s.c_str());
}
