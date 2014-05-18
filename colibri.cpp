#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "configfile.h"
#include "egtb.h"
#include "fileutil.h"
#include "logging.h"
#include "precomp.h"
#include "queryserver.h"

int main(int argc, char **argv) {
  loadConfigFile(CONFIG_FILE);
  logInit(cfgLogFile.c_str());
  precomputeAll();
  initEgtb();
  srand(time(NULL));

  int opt, command = 0;
  while ((opt = getopt(argc, argv, "s")) != -1) {
    switch (opt) {
      case 's': command = 1; break; // Query server
      default:
        log(LOG_ERROR, "Unknown switch -%c", opt);
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
  generateAllEgtb(3, 2);
}
