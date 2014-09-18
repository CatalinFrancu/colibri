#ifndef PNS_H
#define PNS_H

/* Initializes PNS-related data structures. */
void pnsInit();

/* Creates a PnsTree node with no children and no parents and proof/disproof values of 1.
 * Returns its index in the statically allocated array. */
int pnsMakeLeaf();

/* Deallocates a PnsTree. */
void pnsFree(PnsNode *t);

/* Expands the most proving node and sets its proof and disproof numbers.
 * Does not create child nodes if the node has a known EGTB value or if there are no legal moves. */
void pnsExpand(PnsNode *t, Board *b);

/* Takes an input string which can be a FEN board or a sequence of moves. Constructs the board and analyze it.
 * Loads an existing PNS^2 tree from fileName if the file exists. Saves periodically to fileName. */
void pn2AnalyzeString(string input, string fileName);

/* Analyzes a board using proof number search.
 * @param root -- the tree so far (either loaded from a file or initialized with a single node)
 * @param b -- the board to anayze
 * @param maxNodes -- create this many new nodes (0 for no limit)
 */
void pnsAnalyzeBoard(PnsNode *root, Board *b, int maxNodes);

#endif
