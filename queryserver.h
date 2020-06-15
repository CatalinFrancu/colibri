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

  void handleQuery(FILE* fin, FILE* fout);

  void handleConnection(int fd);

};

#endif
