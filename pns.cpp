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

/* Statically allocated memory for the PN^2 book */
PnsNode pnsNode[PNS_BOOK_SIZE];
int pnsNodeSize;

/* Statically allocated memory for PnsNode move lists */
Move pnsMove[PNS_MOVE_SIZE];
int pnsMoveSize;

/* Statically allocated memory for PnsNode child lists */
int pnsChild[PNS_CHILD_SIZE];
int pnsChildSize;

/* Statically allocated memory for PnsNode parent pointers */
PnsNodeList pnsParent[PNS_BOOK_SIZE];
int pnsParentSize;

void pnsInit() {
  pnsNodeSize = pnsMoveSize = pnsChildSize = pnsParentSize = 1;
}

int pnsMakeLeaf() {
  assert(pnsNodeSize < PNS_BOOK_SIZE);
  pnsNode[pnsNodeSize].proof = 1;
  pnsNode[pnsNodeSize].disproof = 1;
  pnsNode[pnsNodeSize].numChildren = 0;
  pnsNode[pnsNodeSize].parent = NIL;
  pnsNode[pnsNodeSize].extra = 0;
  pnsNodeSize++;
  return pnsNodeSize - 1;
}

/* Allocates moves in the statically allocated memory */
int pnsAllocateMoves(int n) {
  pnsMoveSize += n;
  assert(pnsMoveSize < PNS_MOVE_SIZE);
  return pnsMoveSize - n;
}

/* Allocates moves and computes the move list */
int pnsGetMoves(Board *b, int t) {
  int n = getAllMoves(b, pnsMove + pnsMoveSize, FORWARD);
  pnsNode[t].numChildren = n;
  pnsNode[t].move = pnsMoveSize;
  pnsAllocateMoves(n);
  return n;
}

/* Creates child pointers in the statically allocated memory */
int pnsAllocateChildren(int n) {
  pnsChildSize += n;
  assert(pnsChildSize < PNS_CHILD_SIZE);
  return pnsChildSize - n;
}

/* Adds a parent to a PNS node (presumably because we found a transposed path to it).
 * The new pointer is added at the front of the parent list. */
void pnsAddParent(int childIndex, int parentIndex) {
  if (parentIndex != NIL) {
    assert(pnsParentSize < PNS_PARENT_SIZE);
    pnsParent[pnsParentSize].node = parentIndex;
    pnsParent[pnsParentSize].next = pnsNode[childIndex].parent;
    pnsNode[childIndex].parent = pnsParentSize;
    pnsParentSize++;
  }
}

void pnsPrintTree(int t, int level) {
  for (int i = 0; i < pnsNode[t].numChildren; i++) {
    for (int j = 0; j < level; j++) {
      printf("    ");
    }
    Move m = pnsMove[pnsNode[t].move + i];
    int c = pnsChild[pnsNode[t].child + i];
    string from = SQUARE_NAME(m.from);
    string to = SQUARE_NAME(m.to);
    printf("%s%s %llu/%llu\n", from.c_str(), to.c_str(), pnsNode[c].proof, pnsNode[c].disproof);
    pnsPrintTree(c, level + 1);
  }
}

/* Clears the extra information field */
void pnsClearExtra() {
  for (int i = 0; i < pnsNodeSize; i++) {
    pnsNode[i].extra = 0;
  }
}

/* Adds a node's ancestors to the ancestor hash map. */
void pnsAddAncestors(int t) {
  bool insertSuccess = pnsAncestors.insert(t).second;
  if (insertSuccess) { // new element
    int p = pnsNode[t].parent;
    while (p != NIL) {
      pnsAddAncestors(pnsParent[p].node);
      p = pnsParent[p].next;
    }
  }
}

/* Finds the most proving node in a PNS tree. Starting with the original board b, also makes the necessary moves
 * modifying b, returning the position corresponding to the MPN. */
int pnsSelectMpn(Board *b) {
  int t = 1; // Start at the root
  while (pnsNode[t].numChildren) {
    int i = 0, c = pnsNode[t].child;
    while ((i < pnsNode[t].numChildren) && (pnsNode[pnsChild[c]].disproof != pnsNode[t].proof)) {
      i++;
      c++;
    }
    assert(i < pnsNode[t].numChildren);
    makeMove(b, pnsMove[pnsNode[t].move + i]);
    t = pnsChild[c];
  }

  pnsAncestors.clear();
  pnsAddAncestors(t);
  return t;
}

