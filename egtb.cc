#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "defines.h"
#include "egtb.h"
#include "fileutil.h"
#include "movegen.h"
#include "precomp.h"

int getPieceByName(char c) {
  switch(c) {
    case 'P': return PAWN;
    case 'N': return KNIGHT;
    case 'B': return BISHOP;
    case 'R': return ROOK;
    case 'Q': return QUEEN;
    case 'K': return KING;
    default: return -1;
  }
}

void comboToPieceCounts(const char *combo, int counts[2][KING + 1]) {
  for (int i = 0; i <= 1; i++) {
    for (int j = PAWN; j <= KING; j++) {
      counts[i][j] = 0;
    }
  }
  int line = 0;
  for (char *s = (char*)combo; *s; s++) {
    if (*s == 'v') {
      line++;
    } else {
      counts[line][getPieceByName(*s)]++;
    }
  }
}

void pushPieceSet(PieceSet *ps, int *numPieceSets, bool side, int piece, int count) {
  if (count) {
    ps[*numPieceSets].side = side;
    ps[*numPieceSets].piece = piece;
    ps[*numPieceSets].count = count;
    (*numPieceSets)++;
  }
}

int comboToPieceSets(const char *combo, PieceSet *ps) {
  int n = 0;
  int c[2][KING + 1];
  comboToPieceCounts(combo, c);

  // Push the pawns first
  pushPieceSet(ps, &n, WHITE, PAWN, c[0][PAWN]);
  pushPieceSet(ps, &n, BLACK, PAWN, c[1][PAWN]);

  // If no pawns, push the first group having no more than EGTB_MEN / 2 pieces (1 or 2 for EGTB_MEN = 5)
  if (!n) {
    int line = 0, piece = KNIGHT;
    while (c[line][piece] == 0 || c[line][piece] > EGTB_MEN / 2) {
      if (piece == KING) {
        line++;
        piece = KNIGHT;
      } else {
        piece++;
      }
    }
    pushPieceSet(ps, &n, line ? BLACK : WHITE, piece, c[line][piece]);
    c[line][piece] = 0;
  }

  // Push the remaining non-pawn pieces
  for (int i = 0; i <= 1; i++) {
    for (int j = KNIGHT; j <= KING; j++) {
      pushPieceSet(ps, &n, i ? BLACK : WHITE, j, c[i][j]);
    }
  }
  return n;
}

int getEgtbSize(PieceSet *ps, int numPieceSets) {
  int result = (ps[0].piece == PAWN) ? numCanonical48[ps[0].count] : numCanonical64[ps[0].count];
  int used = ps[0].count;
  int cur = 1;

  if (ps[1].piece == PAWN) {
    result *= choose[48 - used][ps[1].count];
    used += ps[1].count;
    cur++;
  }

  while (cur < numPieceSets) {
    result *= choose[64 - used][ps[cur].count];
    used += ps[cur].count;
    cur++;
  }

  result *= 2; // For White-to-move and Black-to-move
  return result;
}

int getEgtbIndex(PieceSet *ps, int nps, Board *b) {
//  // First rotate the board if needed
//  int base = (ps[0].side == WHITE) ? BB_WALL: BB_BALL;
//  u64 firstBb = b->bb[base + ps[0].piece];
//  int comb = rankCombination(firstBb, 0ull);
//  int canon = (ps[0].piece == PAWN) ? canonical48[ps[0].count][comb] : canonical64[ps[0].count][comb];
//  if (canon < 0) { // Then it indicates a negated orientation
//    rotateBoard(b, -canon);
//  }    *********************** MOVE THIS TO EGTB_LOOKUP ***************************

  u64 occupied = 0ull;
  int base, result = 0, comb;

  for (int i = 0; i < nps; i++) {
    base = (ps[i].side == WHITE) ? BB_WALL : BB_BALL;
    u64 mask = b->bb[base + ps[i].piece];
    int freeSquares;
    if (ps[i].piece == PAWN) {
      comb = rankCombination(mask >> 8, occupied >> 8);
      freeSquares = 48 - popCount(occupied);
    } else {
      comb = rankCombination(mask, occupied);
      freeSquares = 64 - popCount(occupied);
    }
    if (i == 0) {
      result = (ps[0].piece == PAWN) ? canonical48[ps[0].count][comb] : canonical64[ps[0].count][comb];
    } else {
      result = result * choose[freeSquares][ps[i].count] + comb;
    }
    occupied |= mask;
  }
  result = result * 2 + ((b->side == WHITE) ? 0 : 1);
  return result;
}

