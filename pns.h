#ifndef PNS_H
#define PNS_H

/* Creates a PnsTree node with no children and no parents and proof/disproof values of 1. */
PnsTree* pnsMakeLeaf();

/* Expands the most proving node and sets its proof and disproof numbers.
 * Does not create child nodes if the node has a known EGTB value or if there are no legal moves. */
void pnsExpand(PnsTree *t, Board *b);

/* Take an input string which can be a FEN board or a sequence of moves. Construct the board and analyze it.
 * Loads an existing PNS^2 tree from fileName if the file exists. Saves periodically to fileName. */
void pnsAnalyzeString(string input, string fileName);

/* Analyze a board using proof number search */
void pnsAnalyzeBoard(Board *b);

#endif
