#include <assert.h>
#include <stdlib.h>
#include "egtb_queue.h"
#include "logging.h"

EgtbQueue::EgtbQueue(int size) {
  this->size = size;
  head = tail = enqTotal = deqTotal = 0;
  assert(codes = (unsigned*)malloc(size * sizeof(unsigned)));
  assert(scores = (char*)malloc(size));
}

EgtbQueue::~EgtbQueue() {
  free(codes);
  free(scores);
}

void EgtbQueue::enqueue(unsigned code, char score) {
  enqTotal++;
  codes[tail] = code;
  scores[tail++] = score;
  if (tail == size) {
    tail = 0;
  }
  if (!deqTotal && !(enqTotal & (LOG_EVERY - 1))) {
    // log enqueues in the initial phase, before dequeuing starts
    log(LOG_DEBUG, "%d positions enqueued", enqTotal);
  }
}

void EgtbQueue::dequeue(unsigned* code, char* score) {
  deqTotal++;
  *code = codes[head];
  *score = scores[head++];
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
