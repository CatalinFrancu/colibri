#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "egtb.h"
#include "fileutil.h"
#include "precomp.h"
#include "queryserver.h"

int main(int argc, char **argv) {
  precomputeAll();
  initEgtb();

  int opt, command = 0;
  while ((opt = getopt(argc, argv, "s")) != -1) {
    switch (opt) {
      case 's': command = 1; break; // Query server
      default:
        fprintf(stderr, "Unknown switch -%c\n", opt);
        exit(1);
     }
  }

  if (command == 1) {
    startServer();
  }

//  string name = getFileNameForCombo("RRvNN");
//  string compressedName = getCompressedFileNameForCombo("RRvNN");
//  string idxName = getIndexFileNameForCombo("RRvNN");
//  compressFile(name.c_str(), compressedName.c_str(), idxName.c_str(), EGTB_CHUNK_SIZE, false);
//  generateEgtb("KKKvKK");
  generateAllEgtb(4, 1);
//  verifyEgtb("KBNvK");
}
