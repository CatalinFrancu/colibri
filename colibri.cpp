#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "configfile.h"
#include "egtb.h"
#include "fileutil.h"
#include "logging.h"
#include "pns.h"
#include "precomp.h"
#include "queryserver.h"
#include "zobrist.h"

void setCommand(int *oldCmd, int newCmd) {
  if (*oldCmd) {
    die("Only one command out of -a and -s may be given.");
  }
  *oldCmd = newCmd;
}

int main(int argc, char **argv) {
  //srand(time(NULL));
  srand(1234);
  loadConfigFile(CONFIG_FILE);
  logInit(cfgLogFile.c_str());
  precomputeAll();
  initEgtb();
  zobristInit();

  string bookFile = cfgBookFile, position = "";
  int command = 0;
  int opt;
  opterr = 0; // Suppresses error messages from getopt()
  while ((opt = getopt(argc, argv, "a:f:s")) != -1) {
    switch (opt) {
      case 'f':
        bookFile = optarg;
        break;
      case 'a':
        setCommand(&command, CMD_ANALYZE);
        position = optarg;
        break;
      case 's':
        setCommand(&command, CMD_SERVER);
        break;
      default:
        die("Unknown switch -%c", optopt);
     }
  }

  Pns pn1(60000, 10000000);
  Pns pn2(10000000, 100000000, &pn1, bookFile);
  QueryServer qs(&pn2);
  pn2.load();

  switch (command) {
    case CMD_ANALYZE:
      qs.startAsync();
      pn2.analyzeString(position);
      pn2.save();
      break;
    case CMD_SERVER:
      qs.startSync();
      break;
    default:
      log(LOG_WARNING, "No command given. Resuming main() execution.");
  }

  // const char* table = "BPPvPP";
  // generateEgtb(table);
  // verifyEgtb(table);
  // compressEgtb(table);
  // generateAllEgtb(2, 2);

  log(LOG_DEBUG, "shutting down");
}
