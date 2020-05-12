#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "board.h"
#include "configfile.h"
#include "defines.h"
#include "egtb.h"
#include "egtb_hash.h"
#include "egtb_queue.h"
#include "fileutil.h"
#include "logging.h"
#include "lrucache.h"
#include "movegen.h"
#include "precomp.h"
#include "timer.h"

LruCache egtbCache;

/**
 * Data for the table currently being built. The possible combined values are
 * memOpen[i] = 0, memScore[i] < 0: position evaluated to a loss
 * memOpen[i] = 0, memScore[i] = 0: position evaluated to a draw
 * memOpen[i] = 0, memScore[i] > 0: position evaluated to a win
 * memOpen[i] > 0, memScore[i] < 0: open position, all children so far lose in memScore[i] or less
 * memOpen[i] > 0, memScore[i] = 0: open position, best we can do so far is draw
 * memOpen[i] > 0, memScore[i] > 0: undefined
 */
char *memScore; // score -- the data we will eventually dump to the file
byte *memOpen;  // number of open children
EgtbQueue* retro; // positions left to consider in BFS retrograde analysis
Board scanB; // construct positions here during scan()
EgtbHash egtbHash; // to prevent duplicates in child or parent lists

void initEgtb() {
  egtbCache = lruCacheCreate(cfgEgtbChunks);
}

/**
 * Evaluates b to 1 (immediate win) or 0 (draw). The side to move is assumed
 * to be stalemated. Uses joint rules: win if STM has fewer pieces, draw
 * otherwise.
 */
