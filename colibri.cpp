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

  string destName = string(EGTB_PATH) + "/PvP.egt";
  unlink(destName.c_str());
  generateEgtb("PvP");

  Board b;
  int score;

  b = fenToBoard("8/8/8/5p2/P7/8/8/8 w - f6 0 0");
  score = egtbLookup(&b);
  printf("SCORE %d\n", score);
  return 0;
}
