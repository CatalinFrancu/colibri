#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include "board.h"
#include "egtb.h"
#include "logging.h"
#include "movegen.h"
#include "pns.h"
#include "stringutil.h"
#include "zobrist.h"

Pns::Pns(int nodes, int edges, Pns* pn1) {
  this->pn1 = pn1;

  node = (PnsNode*)malloc(nodes * sizeof(PnsNode));
  nodeAllocator = new Allocator("node", nodes);
  edge = (PnsNodeList*)malloc(edges * sizeof(PnsNodeList));
  edgeAllocator = new Allocator("edge", edges);

  reset();
}

u64 Pns::getProof() {
  return node[0].proof;
}

u64 Pns::getDisproof() {
  return node[0].disproof;
}

void Pns::reset() {
  nodeAllocator->reset();
  edgeAllocator->reset();
  trans.clear();
  numEgtbLookups = 0;
}

int Pns::allocateLeaf() {
  int t = nodeAllocator->alloc();
  node[t].proof = node[t].disproof = 1;
  node[t].child = node[t].parent = NIL;
  return t;
}

void Pns::addParent(int childIndex, int parentIndex) {
  int e = edgeAllocator->alloc();
  edge[e].node = parentIndex;
  edge[e].next = node[childIndex].parent;
  node[childIndex].parent = e;
}

void Pns::printTree(int t, int level, int maxDepth) {
  if (level > maxDepth) {
    return;
  }

  for (int e = node[t].child; e != NIL; e = edge[e].next) {
    string s(4 * level, ' ');
    Move m = edge[e].move;
    int c = edge[e].node;
    string from = SQUARE_NAME(m.from);
    string to = SQUARE_NAME(m.to);
    log(LOG_DEBUG, "%s%s%s %llu/%llu",
        s.c_str(), from.c_str(), to.c_str(), node[c].proof, node[c].disproof);
    printTree(c, level + 1, maxDepth);
  }
}

int Pns::nodeCmp(int u, int v) {
  int s = sgn(node[u].disproof - node[v].disproof);
  if (s != 0) {
    return s;
  }
  return sgn(node[v].proof - node[u].proof);
}

void Pns::hashAncestors(int t) {
  bool insertSuccess = ancestors.insert(t).second;
  if (insertSuccess) { // new element
    for (int e = node[t].parent; e != NIL; e = edge[e].next) {
      hashAncestors(edge[e].node);
    }
  }
}

