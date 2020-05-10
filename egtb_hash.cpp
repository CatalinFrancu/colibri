#include "egtb_hash.h"

EgtbHash::EgtbHash() {
  for (int i = 0; i < BUCKETS; i++) {
    bucketCount[i] = 0;
  }
  count = 0;
}

void EgtbHash::clear() {
  // Decrease bucket counts for used buckets only. Should be faster than
  // zeroing all 64 bucket counts.
  while (count) {
    bucketCount[dest[--count]]--;
  }
}

void EgtbHash::add(unsigned index) {
  int b = index & (BUCKETS - 1);
  data[b][bucketCount[b]++] = index;
  dest[count++] = b;
}

bool EgtbHash::contains(unsigned index) {
  int b = index & (BUCKETS - 1);
  data[b][bucketCount[b]] = index; // sentinel

  unsigned i = 0;
  while (data[b][i] != index) {
    i++;
  }
  return i < bucketCount[b];
}
