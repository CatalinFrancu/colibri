#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__
#include <string>

/* Returns the file name for a table */
string getFileNameForCombo(const char *combo);

/* Returns the size of the specified file, in bytes */
unsigned getFileSize(const char *fileName);

/* Reads the char at position. Assumes f is opened for r+ */
char readChar(FILE *f, int position);

/* Log a note of interesting events during EGTB generation / probing */
void appendEgtbNote(const char *note, const char *combo);

#endif
