#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "board.h"
#include "configfile.h"
#include "defines.h"
#include "egtb.h"
#include "logging.h"
#include "queryserver.h"

/**
 * Utility function that delegates pthread callbacks to the query server.
 * I couldn't figure out how to pass a QueryServer method directly to pthread.
 */
void* queryServerHandler(void* arg) {
  QueryServer* qs = (QueryServer*)arg;
  qs->startSync();
  return NULL;  // unreachable
}

QueryServer::QueryServer(Pns* pns) {
  this->pns = pns;
}

/**
 * Reads a board in FEN notation. Outputs an error message on one line and
 * returns on all input errors. Otherwise, queries the EGTB or the PNS tree
 * depending on the number of pieces. Outputs a blank first line, then the
 * score / number of moves / details for each move.
 */
void QueryServer::handleQuery(FILE* fin, FILE* fout) {
  char s[200];
  if (!fgets(s, 200, fin)) {
    fprintf(fout, "%d %d\n", INFTY, 0);
    return;
  }
  Board b;
  if (!fenToBoard(s, &b)) {
    fprintf(fout, "Please make sure your FEN string is correct.\n");
  } else {
    string moveNames[MAX_MOVES], fens[MAX_MOVES], scores[MAX_MOVES], score;
    int numMoves;
    int numPieces = popCount(b.bb[BB_WALL] | b.bb[BB_BALL]);
    if (pns && (numPieces > EGTB_MEN)) {
      score = pns->batchLookup(&b, moveNames, fens, scores, &numMoves);
    } else {
      score = batchEgtbLookup(&b, moveNames, fens, scores, &numMoves);
    }
    fprintf(fout, "\n%s %d\n", score.c_str(), numMoves);
    for (int i = 0; i < numMoves; i++) {
      fprintf(fout, "%s %s %s\n", moveNames[i].c_str(), scores[i].c_str(), fens[i].c_str());
    }
  }
}


void QueryServer::handleConnection(int fd) {
  FILE *fin = fdopen(fd, "r");
  FILE *fout = fdopen(fd, "w");
  char s[100];

  // Read a command
  if (fscanf(fin, "%100s ", s) == 1) {
    if (!strcmp("query", s)) {
      handleQuery(fin, fout);
    }
  }
  fflush(fout);
  fclose(fin);
  fclose(fout);
  close(fd);
}

void QueryServer::startSync() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock != -1);
  int n = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof (n));

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(cfgQueryServerPort);
  sin.sin_addr.s_addr = INADDR_ANY;
  assert(!bind(sock, (struct sockaddr*)&sin, sizeof(sin)));
  assert(!listen(sock, 10));
  log(LOG_INFO, "Listening for connections on port %d", cfgQueryServerPort);
  while (true) {
    int fd = accept(sock, NULL, NULL);
    assert(fd != -1);
    handleConnection(fd);
  }
}

void QueryServer::startAsync() {
  pthread_t* thread = new pthread_t;
  pthread_create(thread, NULL, queryServerHandler, this);
  pthread_detach(*thread);
}
