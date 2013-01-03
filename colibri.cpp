#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "egtb.h"
#include "precomp.h"
#include "queryserver.h"

int main(int argc, char **argv) {
  precomputeAll();

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

  // generateEgtb("KvR");
  Board b = fenToBoard("8/8/8/5k2/8/8/2R5/8 w - - 0 0");
  printBoard(&b);
  string moveNames[200];
  string fens[200];
  int scores[200];
  int numMoves;
  int score = batchEgtbLookup(&b, moveNames, fens, scores, &numMoves);
  printf("%d\n", score);
  for (int i = 0; i < numMoves; i++) {
    printf("%s %d %s\n", moveNames[i].c_str(), scores[i], fens[i].c_str());
  }


  return 0;
}
