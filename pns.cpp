#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include "board.h"
#include "logging.h"
#include "movegen.h"
#include "stringutil.h"
#include "zobrist.h"

/* hash table of positions anywhere in the PNS tree */
TranspositionTable trans;

/* set of nods on any path from the MPN to the root */
NodeSet pnsAncestors;

/* Creates a 1/1 node. */
PnsTree* pnsMakeLeaf() {
  PnsTree *t = (PnsTree*)malloc(sizeof(PnsTree));
  t->proof = t->disproof = 1;
  t->numChildren = t->numParents = 0;
  t->child = t->parent = NULL;
  t->move = NULL;
  t->extra = 0;
  return t;
}

/* Adds a parent to a PNS node (presumably because we found a transposed path to it). */
PnsTree* pnsAddParent(PnsTree *t, PnsTree *parent) {
  t->parent = (PnsTree**)realloc(t->parent, (t->numParents + 1) * sizeof(PnsTree*));
  t->parent[t->numParents++] = parent;
}

void pnsPrintTree(PnsTree *t, int level) {
  for (int i = 0; i < t->numChildren; i++) {
    for (int j = 0; j < level; j++) {
      printf("    ");
    }
    string from = SQUARE_NAME(t->move[i].from);
    string to = SQUARE_NAME(t->move[i].to);
    printf("%s%s %llu/%llu\n", from.c_str(), to.c_str(), t->child[i]->proof, t->child[i]->disproof);
    pnsPrintTree(t->child[i], level + 1);
  }
}

void pnsFree(PnsTree *t) {
  if (t) {
    for (int i = 0; i < t->numChildren; i++) {
      // If a child has several parents, free it the last time we visit it
      if (t == t->child[i]->parent[t->child[i]->numParents - 1]) {
        pnsFree(t->child[i]);
      }
    }
    if (t->parent) {
      free(t->parent);
    }
    if (t->child) {
      free(t->child);
    }
    if (t->move) {
      free(t->move);
    }
    free(t);
  }
}

/* Clears the extra information field */
void pnsClearExtra(PnsTree *t) {
  if (t) {
    t->extra = 0;
    for (int i = 0; i < t->numChildren; i++) {
      // If a child has several parents, clear it the first time we visit it
      if (t == t->child[i]->parent[0]) {
        pnsClearExtra(t->child[i]);
      }
    }
  }
}

void pnsAddAncestors(PnsTree *t) {
  if (t) {
    bool insertSuccess = pnsAncestors.insert(t).second;
    if (insertSuccess) { // new element
      for (int i = 0; i < t->numParents; i++) {
        pnsAddAncestors(t->parent[i]);
      }
    }
  }
}

/* Finds the most proving node in a PNS tree. Starting with the original board b, also makes the necessary moves
 * modifying b, returning the position corresponding to the MPN. */
PnsTree* pnsSelectMpn(PnsTree *t, Board *b) {
  while (t->numChildren) {
    int i = 0;
    while (i < t->numChildren && t->child[i]->disproof != t->proof) {
      i++;
    }
    assert(i < t->numChildren);
    makeMove(b, t->move[i]);
    t = t->child[i];
  }

  pnsAncestors.clear();
  pnsAddAncestors(t);
  return t;
}

/* Sets the proof and disproof numbers for a board with no legal moves */
void pnsSetScoreNoMoves(PnsTree *t, Board *b) {
  int indexMe = (b->side == WHITE) ? BB_WALL : BB_BALL;
  int indexOther = BB_WALL + BB_BALL - indexMe;
  int countMe = popCount(b->bb[indexMe]);
  int countOther = popCount(b->bb[indexOther]);
  t->proof = (countMe < countOther) ? 0 : INFTY64;
  t->disproof = (countMe > countOther) ? 0 : INFTY64;
}

/* Sets the proof and disproof numbers for an EGTB board */
void pnsSetScoreEgtb(PnsTree *t, int score) {
  if (score == EGTB_DRAW) { // EGTB draw
    t->proof = INFTY64;
    t->disproof = INFTY64;
  } else if (score > 0) {   // EGTB win
    t->proof = 0;
    t->disproof = INFTY64;
  } else {                  // EGTB loss
    t->proof = INFTY64;
    t->disproof = 0;
  }
}

