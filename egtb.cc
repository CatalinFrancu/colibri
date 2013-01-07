#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "defines.h"
#include "egtb.h"
#include "fileutil.h"
#include "movegen.h"
#include "precomp.h"

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
      counts[line][PIECE_BY_NAME[*s - 'A']]++;
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

int getEpEgtbSize(PieceSet *ps, int nps) {
  if (ps[0].piece != PAWN || ps[1].piece != PAWN) {
    return 0;
  }
  int result = 14;
  int left = 44; // Out of the 48 pawn positions, 2 are taken by the WP and BP and the 2 squares behind the en passant pawn must be clear

  // Factor in the remaining pawns
  result *= choose[left][ps[0].count - 1];
  left -= ps[0].count - 1;
  result *= choose[left][ps[1].count - 1];
  left -= ps[1].count - 1;

  // Switch from 48 to 64 and factor in the remaining pieces
  left += 16;
  for (int i = 2; i < nps; i++) {
    result *= choose[left][ps[i].count];
    left -= ps[i].count;
  }
  return result;
}

int getEpEgtbIndex(PieceSet *ps, int nps, Board *b) {
  int epSq = ctz(b->bb[BB_EP]);
  int file = epSq & 7;
  int result = file * 2; // So 0, 2, 4 or 6
  u64 occupied = b->bb[BB_EP] ^ (b->bb[BB_EP] << 8) ^ (b->bb[BB_EP] >> 8);
  u64 pawnRight = (b->side == WHITE) ? (b->bb[BB_EP] >> 7) : (b->bb[BB_EP] << 9); // Will see if the capturing pawn is on the right
  u64 stmPawns = (b->side == WHITE) ? b->bb[BB_WP] : b->bb[BB_BP];

  if (stmPawns & pawnRight) {
    occupied ^= pawnRight;
  } else {
    occupied ^= (pawnRight >> 2);
    result--;
  }
  if (b->side == BLACK) {
    result += 7;
  }

  for (int i = 0; i < nps; i++) {
    int base = (ps[i].side == WHITE) ? BB_WALL : BB_BALL;
    u64 mask = b->bb[base + ps[i].piece] & ~occupied; // We have already accounted for two pawns
    int cnt = (i < 2) ? (ps[i].count - 1) : ps[i].count;
    int freeSquares, comb;
    if (ps[i].piece == PAWN) {
      comb = rankCombination(mask >> 8, occupied >> 8);
      freeSquares = 48 - popCount(occupied);
    } else {
      comb = rankCombination(mask, occupied);
      freeSquares = 64 - popCount(occupied);
    }
    result = result * choose[freeSquares][cnt] + comb;
    occupied |= mask;
  }
  return result + getEgtbSize(ps, nps);
}

