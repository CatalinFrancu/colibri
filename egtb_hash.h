#ifndef __EGTB_HASH_H__
#define __EGTB_HASH_H__

/**
 * A hash set for EGTB position indices. Designed for storing on the order of
 * 10-100 values, typically child positions generated from a parent position.
 * Due to this particularity, stored indices will often differ only in a few
 * bits.
 */

class EgtbHash {

  static const int BUCKETS = 64;
  static const int BUCKET_SIZE = 50;
  static const unsigned MULT = 83; // Knuth's multiplicative function

  unsigned data[BUCKETS][BUCKET_SIZE + 1];  // elements go here; allow for sentinels
  unsigned bucketCount[BUCKETS];            // number of elements added to each bucket
  unsigned dest[BUCKETS * BUCKET_SIZE];     // bucket where each element went
  unsigned count;                           // number of elements added

public:

  EgtbHash();
  void clear();
  void add(unsigned index);
  bool contains(unsigned index);

private:

  unsigned hash(unsigned index);

};
  
#endif