int evalStalemate(Board* b) {
  int delta = popCount(b->bb[BB_WALL]) - popCount(b->bb[BB_BALL]);
  return
    (b->side == WHITE && delta < 0) ||
    (b->side == BLACK && delta > 0);
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
  } else {
    log(LOG_WARNING, "Missing EGTB file for combo %s", combo);
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
    // encode a pawn on the first / last line to indicate there is an EP square
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

/**
 * Called during the initial scan of all possible placements for a table.
 *
 * For the current board in scanB, computes:
 *   * memScore: scores decidable positions, initializes open ones;
 *   * memOpen: counts the open children (how many times scanB expects to be
 *     notified);
 *   * if scanB is solved, adds it to the BFS queue.
 *
 * Decidable positions are scored as:
 *   * +1/0 (won/drawn now if stalemate);
 *   * +2 (win in 1 by converting to some lost position);
 *   * -2 (lose in 1 because all moves convert to winning positions);
 *   * 0 (at least one drawn conversion; all other conversions draw/win);
 *   * open (no conversion to a lost position; at least one open child);
 *     * in this case, take note of:
 *       * whether or not a draw is assured by the solved children;
 *       * number of open children.
 */
void evaluatePlacement(PieceSet *ps, int nps) {
  // We can still generate non-canonical boards, but at least we recognize
  // them as such.
  if (canonicalizeBoard(ps, nps, &scanB, true) != ORI_NORMAL) {
    return;
  }

  // allocate these only once  
  static Move m[MAX_MOVES];
  static Board b2;

  int numMoves = getAllMoves(&scanB, m, FORWARD);
  int index = getEgtbIndex(ps, nps, &scanB);
  memScore[index] = memOpen[index] = 0;

  if (!numMoves) {
    memScore[index] = evalStalemate(&scanB);
  } else {
    bool haveWin = false, haveDraw = false;
    int captures = isCapture(&scanB, m[0]);
    int i = 0;

    egtbHash.clear();
    while ((i < numMoves) && !haveWin) {
      b2 = scanB;
      makeMove(&b2, m[i]);

      if (captures || m[i].promotion) {
        // conversion: evaluate child now
        int childScore = egtbLookup(&b2);
        if (childScore < 0) {
          haveWin = true;
        } else if (childScore == 0) {
          haveDraw = true;
        }
      } else {
        canonicalizeBoard(ps, nps, &b2, false);
        unsigned childIndex = getEgtbIndex(ps, nps, &b2);
        if (!egtbHash.contains(childIndex)) {
          egtbHash.add(childIndex);
          memOpen[index]++;
        }
      }
      i++;
    }
    if (haveWin) {
      memScore[index] = 2;
      memOpen[index] = 0;
    } else if (memOpen[index]) {
      // Open position; note whether or not we have a draw so far. The -1
      // value is relevant. This will increase in time if we find
      // longer-lasting losses or it will be upgraded to 0 if we find a draw.
      memScore[index] = haveDraw ? 0 : -1;
    } else if (haveDraw) {
      memScore[index] = 0;
    } else { // all children evaluated and no draw found
      memScore[index] = -2;
    }
  }
  if (!memOpen[index]) {
    retro->enqueue(encodeEgtbBoard(ps, nps, &scanB), index);
  }
}

/**
 * Recursively iterate over all possible placements of the piece sets.
 * Does not deal with EP positions -- those are handled separately by scanEp().
 * ps - array of piece sets
 * nps - number of piece sets
 * level - index of current piece set being placed
 */
void scan(PieceSet *ps, int nps, int level) {
  if (level == nps) {
    // Found a placement, now evaluate it.
    scanB.side = WHITE;
    evaluatePlacement(ps, nps);
    scanB.side = BLACK;
    evaluatePlacement(ps, nps);
    return;
  }

  int baseBb = (ps[level].side == WHITE) ? BB_WALL : BB_BALL;
  int gsize = ps[level].count;
  bool isPawn = ps[level].piece == PAWN;
  int freeSquares = (isPawn ? 48 : 64) - getPieceCount(&scanB);
  int numCombs = choose[freeSquares][gsize];
  u64 occupied = scanB.bb[BB_WALL] ^ scanB.bb[BB_BALL];
  if (isPawn) {
    occupied >>= 8;
  }

  for (int comb = 0; comb < numCombs; comb++) {
    // For the first piece set, the combination must be canonical. Otherwise,
    // all combinations are acceptable.
    bool acceptable = (level == 0)
      ? (isPawn
         ? (canonical48[gsize][comb] >= 0)
         : (canonical64[gsize][comb] >= 0))
      : true;
    if (acceptable) {
      u64 mask = unrankCombination(comb, gsize, occupied);
      if (isPawn) {
        mask <<= 8;
      }
      scanB.bb[baseBb] ^= mask;
      scanB.bb[baseBb + ps[level].piece] = mask;
      scanB.bb[BB_EMPTY] ^= mask;
      scan(ps, nps, level + 1);
      scanB.bb[baseBb] ^= mask;
      scanB.bb[baseBb + ps[level].piece] = 0ull;
      scanB.bb[BB_EMPTY] ^= mask;
    }
  }
}

bool hasRightAndLeftEpPawns(u64 mask) {
  u64 pawnsToMove = (scanB.side == WHITE) ? scanB.bb[BB_WP] : scanB.bb[BB_BP];
  u64 rightMask = (scanB.side == WHITE)
    ? (scanB.bb[BB_EP] >> 7)
    : (scanB.bb[BB_EP] << 9);
  if (pawnsToMove & rightMask) {     // if there is a pawn on the right
    u64 leftMask = (rightMask & ~FILE_B) >> 2; // may be empty
    return mask & leftMask;          // proposed mask covers bad square
  } else {
    return false;
  }
}

/* Params: see scan(). */
void scanEpHelper(PieceSet *ps, int nps, int level, u64 occupied) {
  if (level == nps) {
    evaluatePlacement(ps, nps);
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

    // If there is a pawn on the right that can capture en passant, don't
    // allow a second one on the left. That would enqueue duplicate positions
    // during the initial scan, which would cause duplicate notifications and
    // early closures down the road.
    bool doubleEp =
      isPawn &&                          // placing more pawns...
      (ps[level].side == scanB.side) &&  // ... for the side that can capture...
      hasRightAndLeftEpPawns(mask);      // ... and of them creates the forbidden setup

    if (!doubleEp) {
      scanB.bb[base] ^= mask;
      scanB.bb[base + ps[level].piece] ^= mask;
      scanB.bb[BB_EMPTY] ^= mask;
      scanEpHelper(ps, nps, level + 1, occupied ^ mask);
      scanB.bb[base] ^= mask;
      scanB.bb[base + ps[level].piece] ^= mask;
      scanB.bb[BB_EMPTY] ^= mask;
    }
  }
}

/* Params: see scan(). */
void scanEp(PieceSet *ps, int nps) {
  // Generate the 14 canonical placements for the pair of pawns.
  for (int i = 0; i < 14; i++) {
    emptyBoard(&scanB);
    scanB.side = (i < 7) ? WHITE : BLACK;
    int index = i % 7;
    int allStm = (scanB.side == WHITE) ? BB_WALL : BB_BALL;
    int allSntm = BB_WALL + BB_BALL - allStm;
    int file = (index + 1) / 2;
    scanB.bb[BB_EP] = ((scanB.side == WHITE) ? 0x0000010000000000ull : 0x0000000000010000ull) << file;
    scanB.bb[allSntm + PAWN] = scanB.bb[allSntm] =
      (scanB.side == WHITE)
      ? (scanB.bb[BB_EP] >> 8)
      : (scanB.bb[BB_EP] << 8);
    scanB.bb[allStm + PAWN] = scanB.bb[allStm] =
      (index & 1)
      ? (scanB.bb[allSntm] >> 1)
      : (scanB.bb[allSntm] << 1);
    scanB.bb[BB_EMPTY] = ~(scanB.bb[allStm] ^ scanB.bb[allSntm]);
    u64 occupied = scanB.bb[allStm] | scanB.bb[BB_EP] | (scanB.bb[BB_EP] << 8) | (scanB.bb[BB_EP] >> 8);
    scanEpHelper(ps, nps, 0, occupied);
  }
}

void scanWrapper(PieceSet *ps, int nps, int level) {
  emptyBoard(&scanB);
  scan(ps, nps, 0);
  if (ps[0].piece == PAWN && ps[1].piece == PAWN) {
    scanEp(ps, nps);
  }
}

/**
 * Notifies b that one of b's children has been solved. b is assumed to be
 * canonical.
 *
 * @param unsigned index b's index
 * @param int score The child's score
 */
void notifyBoard(PieceSet *ps, int nps, Board *b, unsigned index, int score) {
  // Skip this position if we've already scored it
  if (memOpen[index]) {
    memOpen[index]--;
    if (score < 0) {
      // child loses => parent converts to a win in -score + 1
      memScore[index] = -score + 1;
      memOpen[index] = 0;
    } else if (score == 0) {
      // child draws => parent has a guaranteed draw
      memScore[index] = 0;
    } else if (memScore[index] < 0) {
      // child wins and parent was losing so far: compare
      memScore[index] = MIN(memScore[index], -score - 1);
    } else {
      // child wins, but parent had a draw: nothing
    }

    if (!memOpen[index]) {
      retro->enqueue(encodeEgtbBoard(ps, nps, b), index); 
    }
  }
}

/**
 * Expands a solved position, notifying its parents.
 */
void retrograde(PieceSet *ps, int nps, Board *b, unsigned index) {
  static Move mb[MAX_MOVES];
  static Board parentB;

  int nb = getAllMoves(b, mb, BACKWARD);
  int score = memScore[index];

  egtbHash.clear();
  for (int i = 0; i < nb; i++)  {
    parentB = *b;
    makeBackwardMove(&parentB, mb[i]);
    canonicalizeBoard(ps, nps, &parentB, false);
    unsigned parentIndex = getEgtbIndex(ps, nps, &parentB);
    if (!egtbHash.contains(parentIndex)) {
      egtbHash.add(parentIndex);
      notifyBoard(ps, nps, &parentB, parentIndex, score);
    }
  }
}

void dumpTable(string destName, int size) {
  log(LOG_DEBUG, "Dumping table to [%s]", destName.c_str());
  FILE *f = fopen(destName.c_str(), "w");
  fwrite(memScore, size, 1, f);
  fclose(f);
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
  PieceSet ps[EGTB_MEN];
  int numPieceSets = comboToPieceSets((char*)combo, ps);

  // Collect and enqueue all the immediate stalemates and conversions.
  int size = getEgtbSize(ps, numPieceSets) + getEpEgtbSize(ps, numPieceSets);
  log(LOG_INFO, "Table size: %d", size);
  memScore = (char*)malloc(size);
  memOpen = (byte*)malloc(size);
  retro = new EgtbQueue(size);

  scanWrapper(ps, numPieceSets, 0);
  log(LOG_INFO, "Discovered %d boards with stalemate or conversion", retro->getTotal());

  // Loop de loop.
  int max = 0; // absolute maximum value encountered so far
  while (!retro->isEmpty() && max < 127) {
    unsigned code, index;
    Board b;
    retro->dequeue(&code, &index);
    decodeEgtbBoard(ps, numPieceSets, &b, code);
    int score = memScore[index];
    if (abs(score) > max) {
      max = abs(score);
      log(LOG_DEBUG, "Encountered score ±%d", max);
    }
    retrograde(ps, numPieceSets, &b, index);
  }

  if (!retro->isEmpty()) {
    appendEgtbNote("Table reached score 127", combo);
  }

  // Any still open positions are draws.
  for (int i = 0; i < size; i++) {
    if (memOpen[i]) {
      memScore[i] = 0;
    }
  }

  // Done! Dump the generated table in the EGTB folder and delete the temp files
  log(LOG_INFO, "Table size: %d, of which non-draws: %d", size, retro->getTotal());
  dumpTable(destName, size);
  free(memScore);
  free(memOpen);
  delete retro;
  u64 delta = timerGet() - timer;
  log(LOG_INFO, "Generation time: %.3f s (%.3f positions/s)", delta / 1000.0, size / (delta / 1000.0));
  logCacheStats(LOG_INFO, &egtbCache, "EGTB");
  return true;
}

int egtbLookup(Board *b) {
  int wp = popCount(b->bb[BB_WALL]), bp = popCount(b->bb[BB_BALL]);
  if (!wp) {
    return (b->side == WHITE) ? 1 : -1; // Won/lost now
  }
  if (!bp) {
    return (b->side == WHITE) ? -1 : 1; // Lost/won now
  }
  if (wp + bp > EGTB_MEN) {
    return EGTB_UNKNOWN;
  }

  changeSidesIfNeeded(b, wp, bp);

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
  canonicalizeBoard(ps, nps, b, false);
  int index = getEgtbIndex(ps, nps, b);
  return readFromCache(combo, index);
}

int batchEgtbLookup(Board *b, string *moveNames, string *fens, int *scores, int *numMoves) {
  Board bcopy = *b;
  int result = egtbLookup(&bcopy);
  if (result == INFTY) {
    *numMoves = 0;
  } else {
    Move m[MAX_MOVES];
    *numMoves = getAllMoves(b, m, FORWARD);
    getAlgebraicNotation(b, m, *numMoves, moveNames);
    for (int i = 0; i < *numMoves; i++) {
      Board b2 = *b;
      makeMove(&b2, m[i]);
      fens[i] = boardToFen(&b2);
      scores[i] = egtbLookup(&b2);
    }
  }
  return result;
}

void matchOrDie(bool condition, Board *b, int score, int minNeg, int maxNeg,
                int minPos, int maxPos, bool anyDraws, int *childScores, int numMoves,
                PieceSet *ps, int nps) {
  if (!condition) {
    printBoard(b);

    log(LOG_ERROR,
        "VERIFICATION ERROR: score %d, minNeg %d, maxNeg %d, minPos %d, maxPos %d, anyDraws %d",
        score, minNeg, maxNeg, minPos, maxPos, anyDraws);
    log(LOG_ERROR, "Child scores:");
    for (int i = 0; i < numMoves; i++) {
      log(LOG_ERROR, "%d", childScores[i]);
    }
    assert(false);
  }
}

void egtbVerifyPosition(Board *b, Move *m, const char *combo, PieceSet *ps, int nps) {
  // Only check 1/8th of the positions. This is OK, because a bug is likely to affect
  // at least hundreds of positions, and the chance of missing 200 buggy positions is
  // 0.875^200 = 2.5 * 10^-12.
  if (rand() & 7) {
    return;
  }
  Board bc = *b;
  int score = egtbLookupWithInfo(&bc, combo, ps, nps);
  bc = *b;
  int numMoves = getAllMoves(&bc, m, FORWARD);

  if (!numMoves) {
    // Joint rules: side to move wins or draws depending on the piece counts.
    int delta = popCount(bc.bb[BB_WALL]) - popCount(bc.bb[BB_BALL]);
    if (bc.side == WHITE) {
      assert(score == ((delta < 0) ? 1 : 0));
    } else {
      assert(score == ((delta > 0) ? 1 : 0));
    }
    return;
  }

  int childScore[numMoves]; // child scores
  int captures = isCapture(b, m[0]);
  for (int i = 0; i < numMoves; i++) {
    Board b2 = bc;
    makeMove(&b2, m[i]);
    if (captures || m[i].promotion) {
      childScore[i] = egtbLookup(&b2);
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
    if (cs == 0) {
      anyDraws = true;
    }
  }
  if (score > 0) {
    matchOrDie(maxNeg == -score + 1,
               &bc, score, minNeg, maxNeg, minPos, maxPos,
               anyDraws, childScore, numMoves, ps, nps);
  } else if (score < 0) {
    matchOrDie((maxPos == -score - 1) && (maxNeg == -INFTY) && !anyDraws,
               &bc, score, minNeg, maxNeg, minPos, maxPos,
               anyDraws, childScore, numMoves, ps, nps);
  } else {
    // Either there isn't a win/loss or we can't prove it in one byte.
    matchOrDie((anyDraws && (maxNeg == -INFTY)) || (maxPos == 127) || (minPos == -127),
               &bc, score, minNeg, maxNeg, minPos, maxPos,
               anyDraws, childScore, numMoves, ps, nps);
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
 * m - reusable space for move generation
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
          logCacheStats(LOG_DEBUG, &egtbCache, "EGTB");
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
  Move m[MAX_MOVES];
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
