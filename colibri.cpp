#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "precomp.h"
#include "queryserver.h"

int main(int argc, char **argv) {
  precomputeAll();

  int opt, command = 0;
  while ((opt = getopt(argc, argv, "w")) != -1) {
    switch (opt) {
      case 'q': command = 1; break; // Query server
      default:
        fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n", argv[0]);
        exit(1);
     }
  }

  if (command == 0) {
    startServer();
  }

  // generateEgtb("KvR");
  return 0;
}
