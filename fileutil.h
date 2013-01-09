#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__
#include <string>

/* Returns the file name for a table */
string getFileNameForCombo(char *combo);

/* Returns the size of the specified file, in bytes */
unsigned getFileSize(const char *fileName);

/* Reads the char at position. Assumes f is opened for r+ */
char readChar(FILE *f, int position);

/* Given a board, compute the number of the file where its EGTB should be.
 * Assumes the board has at most EGTB_MEN pieces.
 * Assumes the board has proper sides, i.e. White has more pieces than Black or they have the same number of pieces and White's
 * combination of pieces is greater than Black's. */
int boardToFileNumber(Board *b);

/* Same as boardToFileNumber(), but takes a combo, not a board.
 * This function is slow and should be used for EGTB generation, not probing. */
int comboToFileNumber(const char *combo);

#endif
