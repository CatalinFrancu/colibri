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
#include "defines.h"
#include "egtb.h"
#include "queryserver.h"

/* Reads a board in FEN notation. Outputs an error message on one line and returns on all input errors.
 * Outputs a blank first line, then the score / number of moves / details for each move.
 */
void handleEgtbQuery(FILE *fin, FILE *fout) {
  char s[200];
  if (!fgets(s, 200, fin)) {
    fprintf(fout, "%d %d\n", INFTY, 0);
    return;
  }
  Board *b = fenToBoard(s);
  if (!b) {
    fprintf(fout, "Please make sure your FEN string is correct.\n");
  } else {
    string moveNames[200];
    string fens[200];
    int scores[200];
    int numMoves;
    int score = batchEgtbLookup(b, moveNames, fens, scores, &numMoves);
    fprintf(fout, "\n%d %d\n", score, numMoves);
    for (int i = 0; i < numMoves; i++) {
      fprintf(fout, "%s %d %s\n", moveNames[i].c_str(), scores[i], fens[i].c_str());
    }
    free(b);
  }
}

void* handleConnection(void* arg) {
  int fd = *(int*)arg;
  FILE *fin = fdopen(fd, "r");
  FILE *fout = fdopen(fd, "w");
  char s[100];

  // Read a command
  if (fscanf(fin, "%100s ", s) == 1) {
    if (!strcmp("egtb", s)) {
      handleEgtbQuery(fin, fout);
    }
  }
  fflush(fout);
  fclose(fin);
  fclose(fout);
  close(fd);
  return NULL;
}

void startServer() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock != -1);
  int n = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof (n));

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(QUERY_SERVER_PORT);
  sin.sin_addr.s_addr = INADDR_ANY;
  assert(!bind(sock, (struct sockaddr*)&sin, sizeof(sin)));
  assert(!listen(sock, 10));
  printf("Listening for connections on port %d\n", QUERY_SERVER_PORT);
  while (true) {
    int conn = accept(sock, NULL, NULL);
    assert(conn != -1);
    pthread_t* thread = new pthread_t;
    pthread_create(thread, NULL, handleConnection, &conn);
    pthread_detach(*thread);
  }
}
