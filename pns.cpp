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

void pnsAnalyzeString(char *input) {
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
  if (b) {
    pnsAnalyzeBoard(b, 1000000);
    free(b);
  } else {
    log(LOG_ERROR, "Invalid input for PNS analysis");
  }
}