unsigned encodeEgtbBoard(PieceSet *ps, int nps, Board *b) {
  unsigned result = 0;
  int doublePushSq = -1, replacementSq = -1;

  // If an en passant bit is set, we can determine (1) where the corresponding pawn is and (2) which square we should encode instead
  if (b->bb[BB_EP]) {
    int epSq = ctz(b->bb[BB_EP]);
    doublePushSq = (b->side == WHITE) ? epSq - 8 : epSq + 8;
    replacementSq = (b->side == WHITE) ? epSq + 16 : epSq - 16;
  }

  for (int i = 0; i < nps; i++) {
    int base = (ps[i].side == WHITE) ? BB_WALL : BB_BALL;
    u64 mask = b->bb[base + ps[i].piece];
    while (mask) {
      int sq;
      GET_BIT_AND_CLEAR(mask, sq);
      if (sq == doublePushSq) {
        sq = replacementSq;
      }
      result = (result << 6) + sq;
    }
  }
  result = (result << 1) + b->side;
  return result;
}

void decodeEgtbBoard(PieceSet *ps, int nps, Board *b, unsigned code) {
  emptyBoard(b);
  b->side = code & 1;
  code >>= 1;
  for (int i = nps - 1; i >= 0; i--) {
    u64 mask = 0;
    for (int j = 0; j < ps[i].count; j++) {
      int sq = code & 63;
      code >>= 6;
      if ((ps[i].piece == PAWN) && (sq < 8 || sq >= 56)) {
        b->bb[BB_EP] = 1ull << ((b->side == WHITE) ? sq - 16 : sq + 16);
        sq = (b->side == WHITE) ? sq - 24 : sq + 24;
      }
      mask ^= 1ull << sq;
    }
    int base = (ps[i].side == WHITE) ? BB_WALL : BB_BALL;
    b->bb[base] ^= mask;
    b->bb[base + ps[i].piece] = mask;
    b->bb[BB_EMPTY] ^= mask;
  }
}

void evaluatePlacement(PieceSet *ps, int nps, Board *b, int side, Move *m, FILE *tmpTable, FILE *tmpBoards) {
  b->side = side;
  Board b2;
  int numMoves = getAllMoves(b, m, true);
  int score;

  if (!numMoves) {
    // Compute score from White's point of view, then negate it if needed
    int delta = popCount(b->bb[BB_BALL]) - popCount(b->bb[BB_WALL]);
    score = (delta > 0) ? 1 : ((delta == 0) ? EGTB_DRAW : -1);
    if (b->side == BLACK) {
      score = -score;
    }
  } else {
    // If I can move into any lost position (for the opponent), then I win. Look for he shortest win.
    // If all positions I can move into are won (for the opponent), then I lost. Look for the longest loss.
    // If there are no moves, the position is won / lost now.
    int shortestWin = INFTY, longestLoss = 0;
    for (int i = 0; i < numMoves; i++) {
      memcpy(&b2, b, sizeof(Board));
      makeMove(&b2, m[i]);
      int childScore = evalBoard(&b2);
      if (childScore == EGTB_DRAW) {
        longestLoss = INFTY; // Can hold out forever
      } else if (childScore < 0 && -childScore < shortestWin) {
        shortestWin = -childScore;
      } else if (childScore > 0 && childScore > longestLoss) {
        longestLoss = childScore;
      }
    }
    // At this point shortestWin / longestLoss already account for my move, because EGTB values are shifted by 1
    score = (shortestWin < INFTY) ? shortestWin : ((longestLoss == INFTY) ? EGTB_DRAW : -longestLoss);
  }

  if (score != EGTB_DRAW) {
    unsigned code = encodeEgtbBoard(ps, nps, b);
    fwrite(&code, sizeof(unsigned), 1, tmpBoards);
    // It is safe to pass b to getEgtbIndex because we generated it in canonical fashion
    int index = getEgtbIndex(ps, nps, b);
    writeChar(tmpTable, index, score);
  }
}

/**
 * Recursively iterate over all possible placements of the piece sets.
 * ps - array of piece sets
 * nps - number of piece sets
 * level - index of current piece set being placed
 * board - stores the pieces we have placed so far
 * m - reusable space for move generation during evaluatePlacement()
 * tmpTable - holds the EGTB scores
 * tmpBoards - collects the "lost/won in 0 or 1" tables we find during the initial scan
 */
