#include <assert.h>
#include <stdlib.h>
#include "egtb_queue.h"
#include "logging.h"

EgtbQueue::EgtbQueue(int size) {
  this->size = size;
  head = tail = enqTotal = deqTotal = 0;
  assert(queue = (EgtbQueueElement*)malloc(size * sizeof(EgtbQueueElement)));
}

EgtbQueue::~EgtbQueue() {
  free(queue);
}

void EgtbQueue::enqueue(unsigned code, unsigned index) {
  enqTotal++;
  queue[tail].code = code;
  queue[tail++].index = index;
  if (tail == size) {
    tail = 0;
  }
  if (!deqTotal && !(enqTotal & (LOG_EVERY - 1))) {
    // log enqueues in the initial phase, before dequeuing starts
    log(LOG_DEBUG, "%d positions enqueued", enqTotal);
  }
}

void EgtbQueue::dequeue(unsigned* code, unsigned* index) {
  deqTotal++;
  *code = queue[head].code;
  *index = queue[head++].index;
  if (head == size) {
    head = 0;
  }
  if (!(deqTotal & (LOG_EVERY - 1))) {
    log(LOG_DEBUG, "%d positions dequeued, queue size %d", deqTotal, enqTotal - deqTotal);
  }
}

bool EgtbQueue::isEmpty() {
  return head == tail;
}

int EgtbQueue::getTotal() {
  return enqTotal;
}
