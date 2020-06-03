#ifndef PNS_H
#define PNS_H

/*
 * Class that handles proof-number search.
 */
class Pns {
  /* Preallocated array of PNS tree nodes */
  PnsNode *node;
  int nodeSize, nodeMax;

  /* Preallocated array of moves (links between PNS tree nodes) */
  Move *move;
  int moveSize, moveMax;

  /* Preallocated array of child pointers (links between PNS tree nodes) */
  int *child;
  int childSize, childMax;

  /* Preallocated array of parent pointers (parents are stored in linked lists) */
  PnsNodeList *parent;
  int parentSize, parentMax;

  /* Hash table of positions anywhere in the PNS tree. Maps Zobrist keys to indices in node. */
  unordered_map<u64, int> trans;

  /* Set of nodes on any path from the MPN to the root. Stores indices in node. */
  unordered_set<int> ancestors;

  /* First level PN tree, if we are a second level PN tree. */
  Pns* pn1;

public:

  /* Creates a new Pns with the given size limits and one leaf. */
  Pns(int nodeMax, int moveMax, int childMax, int parentMax, Pns* pn1);

  /* Constructs a P/N tree until the position is proved or the tree exceeds nodeMax. */
  void analyzeBoard(Board *b);

  /* Analyze a string, which can be a sequence of moves or a position in FEN.
   * Loads a previous PNS tree from fileName, if it exists. */
  void analyzeString(string input, string fileName);

  /* Returns the root's proof number */
  u64 getProof();

  /* Returns the root's disproof number */
  u64 getDisproof();

  /* Returns a specific node. Used in unit tests. */
  PnsNode* getNode(int t);

  /* Returns a specific move. Used in unit tests. */
  Move getMove(int m);

  /* Returns the parent count of a PnsNode. Used in unit tests. */
  int getNumParents(PnsNode* t);

  /* Returns the k-th parent of node t. Used in unit tests. */
  int getParent(PnsNode* t, int k);
  
  /* Clears all the information from the tree, retaining one unexplored leaf. */
  void reset();

private:

  /** Memory management, debugging, loading, saving **/

  /* Creates a PNS tree node with no children and no parents and proof/disproof values of 1.
   * Returns its index in the preallocated array. */
  int allocateLeaf();

  /* Allocates n moves in the preallocated memory. */
  int allocateMoves(int n);

  /* Allocates n children pointers in the preallocated memory. */
  int allocateChildren(int n);

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

  /** Proof-number search **/

  /* Adds a node's ancestors to the ancestor hash map. */
  void hashAncestors(int t);

  /* Finds the most proving node in a PNS tree. Starting with the original board b, also makes the necessary moves
   * modifying b, returning the position corresponding to the MPN. */
  int selectMpn(Board *b);

  /* Sets the proof and disproof numbers for a board with no legal moves. */
  void setScoreNoMoves(int t, Board *b);

  /* Sets the proof and disproof numbers for an EGTB board. */
  void setScoreEgtb(int t, int score);

  /* copy the PN1 root's moves to our own move array */
  void copyMovesFromPn1();

  /* Expands the given leaf with 1/1 children. If there are no legal moves,
     sets the P/D numbers accordingly. If it runs out of tree space, returns
     false. */
  bool expand(int t, Board *b);

  /* Propagate this node's values to each of its parents. */
  void update(int t);
};

#endif
