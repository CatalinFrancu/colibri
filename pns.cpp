#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include "board.h"
#include "logging.h"
#include "movegen.h"
#include "pns.h"
#include "stringutil.h"
#include "zobrist.h"

Pns::Pns(int nodeMax, int moveMax, int childMax, int parentMax) {
  this->nodeMax = nodeMax;
  this->moveMax = moveMax;
  this->childMax = childMax;
  this->parentMax = parentMax;

  nodeSize = moveSize = childSize = parentSize = 1;

  this->node = (PnsNode*)malloc(nodeMax * sizeof(PnsNode));
  this->move = (Move*)malloc(moveMax * sizeof(Move));
  this->child = (int*)malloc(childMax * sizeof(int));
  this->parent = (PnsNodeList*)malloc(parentMax * sizeof(PnsNodeList));
}

int Pns::allocateLeaf() {
  assert(nodeSize < nodeMax);
  node[nodeSize].proof = 1;
  node[nodeSize].disproof = 1;
  node[nodeSize].numChildren = 0;
  node[nodeSize].parent = NIL;
  return nodeSize++;
}

int Pns::allocateMoves(int n) {
  moveSize += n;
  assert(moveSize <= moveMax);
  return moveSize - n;
}

int Pns::getMoves(Board *b, int t) {
  int n = getAllMoves(b, move + moveSize, FORWARD);
  node[t].numChildren = n;
  node[t].move = moveSize;
  allocateMoves(n);
  return n;
}

/* Creates child pointers in the statically allocated memory */
int Pns::allocateChildren(int n) {
  childSize += n;
  assert(childSize <= childMax);
  return childSize - n;
}

void Pns::addParent(int childIndex, int parentIndex) {
  if (parentIndex != NIL) {
    assert(parentSize < parentMax);
    parent[parentSize].node = parentIndex;
    parent[parentSize].next = node[childIndex].parent;
    node[childIndex].parent = parentSize++;
  }
}

void Pns::printTree(int t, int level) {
  for (int i = 0; i < node[t].numChildren; i++) {
    for (int j = 0; j < level; j++) {
      printf("    ");
    }
    Move m = move[node[t].move + i];
    int c = child[node[t].child + i];
    string from = SQUARE_NAME(m.from);
    string to = SQUARE_NAME(m.to);
    printf("%s%s %llu/%llu\n", from.c_str(), to.c_str(), node[c].proof, node[c].disproof);
    printTree(c, level + 1);
  }
}

void Pns::hashAncestors(int t) {
  bool insertSuccess = ancestors.insert(t).second;
  if (insertSuccess) { // new element
    int p = node[t].parent;
    while (p != NIL) {
      hashAncestors(parent[p].node);
      p = parent[p].next;
    }
  }
}

int Pns::selectMpn(Board *b) {
  int t = 1; // Start at the root
  while (node[t].numChildren) {
    int i = 0, c = node[t].child;
    while ((i < node[t].numChildren) && (node[child[c]].disproof != node[t].proof)) {
      i++;
      c++;
    }
    assert(i < node[t].numChildren);
    makeMove(b, move[node[t].move + i]);
    t = child[c];
  }

  ancestors.clear();
  hashAncestors(t);
  return t;
}

void Pns::setScoreNoMoves(int t, Board *b) {
  int indexMe = (b->side == WHITE) ? BB_WALL : BB_BALL;
  int indexOther = BB_WALL + BB_BALL - indexMe;
  int countMe = popCount(b->bb[indexMe]);
  int countOther = popCount(b->bb[indexOther]);
  node[t].proof = (countMe < countOther) ? 0 : INFTY64;
  node[t].disproof = (countMe > countOther) ? 0 : INFTY64;
}

void Pns::setScoreEgtb(int t, int score) {
  if (score == EGTB_DRAW) { // EGTB draw
    node[t].proof = INFTY64;
    node[t].disproof = INFTY64;
  } else if (score > 0) {   // EGTB win
    node[t].proof = 0;
    node[t].disproof = INFTY64;
  } else {                  // EGTB loss
    node[t].proof = INFTY64;
    node[t].disproof = 0;
  }
}

void Pns::expand(int t, Board *b) {
  int score = evalBoard(b);
  if (score != EGTB_UNKNOWN) {
    setScoreEgtb(t, score);                     // EGTB node
  } else {
    int nc = getMoves(b, t);
    if (!nc) {                                  // No legal moves
      setScoreNoMoves(t, b);
    } else {                                    // Regular node
      node[t].child = allocateChildren(nc);
      u64 z = getZobrist(b);
      for (int i = 0; i < nc; i++) {
        int c = node[t].child + i;
        u64 z2 = updateZobrist(z, b, move[node[t].move + i]);
        int orig = trans[z2];
        if (!orig) {                            // Regular child
          child[c] = allocateLeaf();
          trans[z2] = child[c];
        } else if (ancestors.find(orig) == ancestors.end()) { // Transposition
          child[c] = orig;
        } else {                                // Repetition
          child[c] = allocateLeaf();
          node[child[c]].proof = node[child[c]].disproof = INFTY64;
        }
        addParent(child[c], t);
      }
    }
  }
}

