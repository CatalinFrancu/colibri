#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__

/* Create the given file of specified length and fill it with the given character */
void createFile(const char *fileName, int size, char c);

/* Returns the size of the specified file, in bytes */
unsigned getFileSize(const char *fileName);

/* Reads the char at position. Assumes f is opened for r+ */
char readChar(FILE *f, int position);

/* Overwrites the char at position with the value c. Assumes f is opened for r+ */
void writeChar(FILE *f, int position, char c);

#endif