void pnsExpand(PnsTree *t, Board *b) {
  int score = evalBoard(b);
  if (score != EGTB_UNKNOWN) {
    pnsSetScoreEgtb(t, score);                  // EGTB node
  } else {
    Move m[MAX_MOVES];
    t->numChildren = getAllMoves(b, m, FORWARD);
    if (!t->numChildren) {                      // No legal moves
      t->child = NULL;
      t->move = NULL;
      pnsSetScoreNoMoves(t, b);
    } else {                                    // Regular node
      t->child = (PnsTree**)malloc(t->numChildren * sizeof(PnsTree*));
      t->move = (Move*)malloc(t->numChildren * sizeof(Move));
      memcpy(t->move, m, t->numChildren * sizeof(Move));
      u64 z = getZobrist(b);
      for (int i = 0; i < t->numChildren; i++) {
        u64 z2 = updateZobrist(z, b, m[i]);
        PnsTree *orig = trans[z2];
        if (!orig) {                            // Regular child
          t->child[i] = pnsMakeLeaf();
          trans[z2] = t->child[i];
        } else if (pnsAncestors.find(orig) == pnsAncestors.end()) { // Transposition
          t->child[i] = orig;
        } else {                                // Repetition
          t->child[i] = pnsMakeLeaf();
          t->child[i]->proof = t->child[i]->disproof = INFTY64;
        }
        pnsAddParent(t->child[i], t);
      }
    }
  }
}

/* Propagate this node's values to each of its parents. */
void pnsUpdate(PnsTree *t) {
  if (t) {
    if (t->numChildren) {
      // If t has no children, then presumably it's a stalemate or EGTB position, so it already has correct P/D values.
      t->proof = INFTY64;
      t->disproof = 0;
      for (int i = 0; i < t->numChildren; i++) {
        t->proof = MIN(t->proof, t->child[i]->disproof);
        t->disproof += t->child[i]->proof;
      }
      t->disproof = MIN(t->disproof, INFTY64);
    }
    for (int i = 0; i < t->numParents; i++) {
      pnsUpdate(t->parent[i]);
    }
  }
}

/* Recalculates the P/D numbers for a freshly loaded tree (since we only store these numbers for leaves).
 * TODO: avoid duplicating effort for transpositions. */
void pnsRecalculateNumbers(PnsTree *t) {
  if (t->numChildren) {
    t->proof = INFTY64;
    t->disproof = 0;
    for (int i = 0; i < t->numChildren; i++) {
      pnsRecalculateNumbers(t->child[i]);
      t->proof = MIN(t->proof, t->child[i]->disproof);
      t->disproof += t->child[i]->proof;
    }
    t->disproof = MIN(t->disproof, INFTY64);
  }
}

void pnsAnalyzeBoard(PnsTree *root, Board *b, int maxNodes) {
  int treeSize = 1;
  while ((!maxNodes || (treeSize < maxNodes)) &&
         root->proof && root->disproof &&
         (root->proof < INFTY64 || root->disproof < INFTY64)) {
    Board current = *b;
    PnsTree *mpn = pnsSelectMpn(root, &current);
    pnsExpand(mpn, &current);
    pnsUpdate(mpn);
    treeSize += mpn->numChildren;
  }
  printf("Tree size %d, proof %llu, disproof %llu\n", treeSize, root->proof, root->disproof);
}

/* Encodes a move on 16 bits for saving. */
void pnsEncodeMove(Move m, FILE *f) {
  // 6 bits for from;
  // 6 bits for to;
  // 1 bit for promotion;
  // 3 bits for piece being moved / being promoted
  unsigned short x = (m.from << 10) ^ (m.to << 4);
  if (m.promotion) {
    x ^= 8 ^ m.promotion;
  } else {
    x ^= m.piece;
  }
  fwrite(&x, 2, 1, f);
}

Move pnsDecodeMove(FILE *f) {
  unsigned short x;
  assert(fread(&x, 2, 1, f) == 1);
  Move m;
  m.from = x >> 10;
  m.to = (x >> 4) & 0x3f;
  if (x & 8) {
    m.piece = PAWN;
    m.promotion = x & 7;
  } else {
    m.piece = x & 7;
  }
  return m;
}

/* Encode INFTY64 as 0 because it is very frequent and it shouldn't take 8 bytes.
 * Move everything else 1 up. */
void pnsEncodeNumber(u64 x, FILE *f) {
  if (x == INFTY64) {
    varintPut(0, f);
  } else {
    varintPut(x + 1, f);
  }
}

u64 pnsDecodeNumber(FILE *f) {
  u64 x = varintGet(f);
  return x ? (x - 1) : INFTY64;
}

