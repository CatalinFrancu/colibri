#ifndef PNS_H
#define PNS_H

#include "allocator.h"

/* A PN search tree node (it's really a DAG). Pointers are statically represented in a preallocated memory area. */
typedef struct {
  u64 proof;
  u64 disproof;
  int child, parent; // pointers to heads of child and parent lists
} PnsNode;

/**
 * A linked list of PnsNodes. For each node, store the move that takes us
 * there. The move field is used for child lists, but not for parent lists.
 */
typedef struct {
  Move move;
  int node;
  int next;
} PnsNodeList;

/*
 * Class that handles proof-number search.
 */
class Pns {
  Allocator* nodeAllocator;
  Allocator* edgeAllocator;

  // temporary space for move generation and for values copied from PN1
  Move move[MAX_MOVES];
  u64 proof[MAX_MOVES], disproof[MAX_MOVES];

  /* Hash table of positions anywhere in the PNS tree. Maps Zobrist keys to indices in node. */
  unordered_map<u64, int> trans;

  /* Set of nodes on any path from the MPN to the root. Stores indices in node. */
  unordered_set<int> ancestors;

  /* First level PN tree, if we are a second level PN tree. */
  Pns* pn1;

public:

  /* Preallocated arrays of nodes and edges. */
  PnsNode *node;
  PnsNodeList *edge;

  /* Creates a new Pns with the given size limits and one leaf. */
  Pns(int nodes, int edges, Pns* pn1);

  /* Constructs a P/N tree until the position is proved or memory is exhausted. */
  void analyzeBoard(Board *b);

  /* Analyze a string, which can be a sequence of moves or a position in FEN.
   * Loads a previous PNS tree from fileName, if it exists. */
  void analyzeString(string input, string fileName);

  /* Returns the root's proof number. */
  u64 getProof();

  /* Returns the root's disproof number. */
  u64 getDisproof();

  /* Clears all the information from the tree, retaining one unexplored leaf. */
  void reset();

private:

  /* Creates a PNS tree node with no children and no parents and proof/disproof values of 1.
   * Returns its index in the preallocated array. */
  int allocateLeaf();

  /* Adds a parent to a PNS node (presumably because we found a transposed path to it).
   * The new pointer is added at the front of the parent list. */
  void addParent(int childIndex, int parentIndex);

  /* Prints a PNS tree node recursively. */
  void printTree(int t, int level, int maxDepth);

  /* Saves the PNS tree. */
  void saveTree(Board *b, string fileName);

  /* Loads a PN^2 tree from fileName and checks that it applies to b.
   * If fileName does not exist, then creates a 1-node tree. */
  void loadTree(Board *b, string fileName);

  /**
   * Returns -1 if u is less promising than v, 0 if they are equal or 1 if u
   * is more promising than v.
   */
  int nodeCmp(int u, int v);

  /* Adds a node's ancestors to the ancestor hash map. */
  void hashAncestors(int t);

  /* Finds the most proving node in a PNS tree. Starting with the original board b, also makes the necessary moves
   * modifying b, returning the position corresponding to the MPN. */
  int selectMpn(Board *b);

  /* Sets the proof and disproof numbers for a board with no legal moves. */
  void setScoreNoMoves(int t, Board *b);

  /* Sets the proof and disproof numbers for an EGTB board. */
  void setScoreEgtb(int t, int score);

  /**
   * Copies the PN1 root's moves to our own move array.
   * @return The number of moves.
   */
  int copyMovesFromPn1();

  /**
   * Creates a new child list for t and populates it with the moves in move[]
   * @param t The node being expanded.
   * @param numMoves The number of moves in move[]
   */
  void copyMovesToChildList(int t, int numMoves);

  /* Expands the given leaf with 1/1 children. If there are no legal moves,
     sets the P/D numbers accordingly. If it runs out of tree space, returns
     false. */
  bool expand(int t, Board *b);

  /**
   * Floats a node rightwards in an edge list to its sorted position. Assumes
   * the rest of the list is sorted.
   */
  void floatRight(int e);

  /**
   * Finds the node c in the list of p's children. Moves c to its appropriate
   * position in order to keep p's children sorted by (dis)proof. Assumes c
   * appears in p's child list. Assumes that all other children except for c
   * are already sorted.
   *
   * @param p A node pointer.
   * @param c A node pointer.
   */
  void reorder(int p, int c);

  /**
   * Updates the (dis)proof numbers for t based on its children, given that
   * the (dis)proof numbers of some child c may have changed. Reorders t's
   * children by (dis)proof.
   *
   * Rationale: When selecting the MPN, we can always select the first child,
   * because that's the best child. However, when propagating the scores back
   * up, nodes can have multiple parents, so each node is *not* guaranteed to be
   * its parent's first child.
   */
  void update(int t, int c);
};

#endif