/* Sets the proof and disproof numbers for a board with no legal moves */
void pnsSetScoreNoMoves(int t, Board *b) {
  int indexMe = (b->side == WHITE) ? BB_WALL : BB_BALL;
  int indexOther = BB_WALL + BB_BALL - indexMe;
  int countMe = popCount(b->bb[indexMe]);
  int countOther = popCount(b->bb[indexOther]);
  pnsNode[t].proof = (countMe < countOther) ? 0 : INFTY64;
  pnsNode[t].disproof = (countMe > countOther) ? 0 : INFTY64;
}

/* Sets the proof and disproof numbers for an EGTB board */
void pnsSetScoreEgtb(int t, int score) {
  if (score == EGTB_DRAW) { // EGTB draw
    pnsNode[t].proof = INFTY64;
    pnsNode[t].disproof = INFTY64;
  } else if (score > 0) {   // EGTB win
    pnsNode[t].proof = 0;
    pnsNode[t].disproof = INFTY64;
  } else {                  // EGTB loss
    pnsNode[t].proof = INFTY64;
    pnsNode[t].disproof = 0;
  }
}

void pnsExpand(int t, Board *b) {
  int score = evalBoard(b);
  if (score != EGTB_UNKNOWN) {
    pnsSetScoreEgtb(t, score);                  // EGTB node
  } else {
    int nc = pnsGetMoves(b, t);
    if (!nc) {                      // No legal moves
      pnsSetScoreNoMoves(t, b);
    } else {                                    // Regular node
      pnsNode[t].child = pnsAllocateChildren(nc);
      u64 z = getZobrist(b);
      for (int i = 0; i < nc; i++) {
        int c = pnsNode[t].child + i;
        u64 z2 = updateZobrist(z, b, pnsMove[pnsNode[t].move + i]);
        int orig = trans[z2];
        if (!orig) {                            // Regular child
          pnsChild[c] = pnsMakeLeaf();
          trans[z2] = pnsChild[c];
        } else if (pnsAncestors.find(orig) == pnsAncestors.end()) { // Transposition
          pnsChild[c] = orig;
        } else {                                // Repetition
          pnsChild[c] = pnsMakeLeaf();
          pnsNode[pnsChild[c]].proof = pnsNode[pnsChild[c]].disproof = INFTY64;
        }
        pnsAddParent(pnsChild[c], t);
      }
    }
  }
}

/* Propagate this node's values to each of its parents. */
void pnsUpdate(int t) {
  if (pnsNode[t].numChildren) {
    // If t has no children, then presumably it's a stalemate or EGTB position, so it already has correct P/D values.
    u64 p = INFTY64, d = 0;
    for (int i = 0; i < pnsNode[t].numChildren; i++) {
      p = MIN(p, pnsNode[pnsChild[pnsNode[t].child + i]].disproof);
      d += pnsNode[pnsChild[pnsNode[t].child + i]].proof;
    }
    pnsNode[t].proof = p;
    pnsNode[t].disproof = MIN(d, INFTY64);
  }
  int u = pnsNode[t].parent;
  while (u != NIL) {
    pnsUpdate(pnsParent[u].node);
    u = pnsParent[u].next;
  }
}

/* Recalculates the P/D numbers for a freshly loaded tree (since we only store these numbers for leaves).
 * TODO: avoid duplicating effort for transpositions. */
void pnsRecalculateNumbers(int t) {
  if (pnsNode[t].numChildren) {
    u64 p = INFTY64, d = 0;
    for (int i = 0; i < pnsNode[t].numChildren; i++) {
      int c = pnsChild[pnsNode[t].child + i];
      pnsRecalculateNumbers(c);
      p = MIN(p, pnsNode[c].disproof);
      d += pnsNode[c].proof;
    }
    pnsNode[t].proof = p;
    pnsNode[t].disproof = MIN(d, INFTY64);
  }
}

