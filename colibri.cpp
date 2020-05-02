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
  //pnsInit();

  string fileName = "", position = "";
  int command = 0;
  int opt;
  opterr = 0; // Suppresses error messages from getopt()
  while ((opt = getopt(argc, argv, "a:f:s")) != -1) {
    switch (opt) {
      case 'f':
        fileName = optarg;
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

  Pns pns(PNS_STEP_SIZE, PNS_MOVE_SIZE, PNS_CHILD_SIZE, PNS_PARENT_SIZE);
  switch (command) {
    case CMD_ANALYZE:
      if (fileName.empty()) {
        die("Please specify and input/output file with -f.");
      }
      pns.analyzeString(position, fileName);
      break;
    case CMD_SERVER:
      startServer(); break;
    default:
      log(LOG_WARNING, "No command given. Resuming main() execution.");
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
  // generateAllEgtb(3, 2);
}
