#include <stdio.h>
#include <stdlib.h>
#include "precomp.h"

int rotateSquare[64][8];
int choose[65][EGTB_MEN];
int* canonical64[EGTB_MEN / 2 + 1];
int numCanonical64[EGTB_MEN / 2 + 1];
int* canonical48[EGTB_MEN];
int numCanonical48[EGTB_MEN];
u64 kingAttacks[64];
u64 knightAttacks[64];
byte isoMove[8][64];
u64 fileConfiguration[256];
u64 diagonal[64];
u64 antidiagonal[64];

void precomputeRotateSquare() {
  for (int sq = 0; sq < 64; sq++) {
    int rank = sq >> 3, file = sq & 7;
    rotateSquare[sq][ORI_NORMAL] = sq;
    rotateSquare[sq][ORI_ROT_CCW] = SQUARE(file, 7 - rank);
    rotateSquare[sq][ORI_ROT_180] = SQUARE(7 - rank, 7 - file);
    rotateSquare[sq][ORI_ROT_CW] = SQUARE(7 - file, rank);
    rotateSquare[sq][ORI_FLIP_NS] = SQUARE(7 - rank, file);
    rotateSquare[sq][ORI_FLIP_DIAG] = SQUARE(file, rank);
    rotateSquare[sq][ORI_FLIP_EW] = SQUARE(rank, 7 - file);
    rotateSquare[sq][ORI_FLIP_ANTIDIAG] = SQUARE(7 - file, 7 - rank);
  }
}

void precomputeNChooseK() {
  for (int n = 0; n <= 64; n++) {
    for (int k = 0; k < EGTB_MEN; k++) {
      if (k == 0) {
        choose[n][k] = 1;
      } else if (n == 0) {
        choose[n][k] = 0;
      } else {
        choose[n][k] = choose[n - 1][k] + choose[n - 1][k - 1];
      }
    }
  }
}

/* This implementation is O(64). */
u64 unrankCombination(int rank, int k, u64 occupied) {
  u64 result = 0ull;
  int free = 64 - popCount(occupied); /* Number of free bits between positions 0 and bit inclusively */
  int bit = 63;
  while (k) {
    if ((1ull << bit) & ~occupied) {
      if (rank >= choose[free - 1][k]) {
        result |= 1ull << bit;
        rank -= choose[free - 1][k];
        k--;
      }
      free--;
    }
    bit--;
  }
  return result;
}

void precomputeCanonical64() {
  for (int k = 1; k <= EGTB_MEN / 2; k++) {
    int uniqueCounter = 0;
    canonical64[k] = new int[choose[64][k]];
    for (int comboNumber = 0; comboNumber < choose[64][k]; comboNumber++) {
      u64 combo = unrankCombination(comboNumber, k, 0);
      int minNumber = comboNumber;
      int minTransform = -1;
      for (int tr = 1; tr <= 7; tr++) {
        int newNumber = rankCombination(rotate(combo, tr), 0);
        if (newNumber < minNumber) {
          minNumber = newNumber;
          minTransform = tr;
        }
      }
      if (minNumber == comboNumber) {
        canonical64[k][comboNumber] = uniqueCounter++;
      } else {
        canonical64[k][comboNumber] = -minTransform;
      }
    }
    numCanonical64[k] = uniqueCounter;
  }
}

void precomputeCanonical48() {
  for (int k = 1; k < EGTB_MEN; k++) {
    int uniqueCounter = 0;
    canonical48[k] = new int[choose[48][k]];
    for (int comboNumber = 0; comboNumber < choose[48][k]; comboNumber++) {
      u64 combo = unrankCombination(comboNumber, k, 0);
      int mirrorNumber = rankCombination(rotate(combo, ORI_FLIP_EW), 0);
      if (mirrorNumber < comboNumber) {
        canonical48[k][comboNumber] = -ORI_FLIP_EW;
      } else {
        canonical48[k][comboNumber] = uniqueCounter++;
      }
    }
    numCanonical48[k] = uniqueCounter;
  }
}

void precomputeKingAttacks() {
  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      u64 result = 0ull;

      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          if ((i || j) && (rank + i >= 0) && (rank + i < 8) && (file + j >= 0) && (file + j < 8)) {
            result |= BIT(rank + i, file + j);
          }
        }
      }

      kingAttacks[SQUARE(rank, file)] = result;
    }
  }
}

void precomputeKnightAttacks() {
  int delta[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      u64 result = 0ull;

      for (int k = 0; k < 8; k++) {
        if ((rank + delta[k][0] >= 0) && (rank + delta[k][0] < 8) && (file + delta[k][1] >= 0) && (file + delta[k][1] < 8)) {
          result |= BIT(rank + delta[k][0], file + delta[k][1]);
        }
      }

      knightAttacks[SQUARE(rank, file)] = result;
    }
  }
}

void precomputeIsoMoves() {
  for (int pos = 0; pos < 8; pos++) {
    for (int occ = 0; occ < 64; occ++) {
      byte result = 0;

      int currentPos = pos + 1;
      bool canGo = true;
      while (currentPos < 8 && canGo) {
        result |= 1 << currentPos;
        canGo = !((1 << currentPos) & (occ << 1));
        currentPos++;
      }

      currentPos = pos - 1;
      canGo = true;
      while (currentPos >= 0 && canGo) {
        result |= 1 << currentPos;
        canGo = !((1 << currentPos) & (occ << 1));
        currentPos--;
      }

      isoMove[pos][occ] = result;
    }
  }
}

void precomputeFileConfigurations() {
  for (int i = 0; i < 256; i++) {
    u64 result = 0;

    byte currentByte = i;
    for (int j = 0; j < 8; j++) {
      result = (result << 8) + (currentByte & 1);
      currentByte >>= 1;
    }

    fileConfiguration[i] = result;
  }
}

void precomputeDiagonals() {
  for (int sq = 0; sq < 64; sq++) {
    u64 result = 0;

    u64 currentSq = 1ull << sq;
    int rank = sq >> 3;
    int file = sq & 7;

    while (rank < 8 && file < 8) {
      result |= currentSq;
      currentSq <<= 9;
      rank++;
      file++;
    }

    currentSq = 1ull << sq;
    rank = sq >> 3;
    file = sq & 7;

    while (rank >= 0 && file >= 0) {
      result |= currentSq;
      currentSq >>= 9;
      rank--;
      file--;
    }

    diagonal[sq] = result;
  }
}

void precomputeAntidiagonals() {
  for (int sq = 0; sq < 64; sq++) {
    u64 result = 0;

    u64 currentSq = 1ull << sq;
    int rank = sq >> 3;
    int file = sq & 7;

    while (rank < 8 && file >= 0) {
      result |= currentSq;
      currentSq <<= 7;
      rank++;
      file--;
    }

    currentSq = 1ull << sq;
    rank = sq >> 3;
    file = sq & 7;

    while (rank >= 0 && file < 8) {
      result |= currentSq;
      currentSq >>= 7;
      rank--;
      file++;
    }

    antidiagonal[sq] = result;
  }
}

void precomputeAll() {
  precomputeRotateSquare();
  precomputeNChooseK();
  precomputeCanonical64();
  precomputeCanonical48();
  precomputeKingAttacks();
  precomputeKnightAttacks();
  precomputeIsoMoves();
  precomputeFileConfigurations();
  precomputeDiagonals();
  precomputeAntidiagonals();
}
