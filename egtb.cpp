#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "configfile.h"
#include "defines.h"
#include "egtb.h"
#include "fileutil.h"
#include "logging.h"
#include "lrucache.h"
#include "movegen.h"
#include "precomp.h"
#include "timer.h"

LruCache egtbCache;

void initEgtb() {
  egtbCache = lruCacheCreate(cfgEgtbChunks);
}

inline u64 egtbGetKey(const char *combo, int index) {
  u64 result = 0ull;
  for (const char *s = combo; *s; s++) {
    result <<= 3;
    if (*s != 'v') {
      result ^= PIECE_BY_NAME[*s - 'A'];
    }
  }
  return (result << 30) + index;
}

char* readEgtbChunkFromFile(const char *combo, int chunkNo) {
  // Look up the compressed file and index
  string compressedFile = getCompressedFileNameForCombo(combo).c_str();
  string idxFile = getIndexFileNameForCombo(combo).c_str();
  char *data = decompressBlock(compressedFile.c_str(), idxFile.c_str(), chunkNo);
  if (data) {
    return data;
  }

  // Look up the uncompressed file
  string fileName = getFileNameForCombo(combo).c_str();
  FILE *f = fopen(fileName.c_str(), "r");
  if (f) {
    int startPos = chunkNo * EGTB_CHUNK_SIZE;
    data = (char*)malloc(EGTB_CHUNK_SIZE);
    fseek(f, startPos, SEEK_SET);
    if (!fread(data, 1, EGTB_CHUNK_SIZE, f)) {
      log(LOG_WARNING, "No bytes read from EGTB combo %s chunk %d", combo, chunkNo);
      free(data);
      return NULL;
    }
    fclose(f);
    return data;
  }

  return NULL;
}

