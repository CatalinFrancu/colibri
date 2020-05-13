#include <assert.h>
#include <stdlib.h>
#include "egtb_queue.h"
#include "logging.h"

EgtbQueue::EgtbQueue(int size) {
  this->size = size;
  head = tail = total = 0;
  assert(queue = (EgtbQueueElement*)malloc(size * sizeof(EgtbQueueElement)));
}

EgtbQueue::~EgtbQueue() {
  free(queue);
}

void EgtbQueue::enqueue(unsigned code, unsigned index) {
  total++;
  queue[tail].code = code;
  queue[tail++].index = index;
  if (tail == size) {
    tail = 0;
  }
  if (!(total & ((1 << 22) - 1))) {
    log(LOG_DEBUG, "%d positions enqueued", total);
  }
}

void EgtbQueue::dequeue(unsigned* code, unsigned* index) {
  *code = queue[head].code;
  *index = queue[head++].index;
  if (head == size) {
    head = 0;
  }
}

bool EgtbQueue::isEmpty() {
  return head == tail;
}

int EgtbQueue::getTotal() {
  return total;
}
