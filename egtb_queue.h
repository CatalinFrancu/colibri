#ifndef __EGTB_QUEUE_H__
#define __EGTB_QUEUE_H__

/**
 * Class that stores solved but unexpanded positions during the generation of
 * a particular table. Since the table is generated in breadth-first order
 * using retrograde analysis, open positions are stored in a circular buffer.
 *
 * Each element stores a board's EGTB code and score.
 */

class EgtbQueue {

  unsigned* codes;
  char* scores;
  int size; // maximum number of elements
  int head, tail; // indices of first used slot and first free slot
  int enqTotal, deqTotal; // total number of elements enqueued and dequeued
  static const int LOG_EVERY = 1 << 22;

public:

  EgtbQueue(int size);
  ~EgtbQueue();
  void enqueue(unsigned code, char score);
  void dequeue(unsigned* code, char* score);
  bool isEmpty();
  int getTotal();

};

#endif
