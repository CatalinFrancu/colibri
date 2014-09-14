#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include "board.h"
#include "logging.h"
#include "movegen.h"
#include "stringutil.h"

PnsTree* pnsMakeLeaf() {
  PnsTree *t = (PnsTree*)malloc(sizeof(PnsTree));
  t->proof = t->disproof = 1;
  t->numChildren = t->numParents = 0;
  t->child = t->parent = NULL;
  t->move = NULL;
  return t;
}

/* Deallocates a PnsTree. */
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

/* Sets the proof and disproof numbers for a board with no legal moves */
void pnsSetScoreNoMoves(PnsTree *t, Board *b) {
  int indexMe = (b->side == WHITE) ? BB_WALL : BB_BALL;
  int indexOther = BB_WALL + BB_BALL - indexMe;
  int countMe = popCount(b->bb[indexMe]);
  int countOther = popCount(b->bb[indexOther]);
  t->proof = (countMe < countOther) ? 0 : INFTY64;
  t->disproof = (countMe > countOther) ? 0 : INFTY64;
}

/* Finds the most proving node in a PNS tree. Starting with the original board b, also makes the necessary moves
 * modifying b, returning the position corresponding to the MPN. */
PnsTree* pnsSelectMpn(PnsTree *t, Board *b) {
  while (t->numChildren) {
    int i = 0;
    while (t->child[i]->disproof != t->proof) {
      i++;
    }
    makeMove(b, t->move[i]);
    t = t->child[i];
  }
  return t;
}

void pnsExpand(PnsTree *t, Board *b) {
  int score = evalBoard(b);
  if (score == EGTB_UNKNOWN) {
    Move m[MAX_MOVES];
    t->numChildren = getAllMoves(b, m, FORWARD);
    if (t->numChildren) {                                               // Regular node
      t->move = (Move*)malloc(t->numChildren * sizeof(Move));
      memcpy(t->move, m, t->numChildren * sizeof(Move));
      t->proof = 1;
      t->disproof = t->numChildren;
      t->child = (PnsTree**)malloc(t->numChildren * sizeof(PnsTree*));
      for (int i = 0; i < t->numChildren; i++) {
        t->child[i] = pnsMakeLeaf();
        t->child[i]->numParents = 1;
        t->child[i]->parent = (PnsTree**)malloc(sizeof(PnsTree*));
        t->child[i]->parent[0] = t;
      }
    } else {                                                            // No legal moves
      t->child = NULL;
      t->move = NULL;
      pnsSetScoreNoMoves(t, b);
    }
  } else if (score == EGTB_DRAW) {                                      // EGTB draw
    t->proof = INFTY64;
    t->disproof = INFTY64;
  } else if (score > 0) {                                               // EGTB win
    t->proof = 0;
    t->disproof = INFTY64;
  } else {                                                              // EGTB loss
    t->proof = INFTY64;
    t->disproof = 0;
  }
}

/* Propagate this node's values to each of its parents. */
void pnsUpdate(PnsTree *t) {
  if (t) {
    t->proof = INFTY64;
    t->disproof = 0;
    for (int i = 0; i < t->numChildren; i++) {
      t->proof = MIN(t->proof, t->child[i]->disproof);
      t->disproof += t->child[i]->proof;
    }
    t->disproof = MIN(t->disproof, INFTY64);
    for (int i = 0; i < t->numParents; i++) {
      pnsUpdate(t->parent[i]);
    }
  }
}

PnsTree* pnsAnalyzeBoard(Board *b, int maxNodes) {
  PnsTree *root = pnsMakeLeaf();
  Board current;
  int treeSize = 1;
  while ((treeSize < maxNodes) && root->proof && root->disproof &&
         (root->proof < INFTY64 || root->disproof < INFTY64)) {
    memcpy(&current, b, sizeof(Board));
    PnsTree *mpn = pnsSelectMpn(root, &current);
    pnsExpand(mpn, &current);
    for (int i = 0; i < mpn->numParents; i++) {
      pnsUpdate(mpn->parent[i]);
    }
    treeSize += mpn->numChildren;
  }
  printf("Tree size %d, proof %llu, disproof %llu\n", treeSize, root->proof, root->disproof);
  return root;
}

/* Encodes a move on 16 bits for saving. */
void pnsEncondeMove(Move m, FILE *f) {
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
  if (!parent || (parent == t->parent[0])) { // do nothing for transpositions
    fputc(t->numChildren, f);
    if (t->numChildren) {
      for (int i = 0; i < t->numChildren; i++) {
        // Technically, we don't need to save the moves; we know the order in which getAllMoves() generates them.
        // But we save them anyway, in case we ever change the move ordering.
        pnsEncondeMove(t->move[i], f);
        pnsSaveNode(t->child[i], t, f);
      }
    } else {
      pnsEncodeNumber(t->proof, f);
      pnsEncodeNumber(t->disproof, f);
    }
  }
}

PnsTree* pnsLoadNode(FILE *f, PnsTree *parent, Board *b) {
  PnsTree *t = (PnsTree*)malloc(sizeof(PnsTree));
  t->numParents = 1;
  t->parent = (PnsTree**)malloc(sizeof(PnsTree*));
  t->parent[0] = parent;

  t->numChildren = fgetc(f);
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
      makeMove(&b2, m[i]);
      t->child[i] = pnsLoadNode(f, t, &b2);
    }
  } else {
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
    PnsTree *t = pnsLoadNode(f, NULL, &b2);
    fclose(f);
    return t;
  } else if (errno == ENOENT) {
    // File does not exist
    return pnsMakeLeaf();
  } else {
    die("Input file [%s] exists, but cannot be read.", fileName.c_str());
    return NULL; // unreachable, but it suppresses a warning
  }
}

void pnsAnalyzeString(string input, string fileName) {
  Board *b;
  if (isFen(input)) {
    // Input is a board in FEN notation
    b = fenToBoard(NEW_BOARD);
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
  pnsLoadTree(b, fileName);
  PnsTree *t = pnsAnalyzeBoard(b, /*PNS_STEP_SIZE*/ 1000);
  pnsSaveTree(t, b, fileName);
  free(b);
  free(t);
}
