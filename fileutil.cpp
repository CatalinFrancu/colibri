#include <stdio.h>
#include <sys/stat.h>
#include "bitmanip.h"
#include "defines.h"
#include "fileutil.h"
#include "precomp.h"

string getFileNameForCombo(const char *combo) {
  return string(EGTB_PATH) + "/" + combo + ".egt";
}

unsigned getFileSize(const char *fileName) {
  struct stat st;
  stat(fileName, &st);
  return st.st_size;
}

void appendEgtbNote(const char *note, const char *combo) {
  string fileName = string(EGTB_PATH) + "/notes.txt";
  FILE *f = fopen(fileName.c_str(), "at");
  fprintf(f, "Combo %s: %s\n", combo, note);
  fclose(f);
}