int Pns::selectMpn(Board *b) {
  int t = 0; // Start at the root
  string s;
  while (node[t].child != NIL) {
    int e = node[t].child; // first child
    if (pn1) {
      s += ' ' + getMoveName(b, edge[e].move);
    }
    makeMove(b, edge[e].move);
    t = edge[e].node;
  }

  if (pn1) {
    log(LOG_INFO, "Expanding MPN (%llu/%llu)%s", node[t].proof, node[t].disproof, s.c_str());
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
  // international rules: can win or draw, but never lose
  node[t].proof = (countMe < countOther) ? 0 : INFTY64;
  node[t].disproof = INFTY64;
}

void Pns::setScoreEgtb(int t, int score) {
  numEgtbLookups++;
  if (score == 0) { // EGTB draw
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

int Pns::copyMovesFromPn1() {
  int n = 0;
  for (int e = pn1->node[0].child; e != NIL; e = pn1->edge[e].next) {
    proof[n] = pn1->node[pn1->edge[e].node].proof;
    disproof[n] = pn1->node[pn1->edge[e].node].disproof;
    move[n++] = pn1->edge[e].move;
  }
  return n;
}

bool Pns::expand(int t, Board *b) {
  if (!pn1) {                     // no EGTB lookups in PN2
    int score = egtbLookup(b);
    if (score != EGTB_UNKNOWN) {
      setScoreEgtb(t, score);
      return true;
    }
  }

  int nc;
  if (pn1) {
    Board bc = *b;
    pn1->analyzeBoard(&bc);
    nc = copyMovesFromPn1();
  } else {
    nc = getAllMoves(b, move, FORWARD);
  }
  if (!nc) {                                  // no legal moves
    setScoreNoMoves(t, b);
    return true;
  }
  if (nodeAllocator->available() < nc ||
      edgeAllocator->available() < 2 * nc) {  // not enough room left to expand
    return false;
  }

  u64 z = getZobrist(b);
  // nodes are prepended, so copy moves from last to first
  while (nc--) {
    int e = edgeAllocator->alloc();

    // set the node values
    u64 z2 = updateZobrist(z, b, move[nc]);
    int orig = trans[z2], c;
    if (!orig) {                            // regular child
      c = allocateLeaf();
      trans[z2] = c;
      if (pn1) {
        node[c].proof = proof[nc];
        node[c].disproof = disproof[nc];
      }
    } else if (ancestors.find(orig) == ancestors.end()) { // transposition
      c = orig;
    } else {                                // repetition
      c = allocateLeaf();
      node[c].proof = node[c].disproof = INFTY64;
    }
    addParent(c, t);

    // set the edge values and prepend it
    edge[e].move = move[nc];
    edge[e].node = c;
    edge[e].next = node[t].child;
    node[t].child = e;
    floatRight(e);
  }

  return true;
}

void Pns::floatRight(int e) {
  int u = edge[e].node;
  while ((edge[e].next != NIL) &&
         nodeCmp(u, edge[edge[e].next].node) == 1) {
    int f = edge[e].next;
    Move mtmp = edge[e].move;
    edge[e].move = edge[f].move;
    edge[f].move = mtmp;

    edge[e].node = edge[f].node;
    edge[f].node = u;

    e = f;
  }
}

void Pns::reorder(int p, int c) {
  // 1. Find c. If c is better than its predecessor, move it to the beginning
  // of the list. If c is already the first child, do nothing.
  int e = node[p].child;
  if (edge[e].node != c) {
    while (edge[edge[e].next].node != c) {
      e = edge[e].next;
    }
    int f = edge[e].next;            // edge containing c
    if (nodeCmp(edge[e].node, c) == 1) {
      edge[e].next = edge[f].next;   // link edges before and after c
      edge[f].next = node[p].child;  // c points to the first child
      node[p].child = f;             // c is the first child
    }
    e = f;                           // e is the edge containing c
  }

  // 2. Now e is the edge containing c and all the nodes before c in the list
  // are better than c. Move c rightwards while necessary. We move c by
  // swapping payloads, not by changing pointers.
  floatRight(e);
}

void Pns::update(int t, int c) {
  u64 origP = node[t].proof, origD = node[t].disproof;
  bool changed = true;
  if (node[t].child != NIL) {
    // If t has no children after expand(), then it's a stalemate or EGTB
    // position, so it already has correct P/D values and sorted children.
    if (c != INFTY) {
      reorder(t, c);
    }
    u64 p = INFTY64, d = 0;
    for (int e = node[t].child; e != NIL; e = edge[e].next) {
      int c = edge[e].node;
      p = MIN(p, node[c].disproof);
      d += node[c].proof;
    }
    d = MIN(d, INFTY64);
    if (origP != p || origD != d) {
      node[t].proof = p;
      node[t].disproof = d;
    } else {
      changed = false;
    }
  }
  if (changed) {
    for (int e = node[t].parent; e != NIL; e = edge[e].next) {
      update(edge[e].node, t);
    }
  }
}

void Pns::analyzeBoard(Board *b) {
  reset();
  allocateLeaf();
  bool full = false;
  while (!full &&
         node[0].proof && node[0].disproof &&
         (node[0].proof < INFTY64 || node[0].disproof < INFTY64)) {
    if (pn1) {
      log(LOG_INFO, "Root score %llu/%llu", node[0].proof, node[0].disproof);
    }
    Board current = *b;
    int mpn = selectMpn(&current);
    if (expand(mpn, &current)) {
      update(mpn, INFTY);
    } else {
      full = true;
    }
  }
  if (!pn1) {
    log(LOG_INFO, "PN1 complete, score %llu/%llu, %d nodes, %d edges, %d EGTB probes",
        node[0].proof, node[0].disproof, nodeAllocator->used(), edgeAllocator->used(),
        numEgtbLookups);
    printTree(0, 0, 0);
  }
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
    // assert(fread(&moveSize, sizeof(moveSize), 1, f) == 1);
    // assert(fread(&moveMax, sizeof(moveMax), 1, f) == 1);
    // assert((int)fread(move, sizeof(Move), moveSize, f) == moveSize);
    // assert(fread(&childSize, sizeof(childSize), 1, f) == 1);
    // assert(fread(&childMax, sizeof(childMax), 1, f) == 1);
    // assert((int)fread(child, sizeof(int), childSize, f) == childSize);
    // assert(fread(&parentSize, sizeof(parentSize), 1, f) == 1);
    // assert(fread(&parentMax, sizeof(parentMax), 1, f) == 1);
    // assert((int)fread(parent, sizeof(PnsNodeList), parentSize, f) == parentSize);
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
