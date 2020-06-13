#include <assert.h>
#include <stdlib.h>
#include "allocator.h"
#include "logging.h"

Allocator::Allocator(string name, int numElements) {
  this->name = name;
  this->numElements = numElements;
  stack = (int*)malloc(numElements * sizeof(int));
  inUse = (bool*)malloc(numElements * sizeof(bool));
  reset();
}

int Allocator::alloc() {
  int result;
  if (stackSize) {
    result = stack[--stackSize];
  } else if (firstFree < numElements) {
    result = firstFree++;
  } else {
    die("allocator %s is out of space", name.c_str());
    result = 0; // please the compiler
  }
  assert(!inUse[result]);
  inUse[result] = true;
  return result;
}

void Allocator::free(int index) {
  assert(stackSize < numElements);
  assert(index >= 0);
  assert(index < numElements);
  assert(inUse[index]);
  inUse[index] = false;
  // log(LOG_DEBUG, "freed %s %d", name.c_str(), index);
  stack[stackSize++] = index;
}

bool Allocator::isInUse(int index) {
  return inUse[index];
}

int Allocator::used() {
  return firstFree - stackSize;
}

int Allocator::available() {
  // There are two types of free elements: those never yet allocated (from
  // firstFree to numElements), and those previously freed, sitting on the
  // stack.
  return numElements - firstFree + stackSize;
}

void Allocator::reset() {
  for (int i = 0; i < numElements; i++) {
    inUse[i] = false;
  }
  firstFree = 0;
  stackSize = 0;
}
