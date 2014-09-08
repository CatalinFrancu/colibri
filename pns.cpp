#include <stdio.h>
#include <sstream>
#include "board.h"
#include "logging.h"
#include "stringutil.h"

void pnsAnalyzeString(char *input) {
  Board *b;
  if (isFen(input)) {
    // Input is a board in FEN notation
    b = fenToBoard(NEW_BOARD);
  } else {
    // Input is a sequence of moves. Tokenize it.
    stringstream in(input);
    string moves[200];
    int n = 0;
    while (in.good() && n < 200){
      in >> moves[n++];
    }

    b = makeMoveSequence(n, moves);
  }
  if (b) {
    printBoard(b);
  } else {
    log(LOG_ERROR, "Invalid input for PNS analysis");
  }
}
