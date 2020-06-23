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

void QueryServer::handleEgtbQuery(FILE* fin, FILE* fout, Board* b) {
  string cMoves[MAX_MOVES], cFens[MAX_MOVES];
  int numMoves, score, scores[MAX_MOVES];
  score = batchEgtbLookup(b, cMoves, cFens, scores, &numMoves);
  fprintf(fout, "egtb\n%d\n%d\n", score, numMoves);
    for (int i = 0; i < numMoves; i++) {
      fprintf(fout, "%s %d %s\n", cMoves[i].c_str(), scores[i], cFens[i].c_str());
    }
}

void QueryServer::handlePnsQuery(FILE* fin, FILE* fout, Board* b) {
  string cMoves[MAX_MOVES], cFens[MAX_MOVES];
  int numMoves;
  u64 proof, disproof, cProofs[MAX_MOVES], cDisproofs[MAX_MOVES];

  bool found = false;
  if (pns) {
    found = pns->batchLookup(b, &proof, &disproof, cMoves,
                             cFens, cProofs, cDisproofs, &numMoves);
  }

  if (found) {
    fprintf(fout, "pns\n%llu %llu\n%d\n", proof, disproof, numMoves);
    for (int i = 0; i < numMoves; i++) {
      fprintf(fout, "%s %llu %llu %s\n",
              cMoves[i].c_str(), cProofs[i], cDisproofs[i], cFens[i].c_str());
    }
  } else {
    fprintf(fout, "unknown\n");
  }
}

void QueryServer::handleQuery(FILE* fin, FILE* fout) {
  char s[200];
  if (!fgets(s, 200, fin)) {
    fprintf(fout, "error\nCould not read FEN string.\n");
    return;
  }

  Board b;
  if (!fenToBoard(s, &b)) {
    fprintf(fout, "error\nIncorrect FEN string.\n");
    return;
  }

  int numPieces = popCount(b.bb[BB_WALL] | b.bb[BB_BALL]);
  if (numPieces <= EGTB_MEN) {
    handleEgtbQuery(fin, fout, &b);
  } else {
    handlePnsQuery(fin, fout, &b);
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