void iterateEgtb(PieceSet *ps, int nps, int level, Board *b, Move *m, FILE *tmpTable, FILE *tmpBoards) {
  if (level == nps) {
    // Found a placement, now evaluate it.
    evaluatePlacement(ps, nps, b, WHITE, m, tmpTable, tmpBoards);
    evaluatePlacement(ps, nps, b, BLACK, m, tmpTable, tmpBoards);
    return;
  }

  int baseBb = (ps[level].side == WHITE) ? BB_WALL : BB_BALL;
  int gsize = ps[level].count;
  bool isPawn = ps[level].piece == PAWN;
  int freeSquares = (isPawn ? 48 : 64) - getPieceCount(b);
  int numCombs = choose[freeSquares][ps[level].count];
  u64 occupied = b->bb[BB_WALL] ^ b->bb[BB_BALL];
  if (isPawn) {
    occupied >>= 8;
  }

  for (int comb = 0; comb < numCombs; comb++) {
    // For the first piece set, the combination must be canonical. Otherwise, all combinations are acceptable.
    bool acceptable = (level == 0) ? (isPawn ? (canonical48[gsize][comb] >= 0) : (canonical64[gsize][comb] >= 0)) : true;
    if (acceptable) {
      u64 mask = unrankCombination(comb, gsize, occupied);
      if (isPawn) {
        mask <<= 8;
      }
      b->bb[baseBb] ^= mask;
      b->bb[baseBb + ps[level].piece] = mask;
      b->bb[BB_EMPTY] ^= mask;
      iterateEgtb(ps, nps, level + 1, b, m, tmpTable, tmpBoards);
      b->bb[baseBb] ^= mask;
      b->bb[baseBb + ps[level].piece] = 0ull;
      b->bb[BB_EMPTY] ^= mask;
    }
  }
}

void retrograde(PieceSet *ps, int nps, Board *b, FILE *tmpTable, FILE *tmpBoards) {
  Move m[200];
  int numMoves = getAllMoves(b, m, false);
  for (int i = 0; i < numMoves; i++) {
    // Make backward move etc.
  }
}

void generateEgtb(const char *combo) {
  printf("Generating table %s\n", combo);
  const char *tmpTableName = "/tmp/egtb-gen-table";
  const char *tmpBoardName1 = "/tmp/egtb-gen-boards1";
  const char *tmpBoardName2 = "/tmp/egtb-gen-boards2";
  PieceSet ps[EGTB_MEN];
  int numPieceSets = comboToPieceSets(combo, ps);
  Board b;
  Move m[200];

  // First populate all the immediate wins / losses (no legal moves) and immediate conversions (win/loss in 1).
  // Collect all the positions thus evaluated, in encoded form, in a temporary file.
  emptyBoard(&b);
  int size = getEgtbSize(ps, numPieceSets);
  createFile(tmpTableName, size, EGTB_DRAW);
  FILE *fTable = fopen(tmpTableName, "r+");
  FILE *fBoards1 = fopen(tmpBoardName1, "w"), *fBoards2;
  iterateEgtb(ps, numPieceSets, 0, &b, m, fTable, fBoards1);
  fclose(fBoards1);
  printf("Discovered %lu boards with wins and losses in 0 or 1 half-moves\n", getFileSize(tmpBoardName1) / sizeof(unsigned));

  int targetScore = 0;
  while (getFileSize(tmpBoardName1)) {
    targetScore++;
    fBoards1 = fopen(tmpBoardName1, "r");
    fBoards2 = fopen(tmpBoardName2, "w");
    unsigned code;

    while (fread(&code, sizeof(unsigned), 1, fBoards1)) {
      decodeEgtbBoard(ps, numPieceSets, &b, code);
      retrograde(ps, numPieceSets, &b, fTable, fBoards2);
    }

    fclose(fBoards1);
    fclose(fBoards2);
    unlink(tmpBoardName1);
    rename(tmpBoardName2, tmpBoardName1);
    printf("Discovered %lu boards at score ±%d\n", getFileSize(tmpBoardName1) / sizeof(unsigned), targetScore);
  }

  // Done! Move the generated file in the EGTB folder and delete the temp files
  fclose(fTable);
  unlink(tmpBoardName1);
}