void pnsSaveNode(PnsTree *t, PnsTree *parent, FILE *f) {
  if (t->extra) {
    // We are a transposition -- write 0xff for numChildren
    fputc(0xff, f);
  } else {
    t->extra = 1;
    fputc(t->numChildren, f);
    if (t->numChildren) {
      for (int i = 0; i < t->numChildren; i++) {
        // Technically, we don't need to save the moves; we know the order in which getAllMoves() generates them.
        // But we save them anyway, in case we ever change the move ordering.
        pnsEncodeMove(t->move[i], f);
        pnsSaveNode(t->child[i], t, f);
      }
    } else {
      pnsEncodeNumber(t->proof, f);
      pnsEncodeNumber(t->disproof, f);
    }
  }
}

PnsTree* pnsLoadNode(FILE *f, PnsTree *parent, Board *b, u64 zobrist) {
  int numChildren = fgetc(f);
  if (numChildren == 0xff) {
    // If the node is a transposition, just add _parent_ to its parent list.
    PnsTree *orig = trans[zobrist];
    assert(orig);
    pnsAddParent(orig, parent);
    return orig;
  }

  // Create a new node and add it to the transposition table
  PnsTree *t = pnsMakeLeaf();
  pnsAddParent(t, parent);
  if (!trans[zobrist]) {
    // Repetition nodes appear multiple times in the tree -- only hash the first one.
    trans[zobrist] = t;
  }
  t->numChildren = numChildren;

  // For internal nodes, update the board and the Zobrist key and recurse
  if (t->numChildren) {
    Move m[MAX_MOVES];
    int numMoves = getAllMoves(b, m, FORWARD);
    assert(numMoves == t->numChildren);
    t->child = (PnsTree**)malloc(t->numChildren * sizeof(PnsTree*));
    t->move = (Move*)malloc(t->numChildren * sizeof(Move));
    for (int i = 0; i < t->numChildren; i++) {
      t->move[i] = pnsDecodeMove(f);
      assert(equalMove(t->move[i], m[i]));
      Board b2 = *b;
      u64 zobrist2 = updateZobrist(zobrist, &b2, m[i]);
      makeMove(&b2, m[i]);
      t->child[i] = pnsLoadNode(f, t, &b2, zobrist2);
    }
  } else {
    // For leaves, just parse the proof / disproof numbers.
    t->move = NULL;
    t->child = NULL;
    t->proof = pnsDecodeNumber(f);
    t->disproof = pnsDecodeNumber(f);
  }
  return t;
}

void pnsSaveTree(PnsTree *t, Board *b, string fileName) {
  FILE *f = fopen(fileName.c_str(), "w");
  fwrite(b, sizeof(Board), 1, f);
  pnsSaveNode(t, NULL, f);
  fclose(f);
  pnsClearExtra(t);
}

/* Loads a PN^2 tree from fileName and checks that it applies to b.
 * If fileName does not exist, then creates a 1-node tree. */
PnsTree* pnsLoadTree(Board *b, string fileName) {
  FILE *f = fopen(fileName.c_str(), "r");
  if (f) {
    Board b2;
    assert(fread(&b2, sizeof(Board), 1, f) == 1);
    if (!equalBoard(b, &b2)) {
      printBoard(&b2);
      die("Input file stores a PN^2 tree for a different board (see above).");
    }
    PnsTree *t = pnsLoadNode(f, NULL, &b2, getZobrist(&b2));
    fclose(f);
    pnsRecalculateNumbers(t);
    log(LOG_INFO, "Loaded tree from %s.", fileName.c_str());
    return t;

  } else if (errno == ENOENT) { // File does not exist
    log(LOG_INFO, "File %s does not exist, creating new tree.", fileName.c_str());
    PnsTree *t = pnsMakeLeaf();
    trans[getZobrist(b)] = t;
    return t;

  } else { // File exists, but cannot be read for other reasons
    die("Input file [%s] exists, but cannot be read.", fileName.c_str());
  }
}

void pn2AnalyzeString(string input, string fileName) {
  Board *b;
  if (isFen(input)) {
    // Input is a board in FEN notation
    b = fenToBoard(input.c_str());
  } else {
    // Input is a sequence of moves. Tokenize it.
    stringstream in(input);
    string moves[MAX_MOVES];
    int n = 0;
    while (in.good() && n < MAX_MOVES){
      in >> moves[n++];
    }
    if (moves[n - 1].empty()) {
      n--;
    }
    b = makeMoveSequence(n, moves);
  }
  PnsTree *t = pnsLoadTree(b, fileName);
  pnsAnalyzeBoard(t, b, PNS_STEP_SIZE);
  pnsSaveTree(t, b, fileName);
  free(b);
  pnsFree(t);
}
