/**
 * This is a rather hacky script whose purpose was to examine several indexing systems for the EGTB.
 * We needed to balance speed, hard-disk space and ease of implementation.
 *
 * One thing was clear from the start:
 * 0. Pawns should be indexed before non-pawns
 *
 * We evaluated three variables:
 * 1. When both white and black pawns are present, do we index by WP first, or by the largest group of pawns first?
 * 2. In endings like KQvNN, do we index by the largest group of pieces first (NN), or can we index pieces in any order?
 * 3. For groups after the first one, can we use base 64, or should we shrink to base 63, 62... as the board fills up?
 *
 * It turns out for (1) and (2) it doesn't matter much. Indexing by the largest group of pawns and the largest group of
 * pieces can save about 1%. For (3), we need to use a variable base. Using base 64 all the time wastes about 17% disk
 * space.
 *
 * For (2), we decided to index the pieces in any order, but start with a group of size 2 or less. Such a group will always
 * exist for 5-men EGTB (it is not guaranteed for 6-men, where KKKvQQQ only consists of 3-piece groups). This will save us
 * the trouble of storing symmetry mappings for 3- and 4-piece groups (64 choose 4 is 640,000).
 * For the general case, we will store symmetry mappings up to floor(EGTB_MEN / 2) pieces.
 **/

#include <stdio.h>
#include <stdlib.h>
#include "../bitmanip.h"
#include "../defines.h"
#include "../precomp.h"

#define PIECE_TYPES 6
#define NUM_SQUARES 64

#define P_PAWN 0
#define P_KNIGHT 1
#define P_BISHOP 2
#define P_ROOK 3
#define P_QUEEN 4
#define P_KING 5

typedef unsigned long long u64;

char names[PIECE_TYPES + 1] = "PNBRQK";
int pawnPlacements[EGTB_MEN];
int nonPawnPlacements[EGTB_MEN];

// We examine eight strategies coded 0 to 7:
// - bit 0 = 0: index by largest group of pawns first (save space)
// - bit 0 = 1: index by white pawns first (save time)
// - bit 1 = 0: index by largest group first, then remaining groups in piece order (save space)
// - bit 1 = 1: index groups in piece order (save time)
// - bit 2 = 0: store choices as 63,62,61,60-choose-k (save space)
// - bit 2 = 1: store choices as 64-choose-l (save time)
u64 estimatedSizes[8];

int genPawnPlacements(int n, int last, u64 x) {
  if (!n) {
    return (x < rotate(x, ORI_FLIP_EW)) ? 0 : 1;
  }
  int result = 0;
  for (int i = last + 1; i <= 56 - n; i++) {
    result += genPawnPlacements(n - 1, i, x ^ (1ull << i));
  }
  return result;
}

void countPawnPlacements() {
  pawnPlacements[0] = 1;
  for (int i = 1; i < EGTB_MEN; i++) {
    pawnPlacements[i] = genPawnPlacements(i, 7, 0);
    printf("%d pawns: %d placements\n", i, pawnPlacements[i]);
  }
}

int genNonPawnPlacements(int n, int first, u64 x) {
  if (!n) {
    for (int i = 1; i <= 7; i++) {
      if (rotate(x, i) < x) {
        return 0;
      }
    }
    return 1;
  }
  int result = 0;
  for (int i = first; i <= 64 - n; i++) {
    result += genNonPawnPlacements(n - 1, i + 1, x ^ (1ull << i));
  }
  return result;
}

void countNonPawnPlacements() {
  nonPawnPlacements[0] = 1;
  for (int i = 1; i < EGTB_MEN; i++) {
    nonPawnPlacements[i] = genNonPawnPlacements(i, 0, 0);
    printf("%d non-pawns: %d placements\n", i, nonPawnPlacements[i]);
  }
}

u64 estimateSizeStrat(int *wc, int *bc, int sortPawns, int largestGroup, int shrinkBase) {
  u64 result;
  if (wc[P_PAWN] || bc[P_PAWN]) {
    int wp = wc[P_PAWN], bp = bc[P_PAWN];
    if (sortPawns) {
      if (wp < bp) {
        int tmp = wp;
        wp = bp;
        bp = tmp;
      }
      result = pawnPlacements[wp] * choose[shrinkBase ? (48 - wp) : 48][bp];
    } else {
      if (wp && bp) {
        result = pawnPlacements[wp] * choose[shrinkBase ? (48 - wp) : 48][bp];
      } else if (wp) {
        result = pawnPlacements[wp];
      } else {
        result = pawnPlacements[bp];
      }
    }
    u64 left = 64 - wp - bp;
    for (int i = P_KNIGHT; i <= P_KING; i++) {
      result *= choose[shrinkBase ? left : 64][wc[i]];
      left -= wc[i];
    }
    for (int i = P_KNIGHT; i <= P_KING; i++) {
      result *= choose[shrinkBase ? left : 64][bc[i]];
      left -= bc[i];
    }
  } else {
    // Make one single group of pieces
    int c[2 * PIECE_TYPES], nc = 0;
    for (int i = P_PAWN; i <= P_KING; i++) {
      if (wc[i]) {
        c[nc++] = wc[i];
      }
    }
    for (int i = P_PAWN; i <= P_KING; i++) {
      if (bc[i]) {
        c[nc++] = bc[i];
      }
    }
    int largestIndex = 0;
    if (largestGroup) {
      for (int i = 1; i < nc; i++) {
        if (c[i] > c[largestIndex]) {
          largestIndex = i;
        }
      }
    } else {
      while (c[largestIndex] > 2) {
        largestIndex++;
      }
    }
    result = nonPawnPlacements[c[largestIndex]];
    int left = 64 - c[largestIndex];
    for (int i = 0; i < nc; i++) {
      if (i != largestIndex) {
        result *= choose[shrinkBase ? left : 64][c[i]];
        left -= c[i];
      }
    }
  }
  // printf("wp: %d bp: %d sortPawns: %d largestGroup: %d shrinkBase: %d result: %lld\n",
  //        wc[P_PAWN], bc[P_PAWN], sortPawns, largestGroup, shrinkBase, result);
  return result;
}

