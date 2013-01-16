#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "configfile.h"
#include "egtb.h"
#include "fileutil.h"
#include "precomp.h"
#include "queryserver.h"

int main(int argc, char **argv) {
  loadConfigFile(CONFIG_FILE);
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

//  generateEgtb("KQBNvK");
//  verifyEgtb("KQBNvK");
//  compressEgtb("KQBNvK");
//  generateEgtb("KQBNvQ");
//  verifyEgtb("KQBNvQ");
//  compressEgtb("KQBNvQ");
//  generateEgtb("KQBNvR");
//  verifyEgtb("KQBNvR");
//  compressEgtb("KQBNvR");
//  generateAllEgtb(2, 2);
}
