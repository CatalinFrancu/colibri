#include <stdlib.h>
#include "allocator.h"
#include "logging.h"

Allocator::Allocator(string name, int numElements) {
  this->name = name;
  this->numElements = numElements;
  stack = (int*)malloc(numElements * sizeof(int));
  reset();
}

int Allocator::alloc() {
  if (stackSize) {
    return stack[--stackSize];
  } else if (firstFree < numElements) {
    return firstFree++;
  } else {
    die("allocator %s is out of space", name.c_str());
    return 0; // please the compiler
  }
}

void Allocator::free(int index) {
  stack[stackSize++] = index;
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
  firstFree = 0;
  stackSize = 0;
}
