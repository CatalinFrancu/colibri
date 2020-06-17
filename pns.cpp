#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include "board.h"
#include "egtb.h"
#include "fileutil.h"
#include "logging.h"
#include "movegen.h"
#include "pns.h"
#include "stringutil.h"
#include "zobrist.h"

Pns::Pns(int nodes, int edges, Pns* pn1) {
  this->pn1 = pn1;
  trim = (pn1 != NULL); // trim non-winning nodes in PN2, but not in PN1

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

bool Pns::isSolved(int t) {
  return (node[t].proof == 0 || node[t].proof == INFTY64) &&
    (node[t].disproof == 0 || node[t].disproof == INFTY64);
}

string Pns::pnAsString(u64 number) {
  if (number == INFTY64) {
    return "∞";
  }
  return to_string(number);
}

int Pns::allocateLeaf(u64 p, u64 d) {
  int t = nodeAllocator->alloc();
  node[t].proof = p;
  node[t].disproof = d;
  node[t].zobrist = 0;
  node[t].child = node[t].parent = NIL;
  node[t].depth = INFTY;
  return t;
}

void Pns::addParent(int childIndex, int parentIndex) {
  int e = edgeAllocator->alloc();
  edge[e].node = parentIndex;
  edge[e].next = node[childIndex].parent;
  node[childIndex].parent = e;
}

void Pns::prependChild(int parentIndex, int childIndex, Move m) {
  int e = edgeAllocator->alloc();
  edge[e].node = childIndex;
  edge[e].move = m;
  edge[e].next = node[parentIndex].child;
  node[parentIndex].child = e;
  floatRight(e);
}

int Pns::appendChild(int parentIndex, int childIndex, Move m, int tail) {
  int e = edgeAllocator->alloc();
  edge[e].node = childIndex;
  edge[e].move = m;
  edge[e].next = NIL;
  if (tail == NIL) {
    node[parentIndex].child = e;
  } else {
    edge[tail].next = e;
  }
  return e;
}

void Pns::printTree(int t, int level, int maxDepth) {
  if (level > maxDepth) {
    return;
  }

  string s(4 * level, ' ');
  for (int e = node[t].child; e != NIL; e = edge[e].next) {
    Move m = edge[e].move;
    int c = edge[e].node;

    stringstream ss;
    ss << s << getLongMoveName(m) << " -> "
       << pnAsString(node[c].proof) << '/' << pnAsString(node[c].disproof);

    log(LOG_DEBUG, ss.str().c_str());
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

void Pns::updateDepth(int t, int d) {
  if (d < node[t].depth) {
    node[t].depth = d;
    for (int e = node[t].child; e != NIL; e = edge[e].next) {
      updateDepth(edge[e].node, d + 1);
    }
  }
}

int Pns::selectMpn(Board *b) {
  int t = 0; // start at the root
  string s;
  while (node[t].child != NIL) {

    int e = node[t].child; // keep selecting the first child

    if (pn1) {
      s += ' ' + getMoveName(b, edge[e].move);
    }
    makeMove(b, edge[e].move);
    t = edge[e].node;
  }

  if (pn1) {
    log(LOG_INFO, "Root score %llu/%llu, size %d, expanding MPN (%llu/%llu)%s",
        node[0].proof, node[0].disproof, nodeAllocator->used(),
        node[t].proof, node[t].disproof, s.c_str());
  }
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

int Pns::zobristLookup(u64 z, int depth, u64 proof, u64 disproof) {
  auto it = trans.find(z);

  if (it == trans.end()) {
    // first time generating this position
    int c = allocateLeaf(proof, disproof);
    node[c].zobrist = z;
    trans[z] = { c, NIL };
    return c;
  }

  int orig = it->second.first;
  int rep = it->second.second;

  if (node[orig].depth >= depth) {
    return orig;
  }

  // Don't transpose to a node higher up the dag. This effectively prevents
  // repetitions, but also discourages long convoluted paths.
  if (rep == NIL) {
    rep = allocateLeaf(INFTY64, INFTY64);
    it->second.second = rep;
  }
  return rep;
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
    u64 z2 = updateZobrist(z, b, move[nc]);
    u64 childP = pn1 ? proof[nc]: 1;
    u64 childD = pn1 ? disproof[nc]: 1;
    int c = zobristLookup(z2, node[t].depth + 1, childP, childD);
    addParent(c, t);
    prependChild(t, c, move[nc]);
    updateDepth(c, node[t].depth + 1);
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
  bool wasSolved = isSolved(t);
  if (node[t].child != NIL) {
    // Here we used to assert that, with trimming enabled, t shouldn't be
    // called again if t is won. That is, assert(!trim || (origP >
    // 0)). However, this does happen in practice when a descendant d is
    // solved. Sometimes d and d's drawn clone both call the same ancestor.

    // If t has no children after expand(), then it's a stalemate or EGTB
    // position, so it already has correct P/D values.
    if (c != INFTY) {
      reorder(t, c);
    }
    u64 p = INFTY64, d = 0;
    for (int e = node[t].child; e != NIL; e = edge[e].next) {
      int c = edge[e].node;
      p = MIN(p, node[c].disproof);
      d = MIN(d + node[c].proof, INFTY64);
    }
    if (origP != p || origD != d) {
      node[t].proof = p;
      node[t].disproof = d;
    } else {
      changed = false;
    }
    trimNonWinning(t);
  }

  if (!wasSolved && isSolved(t)) {
    solveRepetition(t);
  }
  if (changed) {
    for (int e = node[t].parent; e != NIL; e = edge[e].next) {
      update(edge[e].node, t);
    }
  }
}

void Pns::trimNonWinning(int t) {
  if (!trim ||                          // trimming not enabled
      (node[t].proof > 0) ||            // no need to trim -- not won
      (node[t].child == NIL)) {         // nothing to trim
    return;
  }
  // node is won; delete all children except the first one
  seen.insert(t);
  int e = edge[node[t].child].next; // start at 2nd child
  edge[node[t].child].next = NIL;
  while (e != NIL) {
    int f = edge[e].next;
    deleteParentLink(edge[e].node, t);
    edgeAllocator->free(e);
    e = f;
  }
}

void Pns::deleteParentLink(int c, int p) {
  int e = node[c].parent;
  if (edge[e].node == p) {

    // p is the first child
    node[c].parent = edge[e].next;
    edgeAllocator->free(e);

    if (node[c].parent == NIL) {
      deleteNode(c);
    }

  } else {

    // find p's predecessor
    while (edge[edge[e].next].node != p) {
      e = edge[e].next;
    }

    int f = edge[edge[e].next].next;
    edgeAllocator->free(edge[e].next);
    edge[e].next = f;

  }
}

void Pns::deleteNode(int t) {
  if (!seen.insert(t).second) {
    return;   // t was already visited during this trim
  }
  // notify t's children to sever their links to t; delete t's edge list
  int e = node[t].child;

  // this also covers the case when the zobrist value is 0 (for repetitions)
  trans.erase(node[t].zobrist);

  nodeAllocator->free(t);
  while (e != NIL) {
    int f = edge[e].next;
    deleteParentLink(edge[e].node, t);
    edgeAllocator->free(e);
    e = f;
  }
}

void Pns::solveRepetition(int t) {
  if (node[t].zobrist == 0) {
    return; // t itself is a repetition
  }

  pair<int,int> p = trans[node[t].zobrist];
  assert(p.first == t);

  int rep = p.second;
  if (rep != NIL) {
    node[rep].proof = node[t].proof;
    node[rep].disproof = node[t].disproof;
    update(rep, NIL);
  }
}

void Pns::analyzeBoard(Board *b) {
  reset();
  allocateLeaf(1, 1);
  node[0].depth = 0;
  trans[getZobrist(b)] = { 0, NIL };
  bool full = false;
  while (!full &&
         node[0].proof && node[0].disproof &&
         (node[0].proof < INFTY64 || node[0].disproof < INFTY64)) {
    Board current = *b;
    int mpn = selectMpn(&current);
    assert(node[mpn].proof > 0);
    if (expand(mpn, &current)) {
      seen.clear();
      update(mpn, INFTY);
    } else {
      full = true;
    }
  }
  // verifyConsistencyWrapper(b);
  if (!pn1) {
    log(LOG_INFO, "PN1 complete, score %s/%s, %d nodes, %d edges, %d EGTB probes",
        pnAsString(node[0].proof).c_str(), pnAsString(node[0].disproof).c_str(),
        nodeAllocator->used(), edgeAllocator->used(), numEgtbLookups);
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
  load(b, fileName);
  printTree(0, 0, 100);
  verifyConsistencyWrapper(b);
  getchar();
  analyzeBoard(b);
  log(LOG_DEBUG, "saving");
  printTree(0, 0, 100);
  save(b, fileName);
  free(b);
}

void Pns::saveHelper(int t, FILE* f, unordered_map<int,int>* map, int* nextAvailable) {
  log(LOG_INFO, "saving node %d", t);
  byte numChildren = 0;
  for (int e = node[t].child; e != NIL; e = edge[e].next) {
    numChildren++;
  }

  // Emit the number of children and the depth.
  fwrite(&numChildren, 1, 1, f);
  writeVlq(node[t].depth, f);
  log(LOG_INFO, "saved numChildren %d depth %d", numChildren, node[t].depth);

  // For leaves, emit the encoded proof / disproof numbers. Since ∞ is a
  // frequent value and would take 9 bytes in 7-bit VLQ encoding, rename it to
  // 0, pushing all other value upwards
  if (!numChildren) {
    u64 p = (node[t].proof == INFTY64) ? 0 : (node[t].proof + 1);
    u64 d = (node[t].disproof == INFTY64) ? 0 : (node[t].disproof + 1);
    writeVlq(p, f);
    writeVlq(d, f);
    log(LOG_INFO, "saved p=%llu, d=%llu", p, d);
  }

  for (int e = node[t].child; e != NIL; e = edge[e].next) {
    // Emit the encoded move.
    u16 x = encodeMove(edge[e].move);
    fwrite(&x, 2, 1, f);
    log(LOG_INFO, "saved move %s encoded as %x", getLongMoveName(edge[e].move).c_str(), x);

    // If the child is new, give it a number and call it. Otherwise emit its number.
    int c = edge[e].node;
    auto it = map->find(c);
    if (it == map->end()) {
      writeVlq(*nextAvailable, f);
      log(LOG_INFO, "saved new child pointer %d", *nextAvailable);
      (*map)[c] = (*nextAvailable)++;
      log(LOG_INFO, "mapped node %d to %d", c, (*map)[c]);
      saveHelper(c, f, map, nextAvailable);
    } else {
      writeVlq(it->second, f);
      log(LOG_INFO, "saved old child pointer %d", it->second);
    }
  }
}

void Pns::save(Board *b, string fileName) {
  FILE *f = fopen(fileName.c_str(), "w");
  fwrite(b, sizeof(Board), 1, f);

  // renumber nodes sequentially during the traversal
  unordered_map<int,int> map;
  map[0] = 0;
  int nextAvailable = 1;
  saveHelper(0, f, &map, &nextAvailable);
  fclose(f);
}

void Pns::loadHelper(FILE* f) {
  int t = allocateLeaf(INFTY64, 0);

  // Read the number of children and the depth.
  byte numChildren;
  fread(&numChildren, 1, 1, f);
  node[t].depth = readVlq(f);
  log(LOG_DEBUG, "On node %d read numChildren %d and depth %d", t, numChildren, node[t].depth);

  // For leaves, read and decode the proof / disproof numbers.
  if (!numChildren) {
    node[t].proof = readVlq(f);
    node[t].proof = (node[t].proof == 0) ? INFTY64 : (node[t].proof - 1);
    node[t].disproof = readVlq(f);
    node[t].disproof = (node[t].disproof == 0) ? INFTY64 : (node[t].disproof - 1);
  }
  int tail = NIL;
  while (numChildren--) {
    u16 encodedMove;
    fread(&encodedMove, 2, 1, f);
    Move m = decodeMove(encodedMove);
    int c = readVlq(f);
    log(LOG_DEBUG, "On node %d read child %d and move %s", t, c, getLongMoveName(m).c_str());
    assert(c <= nodeAllocator->used());
    if (c == nodeAllocator->used()) {
      loadHelper(f); // this will load node c
    }

    tail = appendChild(t, c, m, tail);
    addParent(c, t);

    node[t].proof = MIN(node[t].proof, node[c].disproof);
    node[t].disproof = MIN(node[t].disproof + node[c].proof, INFTY64);
  }
}

void Pns::load(Board *b, string fileName) {
  FILE *f = fopen(fileName.c_str(), "r");

  if (!f) {
    die("Cannot read input file [%s].", fileName.c_str());
  }

  Board b2;
  assert(fread(&b2, sizeof(Board), 1, f) == 1);
  if (!equalBoard(b, &b2)) {
    printBoard(&b2);
    die("Input file stores a PN^2 tree for a different board (see above).");
  }

  loadHelper(f);

  fclose(f);
  log(LOG_INFO, "Loaded tree from %s.", fileName.c_str());
}

string Pns::batchLookup(Board *b, string *moveNames, string *fens, string *scores, int *numMoves) {
  *numMoves = 0;

  u64 z = getZobrist(b);
  auto it = trans.find(z);
  if (it == trans.end()) {
    return "unknown";
  }
  int t = it->second.first;
  log(LOG_DEBUG, "query for nodes #%d & #%d, depth %d", t, it->second.second, node[t].depth);

  // Get the names of all legal moves on b. This may not be equal to the
  // number of t's children, which may have beeen trimmed.
  Move m[MAX_MOVES];
  string names[MAX_MOVES];
  int n = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, n, names);

  for (int e = node[t].child; e != NIL; e = edge[e].next) {
    // look up this child's move
    int i = 0;
    while (!equalMove(m[i], edge[e].move)) {
      i++;
    }

    moveNames[*numMoves] = names[i];

    int c = edge[e].node;
    Board b2 = *b;
    makeMove(&b2, edge[e].move);
    fens[*numMoves] = boardToFen(&b2);

    stringstream ss;
    ss << pnAsString(node[c].proof) << '/' << pnAsString(node[c].disproof);
    scores[*numMoves] = ss.str();
    log(LOG_DEBUG, "  returning child #%d move %s score %s depth %d",
        c, names[i].c_str(), ss.str().c_str(), node[c].depth);
    (*numMoves)++;
  }

  stringstream ss;
  ss << pnAsString(node[t].proof) << '/' << pnAsString(node[t].disproof);
  return ss.str();
}

void Pns::verifyConsistency(int t, Board *b, unordered_set<int>* seenNodes,
                            unordered_set<int>* seenEdges) {
  if (!seenNodes->insert(t).second) {
    return; // already visited
  }
  if (node[t].child == NIL) {
    return;
  }
  assert(nodeAllocator->isInUse(t));

  // check that all edge pointers are globally distinct
  for (int e = node[t].parent; e != NIL; e = edge[e].next) {
    if (!seenEdges->insert(e).second) {
      die("Edge %d appears twice", e);
    }
  }
  for (int e = node[t].child; e != NIL; e = edge[e].next) {
    if (!seenEdges->insert(e).second) {
      die("Edge %d appears twice", e);
    }
  }

  // check that our proof is our first child's disproof
  assert(node[t].proof == node[edge[node[t].child].node].disproof);

  if (node[t].disproof == 0) {
    // all our parents should be winning
    for (int e = node[t].parent; e != NIL; e = edge[e].next) {
      assert(node[edge[e].node].proof == 0);
    }
  }

  // verify the move list
  Move m[MAX_MOVES];
  int nc = getAllMoves(b, m, FORWARD);
  Board bc;

  for (int e = node[t].child; e != NIL; e = edge[e].next) {
    int c = edge[e].node, i = 0;

    // find this move in the legal move list and delete it
    m[nc] = edge[e].move;
    while (!equalMove(edge[e].move, m[i])) {
      i++;
    }
    if (i == nc) {
      printBoard(b);
      log(LOG_WARNING, "Illegal move in child list: %s, stored in edge #%d between nodes %d->%d",
          getLongMoveName(edge[e].move).c_str(), e, t, c);
      assert(false);
    }
    m[i] = m[--nc];

    // check that the child links back to us
    int f = node[c].parent;
    while ((f != NIL) && (edge[f].node != t)) {
      f = edge[f].next;
    }
    assert(f != NIL);

    // check that this child is better than the next one.
    if (edge[e].next != NIL) {
      assert(nodeCmp(c, edge[edge[e].next].node) <= 0);
    }

    bc = *b;
    makeMove(&bc, edge[e].move);
    verifyConsistency(c, &bc, seenNodes, seenEdges);
  }

  if (nc) {
    // some moves have been deleted: node should be won and have one losing child
    assert(node[t].child != NIL);
    assert(edge[node[t].child].next == NIL);
    int c = edge[node[t].child].node;

    if (node[t].proof) {
      printBoard(b);
      log(LOG_WARNING, "remaining moves:");
      for (int i = 0; i < nc; i++) {
        printf("%s\n", getMoveName(b, m[i]).c_str());
      }
    }
    assert(node[t].proof == 0);
    assert(node[t].disproof == INFTY64);
    assert(node[c].proof == INFTY64);
    assert(node[c].disproof == 0);
  } else {
    // With trimming enabled, if all moves have a match, than either there is
    // only one move or no moves are winning.
    if (trim) {
      assert((edge[node[t].child].next == NIL) ||
             (node[t].proof > 0));
    }
  }
}

void Pns::verifyConsistencyWrapper(Board* b) {
  unordered_set<int> seenNodes;
  unordered_set<int> seenEdges;
  verifyConsistency(0, b, &seenNodes, &seenEdges);
}