int readFromCache(const char *combo, int index) {
  int chunkNo = index / EGTB_CHUNK_SIZE, chunkOffset = index % EGTB_CHUNK_SIZE;
  u64 key = egtbGetKey(combo, chunkNo);
  char *data = (char*)lruCacheGet(&egtbCache, key);
  if (!data) {
    data = readEgtbChunkFromFile(combo, chunkNo);
    lruCachePut(&egtbCache, key, data);
  }
  return data ? data[chunkOffset] : INFTY;
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

int getComboSize(const char *combo) {
  PieceSet ps[EGTB_MEN];
  int nps = comboToPieceSets(combo, ps);
  return getEgtbSize(ps, nps) + getEpEgtbSize(ps, nps);
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
  u64 occupied = 0ull, occupiedSq = 0;
  int base, result = 0, comb;

  for (int i = 0; i < nps; i++) {
    base = (ps[i].side == WHITE) ? BB_WALL : BB_BALL;
    u64 mask = b->bb[base + ps[i].piece];
    int freeSquares;
    if (ps[i].piece == PAWN) {
      comb = rankCombination(mask >> 8, occupied >> 8);
      freeSquares = 48 - occupiedSq;
    } else {
      comb = rankCombination(mask, occupied);
      freeSquares = 64 - occupiedSq;
    }
    if (i == 0) {
      result = (ps[0].piece == PAWN) ? canonical48[ps[0].count][comb] : canonical64[ps[0].count][comb];
    } else {
      result = result * choose[freeSquares][ps[i].count] + comb;
    }
    occupied ^= mask;
    occupiedSq += ps[i].count;
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
 * memTable - if this is NULL, then we are at the first step of collecting wins/losses in 0/1 for our table.
 *   If this is not null, then we have computed some partial information that we can query.
 *   memTable != NULL also implies that none of the moves are captures (because we are now doing retrograde analysis).
 */
int forwardStep(Board *b, Move *m, int numMoves, PieceSet *ps, int nps, char *memTable) {
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
    int captures = isCapture(b, m[0]);
    for (int i = 0; i < numMoves; i++) {
      memcpy(&b2, b, sizeof(Board));
      makeMove(&b2, m[i]);
      int childScore;
      if (m[i].promotion) {
        childScore = sgn(evalBoard(&b2));
      } else if (memTable) {
        canonicalizeBoard(ps, nps, &b2);
        int index = getEgtbIndex(ps, nps, &b2);
        childScore = memTable[index];
      } else if (captures) {
        childScore = evalBoard(&b2);
      } else {
        // If there is no memTable and there are no promotions and no captures, then we are iterating this table for the first time
        // and this position isn't a conversion, so it's a draw / unknown.
        childScore = EGTB_DRAW;
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

void evaluatePlacement(PieceSet *ps, int nps, Board *b, int side, Move *m, char *memTable, FILE *tmpBoards) {
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
    memTable[index] = fileScore;
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
 * memTable - memory buffer that holds the EGTB scores
 * tmpBoards - FILE that collects the "lost/won in 0 or 1" tables we find during the initial scan
 */
void iterateEgtb(PieceSet *ps, int nps, int level, Board *b, Move *m, char *memTable, FILE *tmpBoards) {
  if (level == nps) {
    // Found a placement, now evaluate it.
    evaluatePlacement(ps, nps, b, WHITE, m, memTable, tmpBoards);
    evaluatePlacement(ps, nps, b, BLACK, m, memTable, tmpBoards);
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
      iterateEgtb(ps, nps, level + 1, b, m, memTable, tmpBoards);
      b->bb[baseBb] ^= mask;
      b->bb[baseBb + ps[level].piece] = 0ull;
      b->bb[BB_EMPTY] ^= mask;
    }
  }
}

/* Params: see iterateEgtb(). */
void iterateEpEgtbHelper(PieceSet *ps, int nps, int level, Board *b, Move *m, u64 occupied, char *memTable, FILE *tmpBoards) {
  if (level == nps) {
    evaluatePlacement(ps, nps, b, b->side, m, memTable, tmpBoards);
    return;
  }

  int base = (ps[level].side == WHITE) ? BB_WALL : BB_BALL;
  bool isPawn = ps[level].piece == PAWN;
  int gsize = isPawn ? (ps[level].count - 1) : ps[level].count;
  int freeSquares = (isPawn ? 48 : 64) - popCount(occupied);
  int numCombs = choose[freeSquares][gsize];

  for (int comb = 0; comb < numCombs; comb++) {
    u64 mask;
    if (isPawn) {
      mask = unrankCombination(comb, gsize, occupied >> 8) << 8;
    } else {
      mask = unrankCombination(comb, gsize, occupied);
    }
    b->bb[base] ^= mask;
    b->bb[base + ps[level].piece] ^= mask;
    b->bb[BB_EMPTY] ^= mask;
    iterateEpEgtbHelper(ps, nps, level + 1, b, m, occupied ^ mask, memTable, tmpBoards);
    b->bb[base] ^= mask;
    b->bb[base + ps[level].piece] ^= mask;
    b->bb[BB_EMPTY] ^= mask;
  }
}

/* Params: see iterateEgtb(). */
void iterateEpEgtb(PieceSet *ps, int nps, char *memTable, FILE *tmpBoards) {
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
    b.bb[BB_EMPTY] = ~(b.bb[allStm] ^ b.bb[allSntm]);
    u64 occupied = b.bb[allStm] | b.bb[BB_EP] | (b.bb[BB_EP] << 8) | (b.bb[BB_EP] >> 8);
    iterateEpEgtbHelper(ps, nps, 0, &b, m, occupied, memTable, tmpBoards);
  }
}

/* Writes the score we have found for this board, but also for all its rotations, if they are canonical.
 * An example of why this is necessary: Kc3/ke1/w is canonical and it's a win in 2 (Kd2 wins).
 * Kc3/ka5/w is also canonical and a win in 2 (Kb4 wins). Retrograde analysis will correctly find the first position,
 * doing a backward move from Kd2/ke1/b. However, it won't solve the second one, because Kb4/ka5/b is not canonical. */
void scoreAllRotations(PieceSet *ps, int nps, Board *b, int index, int score, char *memTable, FILE *tmpBoards) {
  unsigned code = encodeEgtbBoard(ps, nps, b);
  fwrite(&code, sizeof(unsigned), 1, tmpBoards);
  int fileScore = (score > 0) ? (score + 1) : (score - 1);
  memTable[index] = fileScore;

  // Now try 1 or 7 rotations, based on whether there are pawns on the board
  int legalOri[7] = { ORI_FLIP_EW, ORI_ROT_CCW, ORI_ROT_180, ORI_ROT_CW, ORI_FLIP_NS, ORI_FLIP_DIAG, ORI_FLIP_ANTIDIAG };
  int oriCount = (ps[0].piece == PAWN) ? 1 : 7;
  for (int ori = 0; ori < oriCount; ori++) {
    Board bc = *b;
    rotateBoard(&bc, legalOri[ori]);
    if (canonicalizeBoard(ps, nps, &bc) == ORI_NORMAL) {
      index = getEgtbIndex(ps, nps, &bc);
      if (memTable[index] == EGTB_DRAW) {
        code = encodeEgtbBoard(ps, nps, &bc);
        fwrite(&code, sizeof(unsigned), 1, tmpBoards);
        memTable[index] = fileScore;
      }
    }
  }
}

void retrograde(PieceSet *ps, int nps, Board *b, int targetScore, char *memTable, FILE *tmpBoards, Move *mf, Move *mb) {
  int nb = getAllMoves(b, mb, BACKWARD);
  for (int i = 0; i < nb; i++) {
    // Make backward move
    Board br = *b; // Retro-board
    makeBackwardMove(&br, mb[i]);

    // Making a move may cause br to become non-canonical. Rotate it and the move that took us to it.
    int orientation = canonicalizeBoard(ps, nps, &br);
    Move mcopy = mb[i];
    rotateMove(&mcopy, orientation);

    // Make sure that one of the forward moves is symmetric to mb[i]. See the comments for getAllMoves() for details.
    int nf = getAllMoves(&br, mf, FORWARD);
    int sym = 0;
    while ((sym < nf) && ((mf[sym].piece != mcopy.piece) || (mf[sym].from != mcopy.to) || (mf[sym].to != mcopy.from))) {
      sym++;
    }
    if (sym < nf) {
      // Don't look at this position if we've already scored it
      int index = getEgtbIndex(ps, nps, &br);
      if (memTable[index] == EGTB_DRAW) {
        int score = forwardStep(&br, mf, nf, ps, nps, memTable);
        if ((score != EGTB_DRAW) && (abs(score) <= targetScore)) {
          scoreAllRotations(ps, nps, &br, index, score, memTable, tmpBoards);
        }
      }
    }
  }
}

bool generateEgtb(const char *combo) {
  string destName = getFileNameForCombo(combo);
  string compressedName = getCompressedFileNameForCombo(combo);
  if (fileExists(destName.c_str()) || fileExists(compressedName.c_str())) {
    log(LOG_INFO, "Table %s already exists, skipping", combo);
    return false;
  }
  u64 timer = timerGet();
  log(LOG_INFO, "Generating table %s into file %s", combo, destName.c_str());
  const char *tmpBoardName1 = "/tmp/egtb-gen-boards1";
  const char *tmpBoardName2 = "/tmp/egtb-gen-boards2";
  PieceSet ps[EGTB_MEN];
  int numPieceSets = comboToPieceSets((char*)combo, ps);
  Board b;
  Move m[200];

  // First populate all the immediate wins / losses (no legal moves) and immediate conversions (win/loss in 1).
  // Collect all the positions thus evaluated, in encoded form, in a temporary file.
  emptyBoard(&b);
  int size = getEgtbSize(ps, numPieceSets) + getEpEgtbSize(ps, numPieceSets);
  char *memTable = (char*)malloc(size);
  memset(memTable, EGTB_DRAW, size);
  FILE *fBoards1 = fopen(tmpBoardName1, "w"), *fBoards2;
  iterateEgtb(ps, numPieceSets, 0, &b, m, memTable, fBoards1);
  if (ps[0].piece == PAWN && ps[1].piece == PAWN) {
    iterateEpEgtb(ps, numPieceSets, memTable, fBoards1);
  }
  fclose(fBoards1);
  int solved = getFileSize(tmpBoardName1) / sizeof(unsigned);
  log(LOG_INFO, "Discovered %d boards with wins and losses in 0 or 1 half-moves", solved);

  int targetScore = 1;
  Move mf[200], mb[200]; // Storage space for forward and backward moves
  while (getFileSize(tmpBoardName1) && targetScore < 126) {
    targetScore++;
    fBoards1 = fopen(tmpBoardName1, "r");
    fBoards2 = fopen(tmpBoardName2, "w");
    unsigned code;

    while (fread(&code, sizeof(unsigned), 1, fBoards1)) {
      decodeEgtbBoard(ps, numPieceSets, &b, code);
      retrograde(ps, numPieceSets, &b, targetScore, memTable, fBoards2, mf, mb);
    }

    fclose(fBoards1);
    fclose(fBoards2);
    unlink(tmpBoardName1);
    rename(tmpBoardName2, tmpBoardName1);
    int solvedStep = getFileSize(tmpBoardName1) / sizeof(unsigned);
    solved += solvedStep;
    log(LOG_DEBUG, "Discovered %d boards at score ±%d", solvedStep, targetScore);
  }
  if (targetScore == 126 && getFileSize(tmpBoardName1)) {
    appendEgtbNote("Table reached score 127", combo);
  }

  // Done! Dump the generated table in the EGTB folder and delete the temp files
  unlink(tmpBoardName1);
  log(LOG_DEBUG, "Dumping table to [%s]", destName.c_str());
  FILE *fTable = fopen(destName.c_str(), "w");
  fwrite(memTable, size, 1, fTable);
  fclose(fTable);
  free(memTable);
  log(LOG_INFO, "Table size: %d, of which non-draws: %d", size, solved);
  u64 delta = timerGet() - timer;
  log(LOG_INFO, "Generation time: %.3f s (%.3f positions/s)", delta / 1000.0, size / (delta / 1000.0));
  log(LOG_DEBUG, "Longest win/loss:");
  printBoard(&b);
  logCacheStats(&egtbCache, "EGTB");
  return true;
}

int egtbLookup(Board *b) {
  int wp = popCount(b->bb[BB_WALL]), bp = popCount(b->bb[BB_BALL]);
  if (!wp || !bp) {
    return EGTB_DRAW; // You're looking in the wrong place, buddy -- evalBoard() should catch this
  }
  if (wp + bp > EGTB_MEN) {
    return EGTB_DRAW;
  }

  changeSidesIfNeeded(b);

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

  PieceSet ps[EGTB_MEN];
  int nps = comboToPieceSets(combo, ps);
  return egtbLookupWithInfo(b, combo, ps, nps);
}

int egtbLookupWithInfo(Board *b, const char *combo, PieceSet *ps, int nps) {
  canonicalizeBoard(ps, nps, b);
  int index = getEgtbIndex(ps, nps, b);
  return readFromCache(combo, index);
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

void matchOrDie(bool condition, Board *b, int score, int minNeg, int maxNeg, int minPos, int maxPos, bool anyDraws, int *childScores, int numMoves) {
  if (!condition) {
    log(LOG_ERROR, "VERIFICATION ERROR: score %d, minNeg %d, maxNeg %d, minPos %d, maxPos %d, anyDraws %d",score, minNeg, maxNeg, minPos, maxPos, anyDraws);
    log(LOG_ERROR, "Child scores:");
    for (int i = 0; i < numMoves; i++) {
      log(LOG_ERROR, "%d", childScores[i]);
    }
    printBoard(b);
    assert(false);
  }
}

void egtbVerifyPosition(Board *b, Move *m, const char *combo, PieceSet *ps, int nps) {
  // Only check 10% of the positions. This is ok, because a bug is likely to affect at least hundreds of positions, and the chance of missing
  // 300 buggy positions is 0.90^300 = 1.8 * 10^-14
  if (rand() % 10) {
    return;
  }
  Board bc = *b;
  int score = egtbLookupWithInfo(&bc, combo, ps, nps);
  bc = *b;
  int numMoves = getAllMoves(&bc, m, FORWARD);

  // If no moves are possible, the score should be 1, 0 or -1 depending on the count of white and black pieces.
  if (!numMoves) {
    int stmCount = popCount((bc.side == WHITE) ? bc.bb[BB_WALL] : bc.bb[BB_BALL]);
    int sntmCount = popCount((bc.side == WHITE) ? bc.bb[BB_BALL] : bc.bb[BB_WALL]);
    if (stmCount > sntmCount) {
      assert(score == -1); // Lost now
    } else if (stmCount < sntmCount) {
      assert(score == 1); // Won now
    } else {
      assert(score == EGTB_DRAW);
    }
    return;
  }

  int childScore[numMoves]; // child scores
  int captures = isCapture(b, m[0]);
  for (int i = 0; i < numMoves; i++) {
    Board b2 = bc;
    makeMove(&b2, m[i]);
    if (captures || m[i].promotion) {
      childScore[i] = evalBoard(&b2);
    } else {
      childScore[i] = egtbLookupWithInfo(&b2, combo, ps, nps);
    }
  }

  // If all moves are captures, they all convert, so the score should be 2, 0 or -2.
  if (captures) {
    int anyNeg = false, allPos = true;
    for (int i = 0; i < numMoves; i++) {
      if (childScore[i] < 0) {
        anyNeg = true;
      }
      if (childScore[i] <= 0) {
        allPos = false;
      }
    }
    if (anyNeg) {
      assert(score == 2); // Convert to a win in 1
    } else if (allPos) {
      assert(score == -2); // Convert to a loss in 1
    } else {
      assert(score == 0); // Cannot win, but can convert to a draw
    }
    return;
  }

  // Promotions and normal moves
  int minNeg = INFTY, maxNeg = -INFTY, minPos = INFTY, maxPos = -INFTY, anyDraws = false;
  for (int i = 0; i < numMoves; i++) {
    int cs = childScore[i];
    if (m[i].promotion) {
      if (cs > 0) {
        cs = 1;
      } else if (cs < 0) {
        cs = -1;
      }
    }
    if (cs < 0 && cs < minNeg) {
      minNeg = cs;
    }
    if (cs < 0 && cs > maxNeg) {
      maxNeg = cs;
    }
    if (cs > 0 && cs > maxPos) {
      maxPos = cs;
    }
    if (cs > 0 && cs < minPos) {
      minPos = cs;
    }
    if (cs == EGTB_DRAW) {
      anyDraws = true;
    }
  }
  if (score > 0) {
    matchOrDie(maxNeg == -score + 1, &bc, score, minNeg, maxNeg, minPos, maxPos, anyDraws, childScore, numMoves);
  } else if (score < 0) {
    matchOrDie((maxPos == -score - 1) && (maxNeg == -INFTY) && !anyDraws, &bc, score, minNeg, maxNeg, minPos, maxPos, anyDraws, childScore, numMoves);
  } else {
    // Either there isn't a win/loss or we can't prove it in one byte.
    matchOrDie((anyDraws && (maxNeg == -INFTY)) || (maxPos == 127) || (minPos == -127),
               &bc, score, minNeg, maxNeg, minPos, maxPos, anyDraws, childScore, numMoves);
  }
}

void egtbVerifySideAndEp(Board *b, Move *m, const char *combo, PieceSet *ps, int nps) {
  // Check all the possible epSquares if White is to move
  for (int sq = 40; sq < 48; sq++) {
    u64 mask = 1ull << sq;
    if ((b->bb[BB_EMPTY] & mask) &&
        (b->bb[BB_EMPTY] & (mask << 8)) &&
        (b->bb[BB_BP] & (mask >> 8)) &&
        (b->bb[BB_WP] & RANK_5 & ((mask >> 7) ^ (mask >> 9)))) {
      b->bb[BB_EP] = mask;
      b->side = WHITE;
      egtbVerifyPosition(b, m, combo, ps, nps);
    }
  }
  for (int sq = 16; sq < 24; sq++) {
    u64 mask = 1ull << sq;
    if ((b->bb[BB_EMPTY] & mask) &&
        (b->bb[BB_EMPTY] & (mask >> 8)) &&
        (b->bb[BB_WP] & (mask << 8)) &&
        (b->bb[BB_BP] & RANK_4 & ((mask << 7) ^ (mask << 9)))) {
      b->bb[BB_EP] = mask;
      b->side = BLACK;
      egtbVerifyPosition(b, m, combo, ps, nps);
    }
  }
  b->bb[BB_EP] = 0ull;
  b->side = WHITE;
  egtbVerifyPosition(b, m, combo, ps, nps);
  b->side = BLACK;
  egtbVerifyPosition(b, m, combo, ps, nps);
}

/**
 * Recursively construct all possible positions of the given combo, canonical or not, including EP positions
 * combo - combination to verify, eg NNPvPP
 * side - side whose pieces we are currently placing (starts as White, switches to Black once we hit the 'v')
 * level - index of current piece set being placed
 * maxLevel - maximum numer of level (shortcut for strlen(combo))
 * b - board being constructed
 * m - reusable space for move generation during evaluatePlacement()
 * ps, nps - the piece sets for the combo, to speed up the EGTB lookups
 */
void egtbVerifyHelper(const char *combo, int side, int level, int maxLevel, int prevSq, Board *b, Move *m, PieceSet *ps, int nps) {
  if (level == maxLevel) {
    egtbVerifySideAndEp(b, m, combo, ps, nps);
  } else if (combo[level] == 'v') {
    egtbVerifyHelper(combo, BLACK, level + 1, maxLevel, 0, b, m, ps, nps);
  } else {
    int base = (side == WHITE) ? BB_WALL : BB_BALL;
    int piece = PIECE_BY_NAME[combo[level] - 'A'];
    int startSq = (piece == PAWN) ? 8 : 0;
    int endSq = (piece == PAWN) ? 56 : 64;
    if (level && (combo[level] == combo[level - 1])) {
      startSq = prevSq + 1;
    }
    for (int sq = startSq; sq < endSq; sq++) {
      u64 mask = 1ull << sq;
      if (b->bb[BB_EMPTY] & mask) {
        if (!level) {
          log(LOG_DEBUG, "  Level %d: placing a %c at %s", level, combo[level], SQUARE_NAME(sq).c_str());
          logCacheStats(&egtbCache, "EGTB");
        }
        b->bb[base] ^= mask;
        b->bb[base + piece] ^= mask;
        b->bb[BB_EMPTY] ^= mask;
        egtbVerifyHelper(combo, side, level + 1, maxLevel, sq, b, m, ps, nps);
        b->bb[base] ^= mask;
        b->bb[base + piece] ^= mask;
        b->bb[BB_EMPTY] ^= mask;
      }
    }
  }
}

void verifyEgtb(const char *combo) {
  u64 timer = timerGet();
  log(LOG_INFO, "Verifying table %s", combo);
  Board b;
  emptyBoard(&b);
  Move m[200];
  PieceSet ps[EGTB_MEN];
  int nps = comboToPieceSets(combo, ps);
  int size = getComboSize(combo);
  egtbVerifyHelper(combo, WHITE, 0, strlen(combo), 0, &b, m, ps, nps);
  u64 delta = timerGet() - timer;
  log(LOG_INFO, "Verification time: %.3f s (%.3f positions/s)", delta / 1000.0, size / (delta / 1000.0));
}

/* Converts a combination between 0 and choose(k + 5, k) to a string of k piece names */
string comboEnumerate(int comb, int k) {
  comb = choose[k + 5][k] - 1 - comb;
  u64 mask = unrankCombination(comb, k, 0ull);
  string result = "";
  int piece = PAWN;
  while (mask) {
    if (mask & 1) {
      result += PIECE_INITIALS[piece];
    } else {
      piece++;
    }
    mask >>= 1;
  }
  result = string(result.rbegin(), result.rend());
  return result;
}

void compressEgtb(const char *combo) {
  string name = getFileNameForCombo(combo);
  string compressedName = getCompressedFileNameForCombo(combo);
  string idxName = getIndexFileNameForCombo(combo);
  if (fileExists(compressedName.c_str()) && fileExists(idxName.c_str())) {
    return;
  }
  u64 timer = timerGet();
  int size = getComboSize(combo);
  compressFile(name.c_str(), compressedName.c_str(), idxName.c_str(), EGTB_CHUNK_SIZE, true);
  u64 delta = timerGet() - timer;
  log(LOG_INFO, "Compression time: %.3f s (%.3f positions/s)", delta / 1000.0, size / (delta / 1000.0));
}

void generateAllEgtb(int wc, int bc) {
  for (int i = 0; i < choose[wc + 5][wc]; i++) {
    string ws = comboEnumerate(i, wc);
    for (int j = 0; j < choose[bc + 5][bc]; j++) {
      string bs = comboEnumerate(j, bc);
      if ((wc > bc) || (i <= j)) {
        string combo = ws + "v" + bs;
        int size = getComboSize(combo.c_str());
        u64 timer = timerGet(), delta;
        if (generateEgtb(combo.c_str())) {
          verifyEgtb(combo.c_str());
        }
        compressEgtb(combo.c_str());
        delta = timerGet() - timer;
        log(LOG_INFO, "Total G + V + C time: %.3f s (%.3f positions/s)", delta / 1000.0, size / (delta / 1000.0));
      }
    }
  }
}
