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
  node[nodeSize].extra = 0;
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

void Pns::clearExtra() {
  for (int i = 0; i < nodeSize; i++) {
    node[i].extra = 0;
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

void Pns::recalculateNumbers(int t) {
  if (node[t].numChildren) {
    u64 p = INFTY64, d = 0;
    for (int i = 0; i < node[t].numChildren; i++) {
      int c = child[node[t].child + i];
      recalculateNumbers(c);
      p = MIN(p, node[c].disproof);
      d += node[c].proof;
    }
    node[t].proof = p;
    node[t].disproof = MIN(d, INFTY64);
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
  log(LOG_INFO, "Tree size %d, proof %llu, disproof %llu\n", nodeSize, node[1].proof, node[1].disproof);
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
  printf("%d/%d %d/%d %d/%d %d/%d\n", nodeSize, nodeMax, moveSize, moveMax, childSize, childMax, parentSize, parentMax);
  saveTree(b, fileName);
  free(b);
}

void Pns::encodeMove(Move m, FILE *f) {
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

Move Pns::decodeMove(FILE *f) {
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
    m.promotion = 0;
  }
  return m;
}

void Pns::encodeNumber(u64 x, FILE *f) {
  if (x == INFTY64) {
    varintPut(0, f);
  } else {
    varintPut(x + 1, f);
  }
}

u64 Pns::decodeNumber(FILE *f) {
  u64 x = varintGet(f);
  return x ? (x - 1) : INFTY64;
}

void Pns::saveNode(int t, int parent, FILE *f) {
  if (node[t].extra) {
    // We are a transposition -- write 0xff for numChildren
    fputc(0xff, f);
  } else {
    node[t].extra = 1;
    fputc(node[t].numChildren, f);
    if (node[t].numChildren) {
      // Technically, we don't need to save the moves; we know the order in which getAllMoves() generates them.
      // But we save them anyway, in case we ever change the move ordering.
      for (int i = 0; i < node[t].numChildren; i++) {
        encodeMove(move[node[t].move + i], f);
      }
      for (int i = 0; i < node[t].numChildren; i++) {
        saveNode(child[node[t].child + i], t, f);
      }
    } else {
      encodeNumber(node[t].proof, f);
      encodeNumber(node[t].disproof, f);
    }
  }
}

int Pns::loadNode(FILE *f, int parent, Board *b, u64 zobrist) {
  int numChildren = fgetc(f);
  if (numChildren == 0xff) {
    // If the node is a transposition, just add _parent_ to its parent list.
    int orig = trans[zobrist];
    assert(orig);
    addParent(orig, parent);
    return orig;
  }

  // Create a new node and add it to the transposition table
  int t = allocateLeaf();
  addParent(t, parent);
  if (!trans[zobrist]) {
    // Repetition nodes appear multiple times in the tree -- only hash the first one.
    trans[zobrist] = t;
  }
  node[t].numChildren = numChildren;

  // For internal nodes, update the board and the Zobrist key and recurse
  if (numChildren) {
    node[t].child = allocateChildren(numChildren);
    node[t].move = allocateMoves(numChildren);
    for (int i = 0; i < numChildren; i++) {
      move[node[t].move + i] = decodeMove(f);
    }
#ifdef ENABLE_SLOW_ASSERTS
    Move m[MAX_MOVES];
    int numMoves = getAllMoves(b, m, FORWARD);
    assert(numMoves == numChildren);
    for (int i = 0; i < numChildren; i++) {
      assert(equalMove(m[i], move[node[t].move + i]));
    }
#endif // ENABLE_SLOW_ASSERTS
    for (int i = 0, m = node[t].move; i < numChildren; i++, m++) {
      Board b2 = *b;
      u64 zobrist2 = updateZobrist(zobrist, &b2, move[m]);
      makeMove(&b2, move[m]);
      child[node[t].child + i] = loadNode(f, t, &b2, zobrist2);
    }
  } else {
    // For leaves, just parse the proof / disproof numbers.
    node[t].proof = decodeNumber(f);
    node[t].disproof = decodeNumber(f);
  }
  return t;
}

void Pns::saveTree(Board *b, string fileName) {
  clearExtra();
  FILE *f = fopen(fileName.c_str(), "w");
  fwrite(b, sizeof(Board), 1, f);
  saveNode(1, NIL, f);
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
    log(LOG_INFO, "Calling pnsLoadNode()");
    loadNode(f, NIL, &b2, getZobrist(&b2));
    log(LOG_INFO, "Back from pnsLoadNode()");
    fclose(f);
    recalculateNumbers(1);
    log(LOG_INFO, "Loaded tree from %s.", fileName.c_str());

  } else if (errno == ENOENT) { // File does not exist
    log(LOG_INFO, "File %s does not exist, creating new tree.", fileName.c_str());
    int t = allocateLeaf();
    trans[getZobrist(b)] = t;

  } else { // File exists, but cannot be read for other reasons
    die("Input file [%s] exists, but cannot be read.", fileName.c_str());
  }
}
