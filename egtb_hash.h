#ifndef __EGTB_HASH_H__
#define __EGTB_HASH_H__

/**
 * A hash set for EGTB indices. Designed for storing on the order of 10-100
 * indices, typically child positions generated from a parent position.
 */

class EgtbHash {

  static const int BITS = 6;
  static const int BUCKETS = 1 << BITS;
  static const int BUCKET_SIZE = 20;

  unsigned data[1 << BITS][BUCKET_SIZE]; // elements go here
  unsigned bucketCount[1 << BITS];       // number of elements added to each bucket
  unsigned dest[BUCKETS * BUCKET_SIZE];  // bucket where each element went
  unsigned count;                        // number of elements added

public:

  EgtbHash();
  void clear();
  void add(unsigned index);
  bool contains(unsigned index);
};
  
#endif
