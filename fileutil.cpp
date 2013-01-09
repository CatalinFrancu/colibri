#include <stdio.h>
#include <sys/stat.h>
#include "bitmanip.h"
#include "defines.h"
#include "fileutil.h"
#include "precomp.h"

string getFileNameForCombo(char *combo) {
  return string(EGTB_PATH) + "/" + combo + ".egt";
}

unsigned getFileSize(const char *fileName) {
  struct stat st;
  stat(fileName, &st);
  return st.st_size;
}

char readChar(FILE *f, int position) {
  fseek(f, position, SEEK_SET);
  return fgetc(f);
}

/* Takes a pointer to the king's bitboard inside a Board and returns an EGTB combination number */
inline int bitBoardsToComb(u64 *pawn, u64 *king) {
  u64 mask = 0ull;
  for (u64 *p = pawn; p <= king; p++) {
    int pc = popCount(*p);
    mask = (mask << pc) ^ ((1ull << pc) - 1);
    mask <<= 1;
  }
  mask >>= 1;
  return rankCombination(mask, 0ull);
}

int boardToFileNumber(Board *b) {
  int wp = popCount(b->bb[BB_WALL]), bp = popCount(b->bb[BB_BALL]);
  int wComb = bitBoardsToComb(b->bb + BB_WP, b->bb + BB_WK);
  int bComb = bitBoardsToComb(b->bb + BB_BP, b->bb + BB_BK);
  if (wp == bp) {
    return egtbFileSegment[wp][bp] + choose[bComb + 1][2] + wComb;
  } else {
    return egtbFileSegment[wp][bp] + wComb * choose[bp + 5][bp] + bComb;
  }
}

int comboToFileNumber(const char *combo) {
  Board b;
  emptyBoard(&b);
  int base = BB_WALL, mask = 1ull << 8;
  for (const char *s = combo; *s; s++) {
    if (*s == 'v') {
      base = BB_BALL;
    } else {
      int piece = PIECE_BY_NAME[*s - 'A'];
      b.bb[base] ^= mask;
      b.bb[base + piece] ^= mask;
      b.bb[BB_EMPTY] ^= mask;
      mask <<= 1;
    }
  }
  return boardToFileNumber(&b);
}
