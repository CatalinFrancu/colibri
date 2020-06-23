#ifndef __QUERYSERVER_H__
#define __QUERYSERVER_H__

#include "pns.h"

/**
 * A query server that answers EGTB / book lookups.
 */
class QueryServer {

  Pns* pns;

public:
  /**
   * Starts the query server.
   * @param pns The book, or null if there is no book.
   */
  QueryServer(Pns* pns);

  /**
   * Creates and detaches a listener thread. All the socket binding and
   * listening is done in the thread.
   */
  void startAsync();

  void startSync();

private:

  /**
   * Reads a board in FEN notation. Outputs an error message on one line and
   * returns on all input errors. Otherwise, queries the EGTB or the PNS tree
   * depending on the number of pieces. Outputs a blank first line, then the
   * score / number of moves / details for each move.
   */
  void handleQuery(FILE* fin, FILE* fout);

  void handleEgtbQuery(FILE* fin, FILE* fout, Board* b);

  void handlePnsQuery(FILE* fin, FILE* fout, Board* b);

  void handleConnection(int fd);

};

#endif
