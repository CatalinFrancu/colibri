#include <stdio.h>
#include <sys/stat.h>
#include "bitmanip.h"
#include "defines.h"
#include "fileutil.h"
#include "precomp.h"

string getFileNameForCombo(const char *combo) {
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
