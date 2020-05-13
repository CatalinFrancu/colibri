#ifndef __EGTB_QUEUE_H__
#define __EGTB_QUEUE_H__

/**
 * Class that stores solved but unexpanded positions during the generation of
 * a particular table. Since the table is generated in breadth-first order
 * using retrograde analysis, open positions are stored in a circular buffer.
 *
 * Each element stores a board's EGTB code and index.
 */

typedef struct {
  unsigned code, index;
} EgtbQueueElement;

class EgtbQueue {

  EgtbQueueElement *queue;
  int size; // maximum number of elements
  int head, tail; // indices of first used slot and first free slot
  int total; // total number of elements enqueued

public:

  EgtbQueue(int size);
  ~EgtbQueue();
  void enqueue(unsigned code, unsigned index);
  void dequeue(unsigned* code, unsigned* index);
  bool isEmpty();
  int getTotal();

};

#endif
