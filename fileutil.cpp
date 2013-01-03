#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "fileutil.h"

void createFile(const char *fileName, int size, char c) {
  int blockSize = 4096;
  char buf[blockSize];
  memset(buf, c, blockSize);
  FILE *f = fopen(fileName, "w");
  for (int i = 0; i < size / blockSize; i++) {
    fwrite(buf, blockSize, 1, f);
  }
  fwrite(buf, size % blockSize, 1, f);
  fclose(f);
}

unsigned getFileSize(const char *fileName) {
  struct stat st;
  stat(fileName, &st);
  return st.st_size;
}

void writeChar(FILE *f, int position, char c) {
  fseek(f, position, SEEK_SET);
  fputc(c, f);
}