void Pns::update(int t) {
  if (node[t].numChildren) {
    // If t has no children, then presumably it's a stalemate or EGTB position, so it already has correct P/D values.
    u64 p = INFTY64, d = 0;
    for (int i = 0; i < node[t].numChildren; i++) {
      int c = child[node[t].child + i];
      p = MIN(p, node[c].disproof);
      d += node[c].proof;
    }
    node[t].proof = p;
    node[t].disproof = MIN(d, INFTY64);
  }
  int u = node[t].parent;
  while (u != NIL) {
    update(parent[u].node);
    u = parent[u].next;
  }
}

void Pns::analyzeBoard(Board *b) {
  int treeSize = 1;
  while (nodeSize < nodeMax - MAX_MOVES &&
         node[1].proof && node[1].disproof &&
         (node[1].proof < INFTY64 || node[1].disproof < INFTY64)) {
    Board current = *b;
    int mpn = selectMpn(&current);
    expand(mpn, &current);
    update(mpn);
  }
  log(LOG_INFO, "Tree size %d, proof %llu, disproof %llu", nodeSize, node[1].proof, node[1].disproof);
  log(LOG_INFO, "Usage: nodes %d/%d moves %d/%d children %d/%d parents %d/%d", nodeSize, nodeMax, moveSize, moveMax, childSize, childMax, parentSize, parentMax);
}

void Pns::analyzeString(string input, string fileName) {
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
  loadTree(b, fileName);
  analyzeBoard(b);
  saveTree(b, fileName);
  free(b);
}

void Pns::saveTree(Board *b, string fileName) {
  FILE *f = fopen(fileName.c_str(), "w");
  fwrite(b, sizeof(Board), 1, f);
  fwrite(&nodeSize, sizeof(nodeSize), 1, f);
  fwrite(&nodeMax, sizeof(nodeMax), 1, f);
  fwrite(node, sizeof(PnsNode), nodeSize, f);
  fwrite(&moveSize, sizeof(moveSize), 1, f);
  fwrite(&moveMax, sizeof(moveMax), 1, f);
  fwrite(move, sizeof(Move), moveSize, f);
  fwrite(&childSize, sizeof(childSize), 1, f);
  fwrite(&childMax, sizeof(childMax), 1, f);
  fwrite(child, sizeof(int), childSize, f);
  fwrite(&parentSize, sizeof(parentSize), 1, f);
  fwrite(&parentMax, sizeof(parentMax), 1, f);
  fwrite(parent, sizeof(PnsNodeList), parentSize, f);
  fclose(f);
}

void Pns::loadTree(Board *b, string fileName) {
  FILE *f = fopen(fileName.c_str(), "r");
  if (f) {
    Board b2;
    assert(fread(&b2, sizeof(Board), 1, f) == 1);
    if (!equalBoard(b, &b2)) {
      printBoard(&b2);
      die("Input file stores a PN^2 tree for a different board (see above).");
    }
    assert(fread(&nodeSize, sizeof(nodeSize), 1, f) == 1);
    assert(fread(&nodeMax, sizeof(nodeMax), 1, f) == 1);
    assert(fread(node, sizeof(PnsNode), nodeSize, f) == nodeSize);
    assert(fread(&moveSize, sizeof(moveSize), 1, f) == 1);
    assert(fread(&moveMax, sizeof(moveMax), 1, f) == 1);
    assert(fread(move, sizeof(Move), moveSize, f) == moveSize);
    assert(fread(&childSize, sizeof(childSize), 1, f) == 1);
    assert(fread(&childMax, sizeof(childMax), 1, f) == 1);
    assert(fread(child, sizeof(int), childSize, f) == childSize);
    assert(fread(&parentSize, sizeof(parentSize), 1, f) == 1);
    assert(fread(&parentMax, sizeof(parentMax), 1, f) == 1);
    assert(fread(parent, sizeof(PnsNodeList), parentSize, f) == parentSize);
    fclose(f);
    log(LOG_INFO, "Loaded tree from %s.", fileName.c_str());

  } else if (errno == ENOENT) { // File does not exist
    log(LOG_INFO, "File %s does not exist, creating new tree.", fileName.c_str());
    int t = allocateLeaf();
    trans[getZobrist(b)] = t;

  } else { // File exists, but cannot be read for other reasons
    die("Input file [%s] exists, but cannot be read.", fileName.c_str());
  }
}
