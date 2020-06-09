#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <string>
#include "defines.h"

/**
 * A class that manages indices in a previously allocated array. Deallocated
 * elements are stacked for reuse. Does not perform boundary checks.
 */
class Allocator {

  string name;     // a human-readable string to be used in error messages
  int numElements; // number of elements to accommodate
  int firstFree;   // first index that has never been allocated
  int* stack;      // stack of deallocated indices
  int stackSize;   // number of deallocated indices

public:
  /**
   * @param name A human-readable name.
   * @param numElements Number of elements to manage.
   */
  Allocator(string name, int numElements);

  /**
   * Returns the index of a free element. Terminates the program if there are no
   * free slots.
   */
  int alloc();

  /**
   * Marks the given index as free, making it available for reuse. Does not
   * perform sanity checks.
   */
  void free(int index);

  /**
   * Returns the number of used elements.
   */
  int used();

  /**
   * Returns the number of free elements.
   */
  int available();

  /**
   * Resets the allocator to an empty state.
   */
  void reset();

};

#endif