void pnsAnalyzeBoard(Board *b, int maxNodes) {
  int treeSize = 1;
  while ((!maxNodes || (treeSize < maxNodes)) &&
         pnsNode[1].proof && pnsNode[1].disproof &&
         (pnsNode[1].proof < INFTY64 || pnsNode[1].disproof < INFTY64)) {
    Board current = *b;
    int mpn = pnsSelectMpn(&current);
    pnsExpand(mpn, &current);
    pnsUpdate(mpn);
    treeSize += pnsNode[mpn].numChildren;
  }
  printf("Tree size %d, proof %llu, disproof %llu\n", treeSize, pnsNode[1].proof, pnsNode[1].disproof);
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
    m.promotion = 0;
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

void pnsSaveNode(int t, int parent, FILE *f) {
  if (pnsNode[t].extra) {
    // We are a transposition -- write 0xff for numChildren
    fputc(0xff, f);
  } else {
    pnsNode[t].extra = 1;
    fputc(pnsNode[t].numChildren, f);
    if (pnsNode[t].numChildren) {
      // Technically, we don't need to save the moves; we know the order in which getAllMoves() generates them.
      // But we save them anyway, in case we ever change the move ordering.
      for (int i = 0; i < pnsNode[t].numChildren; i++) {
        pnsEncodeMove(pnsMove[pnsNode[t].move + i], f);
      }
      for (int i = 0; i < pnsNode[t].numChildren; i++) {
        pnsSaveNode(pnsChild[pnsNode[t].child + i], t, f);
      }
    } else {
      pnsEncodeNumber(pnsNode[t].proof, f);
      pnsEncodeNumber(pnsNode[t].disproof, f);
    }
  }
}

int pnsLoadNode(FILE *f, int parent, Board *b, u64 zobrist) {
  int numChildren = fgetc(f);
  if (numChildren == 0xff) {
    // If the node is a transposition, just add _parent_ to its parent list.
    int orig = trans[zobrist];
    assert(orig);
    pnsAddParent(orig, parent);
    return orig;
  }

  // Create a new node and add it to the transposition table
  int t = pnsMakeLeaf();
  pnsAddParent(t, parent);
  if (!trans[zobrist]) {
    // Repetition nodes appear multiple times in the tree -- only hash the first one.
    trans[zobrist] = t;
  }
  pnsNode[t].numChildren = numChildren;

  // For internal nodes, update the board and the Zobrist key and recurse
  if (numChildren) {
    pnsNode[t].child = pnsAllocateChildren(numChildren);
    pnsNode[t].move = pnsAllocateMoves(numChildren);
    for (int i = 0; i < numChildren; i++) {
      pnsMove[pnsNode[t].move + i] = pnsDecodeMove(f);
    }
#ifdef ENABLE_SLOW_ASSERTS
    Move m[MAX_MOVES];
    int numMoves = getAllMoves(b, m, FORWARD);
    assert(numMoves == numChildren);
    for (int i = 0; i < numChildren; i++) {
      assert(equalMove(m[i], pnsMove[pnsNode[t].move + i]));
    }
#endif // ENABLE_SLOW_ASSERTS
    for (int i = 0, m = pnsNode[t].move; i < numChildren; i++, m++) {
      Board b2 = *b;
      u64 zobrist2 = updateZobrist(zobrist, &b2, pnsMove[m]);
      makeMove(&b2, pnsMove[m]);
      pnsChild[pnsNode[t].child + i] = pnsLoadNode(f, t, &b2, zobrist2);
    }
  } else {
    // For leaves, just parse the proof / disproof numbers.
    pnsNode[t].proof = pnsDecodeNumber(f);
    pnsNode[t].disproof = pnsDecodeNumber(f);
  }
  return t;
}

void pnsSaveTree(Board *b, string fileName) {
  pnsClearExtra();
  FILE *f = fopen(fileName.c_str(), "w");
  fwrite(b, sizeof(Board), 1, f);
  pnsSaveNode(1, NIL, f);
  fclose(f);
}

/* Loads a PN^2 tree from fileName and checks that it applies to b.
 * If fileName does not exist, then creates a 1-node tree. */
void pnsLoadTree(Board *b, string fileName) {
  FILE *f = fopen(fileName.c_str(), "r");
  if (f) {
    Board b2;
    assert(fread(&b2, sizeof(Board), 1, f) == 1);
    if (!equalBoard(b, &b2)) {
      printBoard(&b2);
      die("Input file stores a PN^2 tree for a different board (see above).");
    }
    log(LOG_INFO, "Calling pnsLoadNode()");
    pnsLoadNode(f, NIL, &b2, getZobrist(&b2));
    log(LOG_INFO, "Back from pnsLoadNode()");
    fclose(f);
    pnsRecalculateNumbers(1);
    log(LOG_INFO, "Loaded tree from %s.", fileName.c_str());

  } else if (errno == ENOENT) { // File does not exist
    log(LOG_INFO, "File %s does not exist, creating new tree.", fileName.c_str());
    int t = pnsMakeLeaf();
    trans[getZobrist(b)] = t;

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
  pnsLoadTree(b, fileName);
  pnsAnalyzeBoard(b, PNS_STEP_SIZE);
  printf("%d/%d %d/%d %d/%d %d/%d\n", pnsNodeSize, PNS_BOOK_SIZE, pnsMoveSize, PNS_MOVE_SIZE, pnsChildSize, PNS_CHILD_SIZE, pnsParentSize, PNS_PARENT_SIZE);
  pnsSaveTree(b, fileName);
  free(b);
}
