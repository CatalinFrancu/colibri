#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "egtb.h"
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

  generateEgtb("PPvP");
//  Board b = fenToBoard("K7/8/8/8/8/8/8/K1k5 b - - 0 0");
//  PieceSet ps[EGTB_MEN];
//  int nps = comboToPieceSets("KKvK", ps);
//  int score = egtbLookup(&b);
//  int index = getEgtbIndex(ps, nps, &b);
//  printf("Score %d index %d:\n", score, index);
  //printBoard(&b);
//  generateAllEgtb(2, 1);
//  verifyEgtb("PPvP");
}