void estimateSize(int *whiteCount, int *blackCount) {
  estimatedSizes[0] += estimateSizeStrat(whiteCount, blackCount, true, true, true);
  estimatedSizes[1] += estimateSizeStrat(whiteCount, blackCount, false, true, true);
  estimatedSizes[2] += estimateSizeStrat(whiteCount, blackCount, true, false, true);
  estimatedSizes[3] += estimateSizeStrat(whiteCount, blackCount, false, false, true);
  estimatedSizes[4] += estimateSizeStrat(whiteCount, blackCount, true, true, false);
  estimatedSizes[5] += estimateSizeStrat(whiteCount, blackCount, false, true, false);
  estimatedSizes[6] += estimateSizeStrat(whiteCount, blackCount, true, false, false);
  estimatedSizes[7] += estimateSizeStrat(whiteCount, blackCount, false, false, false);
}

void sol(int *whiteCount, int *blackCount) {
  int cw = 0, cb = 0, cmp = 0;
  for (int i = 0; i < PIECE_TYPES; i++) {
    cw += whiteCount[i];
    cb += blackCount[i];
    // Compare the white and black arrays for the case where cw = cb
    if (!cmp && (whiteCount[i] != blackCount[i])) {
      cmp = (whiteCount[i] < blackCount[i]) ? -1 : 1;
    }
  }
  if (cw == cb && cmp == -1) {
    return;
  }
  estimateSize(whiteCount, blackCount);
  for (int i = 5; i >= 0; i--) {
    for (int j = 0; j < whiteCount[i]; j++) {
      printf("%c", names[i]);
    }
  }
  printf("v");
  for (int i = 5; i >= 0; i--) {
    for (int j = 0; j < blackCount[i]; j++) {
      printf("%c", names[i]);
    }
  }
  printf("\n");
}

void genBlackTables(int *whiteCount, int *blackCount, int blackMen, int level) {
  if (blackMen == 0) {
    sol(whiteCount, blackCount);
  } else if (level == PIECE_TYPES - 1) {
    blackCount[level] = blackMen;
    sol(whiteCount, blackCount);
    blackCount[level] = 0;
  } else {
    for (int i = 0; i <= blackMen; i++) {
      blackCount[level] = i;
      genBlackTables(whiteCount, blackCount, blackMen - i, level + 1);
      blackCount[level] = 0;
    }
  }
}

u64 genWhiteTables(int *whiteCount, int whiteMen, int *blackCount, int blackMen, int level) {
  if (whiteMen == 0) {
    genBlackTables(whiteCount, blackCount, blackMen, 0);
  } else if (level == PIECE_TYPES - 1) {
    whiteCount[level] = whiteMen;
    genBlackTables(whiteCount, blackCount, blackMen, 0);
    whiteCount[level] = 0;
  } else {
    for (int i = 0; i <= whiteMen; i++) {
      whiteCount[level] = i;
      genWhiteTables(whiteCount, whiteMen - i, blackCount, blackMen, level + 1);
      whiteCount[level] = 0;
    }
  }
}

int main() {
  precomputeAll();
  int whiteCount[PIECE_TYPES], blackCount[PIECE_TYPES];
  for (int i = 0; i < PIECE_TYPES; i++) {
    whiteCount[i] = blackCount[i] = 0;
  }
  countPawnPlacements();
  countNonPawnPlacements();
  //  exit(1);

  for (int men = 2; men <= EGTB_MEN; men++) {
    printf("** %d-men tables\n", men);
    for (int whiteMen = (men + 1) / 2; whiteMen < men; whiteMen++) {
      int blackMen = men - whiteMen;
      printf("**** %dv%d tables\n", whiteMen, blackMen);
      genWhiteTables(whiteCount, whiteMen, blackCount, blackMen, 0);
    }
  }

  for (int i = 0; i < 8; i++) {
    printf("Strategy %d: %llu bytes\n", i, estimatedSizes[i]);
  }
}