int getEgtbIndex(PieceSet *ps, int nps, Board *b) {
  if (epCapturePossible(b)) {
    return getEpEgtbIndex(ps, nps, b);
  } else {
    b->bb[BB_EP] = 0ull;
  }
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

/* Computes a score for a board and its set of forward moves by looking at all the resulting boards.
 * - If I can move into any lost position (for the opponent), then I win. Look for he shortest win.
 * - If all positions I can move into are won (for the opponent), then I lost. Look for the longest loss.
 * - If there are no moves, the position is won / lost now depending on the number of pieces.
 * - Otherwise the position is a DRAW with the current information
 *
 * fTable - if this is NULL, then we are at the first step of collecting wins/losses in 0/1 for our table.
 *   If this is not null, then we have computed some partial information that we can query.
 *   fTable != NULL also implies that none of the moves are captures (because we are now doing retrograde analysis).
 */
int forwardStep(Board *b, Move *m, int numMoves, PieceSet *ps, int nps, FILE *fTable) {
  int score;
  if (!numMoves) {
    // Compute score from White's point of view, then negate it if needed
    int delta = popCount(b->bb[BB_BALL]) - popCount(b->bb[BB_WALL]);
    score = (delta > 0) ? 1 : ((delta == 0) ? EGTB_DRAW : -1);
    if (b->side == BLACK) {
      score = -score;
    }
  } else {
    Board b2;
    int shortestWin = INFTY, longestLoss = 0;
    for (int i = 0; i < numMoves; i++) {
      memcpy(&b2, b, sizeof(Board));
      makeMove(&b2, m[i]);
      canonicalizeBoard(ps, nps, &b2);
      int childScore;
      if (fTable) {
        int index = getEgtbIndex(ps, nps, &b2);
        childScore = readChar(fTable, index);
      } else {
        childScore = evalBoard(&b2);
      }
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
  return score;
}

void evaluatePlacement(PieceSet *ps, int nps, Board *b, int side, Move *m, FILE *tmpTable, FILE *tmpBoards) {
  b->side = side;
  int numMoves = getAllMoves(b, m, FORWARD);
  int score = forwardStep(b, m, numMoves, ps, nps, NULL);

  if (score != EGTB_DRAW) {
    unsigned code = encodeEgtbBoard(ps, nps, b);
    fwrite(&code, sizeof(unsigned), 1, tmpBoards);
    // It is safe to pass b to getEgtbIndex because we generated it in canonical fashion
    int index = getEgtbIndex(ps, nps, b);
    // If there are no moves, the score is a 1 (won now) or -1 (lost now).
    // If there are moves, there is a conversion (capture / promotion) and the score is a 2 (win in 1) or -2 (loss in 1)
    int fileScore = numMoves ? 2 : 1;
    fileScore *= (score > 0) ? 1 : -1;
    writeChar(tmpTable, index, fileScore);
  }
}

/**
 * Recursively iterate over all possible placements of the piece sets.
 * Does not deal with ep positions -- those are handled separately by iterateEpEgtb().
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
  int numCombs = choose[freeSquares][gsize];
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

/* Params: see iterateEgtb(). */
void iterateEpEgtbHelper(PieceSet *ps, int nps, int level, Board *b, Move *m, u64 occupied, FILE *tmpTable, FILE *tmpBoards) {
  if (level == nps) {
    evaluatePlacement(ps, nps, b, b->side, m, tmpTable, tmpBoards);
    return;
  }

  int base = (ps[level].side == WHITE) ? BB_WALL : BB_BALL;
  bool isPawn = ps[level].piece == PAWN;
  int gsize = isPawn ? (ps[level].count - 1) : ps[level].count;
  int freeSquares = (isPawn ? 48 : 64) - popCount(occupied);
  int numCombs = choose[freeSquares][gsize];

  for (int comb = 0; comb < numCombs; comb++) {
    u64 mask = unrankCombination(comb, gsize, occupied);
    b->bb[base] ^= mask;
    b->bb[base + ps[level].piece] ^= mask;
    b->bb[BB_EMPTY] ^= mask;
    iterateEpEgtbHelper(ps, nps, level + 1, b, m, occupied ^ mask, tmpTable, tmpBoards);
    b->bb[base] ^= mask;
    b->bb[base + ps[level].piece] ^= mask;
    b->bb[BB_EMPTY] ^= mask;
  }
}

/* Params: see iterateEgtb(). */
void iterateEpEgtb(PieceSet *ps, int nps, FILE *tmpTable, FILE *tmpBoards) {
  // Generate the 14 canonical placements for the pair of pawns.
  Board b;
  Move m[200];
  for (int i = 0; i < 14; i++) {
    emptyBoard(&b);
    b.side = (i < 7) ? WHITE : BLACK;
    int index = i % 7;
    int allStm = (b.side == WHITE) ? BB_WALL : BB_BALL;
    int allSntm = BB_WALL + BB_BALL - allStm;
    int file = (index + 1) / 2;
    b.bb[BB_EP] = ((b.side == WHITE) ? 0x0000010000000000ull : 0x0000000000010000ull) << file;
    b.bb[allSntm + PAWN] = b.bb[allSntm] = (b.side == WHITE) ? (b.bb[BB_EP] >> 8) : (b.bb[BB_EP] << 8);
    b.bb[allStm + PAWN] = b.bb[allStm] = (index & 1) ? (b.bb[allSntm] >> 1) : (b.bb[allSntm] << 1);
    u64 occupied = b.bb[allStm] | b.bb[BB_EP] | (b.bb[BB_EP] << 8) | (b.bb[BB_EP] >> 8);
    iterateEpEgtbHelper(ps, nps, 0, &b, m, occupied, tmpTable, tmpBoards);
  }
}

void retrograde(PieceSet *ps, int nps, Board *b, int targetScore, FILE *tmpTable, FILE *tmpBoards, Move *mf, Move *mb) {
  // Handle all the legal orientations of the board as well
  int legalOriPawns[2] = { ORI_NORMAL, ORI_FLIP_EW };
  int legalOriNoPawns[8] = { ORI_NORMAL, ORI_ROT_CCW, ORI_ROT_180, ORI_ROT_CW, ORI_FLIP_NS, ORI_FLIP_DIAG, ORI_FLIP_EW, ORI_FLIP_ANTIDIAG };
  int *legalOri, oriCount;
  if (ps[0].piece == PAWN) {
    legalOri = legalOriPawns;
    oriCount = 2;
  } else {
    legalOri = legalOriNoPawns;
    oriCount = 8;
  }

  for (int ori = 0; ori < oriCount; ori++) {
    Board brot = *b;
    rotateBoard(&brot, legalOri[ori]);
    int nb = getAllMoves(&brot, mb, BACKWARD);
    for (int i = 0; i < nb; i++) {
      // Make backward move
      Board br = brot; // Retro-board
      makeBackwardMove(&br, mb[i]);

      // First make sure that one of the forward moves is symmetric to mb[i]. See the comments for getAllMoves() for details.
      int nf = getAllMoves(&br, mf, FORWARD);
      int sym = 0;
      while ((sym < nf) && ((mf[sym].piece != mb[i].piece) || (mf[sym].from != mb[i].to) || (mf[sym].to != mb[i].from))) {
        sym++;
      }
      if (sym < nf) {
        // Canonicalize the board and therefore recompute the forward move list
        canonicalizeBoard(ps, nps, &br);
        nf = getAllMoves(&br, mf, FORWARD);

        // Don't look at this position if we've already scored it
        int index = getEgtbIndex(ps, nps, &br);
        if (readChar(tmpTable, index) == EGTB_DRAW) {
          int score = forwardStep(&br, mf, nf, ps, nps, tmpTable);
          if ((score != EGTB_DRAW) && (abs(score) <= targetScore)) {
            unsigned code = encodeEgtbBoard(ps, nps, &br);
            fwrite(&code, sizeof(unsigned), 1, tmpBoards);
            int fileScore = (score > 0) ? (score + 1) : (score - 1);
            writeChar(tmpTable, index, fileScore);
          }
        }
      }
    }
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
  int size = getEgtbSize(ps, numPieceSets) + getEpEgtbSize(ps, numPieceSets);
  createFile(tmpTableName, size, EGTB_DRAW);
  FILE *fTable = fopen(tmpTableName, "r+");
  FILE *fBoards1 = fopen(tmpBoardName1, "w"), *fBoards2;
  iterateEgtb(ps, numPieceSets, 0, &b, m, fTable, fBoards1);
  if (ps[0].piece == PAWN && ps[1].piece == PAWN) {
    iterateEpEgtb(ps, numPieceSets, fTable, fBoards1);
  }
  fclose(fBoards1);
  int solved = getFileSize(tmpBoardName1) / sizeof(unsigned);
  printf("Discovered %d boards with wins and losses in 0 or 1 half-moves\n", solved);

  int targetScore = 1;
  Move mf[200], mb[200]; // Storage space for forward and backward moves
  while (getFileSize(tmpBoardName1)) {
    targetScore++;
    fBoards1 = fopen(tmpBoardName1, "r");
    fBoards2 = fopen(tmpBoardName2, "w");
    unsigned code;

    while (fread(&code, sizeof(unsigned), 1, fBoards1)) {
      decodeEgtbBoard(ps, numPieceSets, &b, code);
      retrograde(ps, numPieceSets, &b, targetScore, fTable, fBoards2, mf, mb);
    }

    fclose(fBoards1);
    fclose(fBoards2);
    unlink(tmpBoardName1);
    rename(tmpBoardName2, tmpBoardName1);
    int solvedStep = getFileSize(tmpBoardName1) / sizeof(unsigned);
    solved += solvedStep;
    printf("Discovered %d boards at score ±%d\n", solvedStep, targetScore);
  }

  // Done! Move the generated file in the EGTB folder and delete the temp files
  fclose(fTable);
  unlink(tmpBoardName1);
  string destName = string(EGTB_PATH) + "/" + combo + ".egt";
  printf("Moving [%s] to [%s]\n", tmpTableName, destName.c_str());
  rename(tmpTableName, destName.c_str());
  printf("Table size: %d, of which non-draws: %d\n", size, solved);
  printf("Longest win/loss:\n");
  printBoard(&b);
}

int egtbLookup(Board *b) {
  int wp = popCount(b->bb[BB_WALL]), bp = popCount(b->bb[BB_BALL]);
  if (!wp || !bp) {
    return INFTY; // You're looking in the wrong place, buddy -- evalBoard() should catch this
  }
  if (wp + bp > EGTB_MEN) {
    return INFTY;
  }

  // See if we need to change sides
  int change = false;
  if (wp < bp) {
    change = true;
  } else if (wp == bp) {
    int p = KING + 1, whiteGroup, blackGroup;
    do {
      p--;
      whiteGroup = popCount(b->bb[BB_WALL + p]);
      blackGroup = popCount(b->bb[BB_BALL + p]);
    } while ((p > PAWN) && (whiteGroup == blackGroup));
    change = (whiteGroup < blackGroup);
  }
  if (change) {
    changeSides(b);
  }

  char combo[EGTB_MEN + 2];
  int len = 0;

  // Construct the combo name
  for (int p = KING; p >= PAWN; p--) {
    for (int q = popCount(b->bb[BB_WALL + p]); q; q--) {
      combo[len++] = PIECE_INITIALS[p];
    }
  }
  combo[len++] = 'v';
  for (int p = KING; p >= PAWN; p--) {
    for (int q = popCount(b->bb[BB_BALL + p]); q; q--) {
      combo[len++] = PIECE_INITIALS[p];
    }
  }
  combo[len] = '\0';

  string fileName = string(EGTB_PATH) + "/" + combo + ".egt";
  FILE *f = fopen(fileName.c_str(), "r");
  if (!f) {
    return INFTY;
  }
  PieceSet ps[EGTB_MEN];
  int nps = comboToPieceSets(combo, ps);
  canonicalizeBoard(ps, nps, b);
  int index = getEgtbIndex(ps, nps, b);
  int score = readChar(f, index);
  fclose(f);
  return score;
}

int batchEgtbLookup(Board *b, string *moveNames, string *fens, int *scores, int *numMoves) {
  Board bcopy = *b;
  int result = egtbLookup(&bcopy);
  if (result == INFTY) {
    *numMoves = 0;
  } else {
    Move m[200];
    *numMoves = getAllMoves(b, m, FORWARD);
    getAlgebraicNotation(b, m, *numMoves, moveNames);
    for (int i = 0; i < *numMoves; i++) {
      Board b2 = *b;
      makeMove(&b2, m[i]);
      fens[i] = boardToFen(&b2);
      scores[i] = evalBoard(&b2);
    }
  }
  return result;
}
