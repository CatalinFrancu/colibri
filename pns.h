#ifndef PNS_H
#define PNS_H

#include "allocator.h"

/* A PN search tree node (it's really a DAG). Pointers are statically represented in a preallocated memory area. */
typedef struct {
  u64 proof;
  u64 disproof;
  u64 zobrist;       // zobrist key of the position (0 if the node is a repetition)
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

  /**
   * Maps Zobrist keys to pairs of indices in node[]. Each position can occur
   * twice in the DAG: once for transpositions, with (dis)proof numbers
   * computed as usually, and once for repetitions, with (dis)proof numbers
   * set to inf/inf initially. These are stored first and second in the pair,
   * respectively. If the first node is eventually solved, we mark the second
   * one as solved as well and run update() from it once.
   *
   * See Proof-Number Search and Transpositions, Schijf 1993, section 5.4.
   */
  unordered_map<u64, pair<int,int>> trans;

  /* Set of nodes on any path from the MPN to the root. Stores indices in node. */
  unordered_set<int> ancestors;

  /* Set of nodes visiting during a trim operation, for preventing reentry. */
  unordered_set<int> seen;

  /* First level PN tree, if we are a second level PN tree. */
  Pns* pn1;

  int numEgtbLookups;
  bool trim; // whether or not non-winning edges should be trimmed

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

  /**
   * @return True iff node t is solved as a win, loss or draw.
   */
  bool isSolved(int t);

  /* Creates a PNS tree node with no children and no parents and proof/disproof values of 1.
   * Returns its index in the preallocated array. */
  int allocateLeaf(u64 p, u64 d);

  /* Adds a parent to a PNS node (presumably because we found a transposed path to it).
   * The new pointer is added at the front of the parent list. */
  void addParent(int childIndex, int parentIndex);

  /**
   * Prepends a node to a parent's child list.
   */
  void addChild(int parentIndex, int childIndex, Move m);

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
   * Looks up a zobrist key in the transposition table. Depending on the
   * presence of the key and on the contents of the ancestors hash map,
   * returns one of the existing transposition/repetition nodes or creates a
   * new one.
   */
  int zobristLookup(u64 z, u64 proof, u64 disproof);

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

  /**
   * Severs all links from t to its children and viceversa. Assumes that t's
   * first child wins.
   */
  void trimNonWinning(int t);

  /**
   * Finds p in c's parent list and deletes the edge. If c becomes orphaned,
   * deletes it.
   */
  void deleteParentLink(int c, int p);

  /**
   * Severs all links from t to its children and viceversa. Reclaims t for
   * reuse. t is assumed to have no parents.
   */
  void deleteNode(int t);

  /**
   * Marks t's repetition node as solved and updates that node's ancestors.
   * Assumes t has just been solved.
   */
  void solveRepetition(int t);

  /**
   * Performs a recursive consistency check of the DAG.
   * @param t Node to verify
   * @param b Board corresponding to t
   * @param seenNodes set of seen nodes (to prevent reentry)
   * @param seenEdges global set of edge pointers (to check for duplicates)
   */
  void verifyConsistency(int t, Board *b, unordered_set<int>* seenNodes,
                         unordered_set<int>* seenEdges);

  /**
   * Performs some additional checks on top of verifyConsistency().
   * @param b Board corresponding to the root node.
   */
  void verifyConsistencyWrapper(Board *b);

};

#endif
