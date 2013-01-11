#ifndef __LRUCACHE_H__
#define __LRUCACHE_H__
#include <unordered_map>
#include "defines.h"

using namespace std;

typedef struct _list {
  u64 key;
  _list *prev, *next;
} DoublyLinkedList;

typedef struct {
  void *data;
  DoublyLinkedList *listPtr;
} LruCacheElement;

/* First key (sentinel->next) is the oldest. Last key (sentinel->prev) is the newest. */
typedef struct {
  unordered_map<u64, LruCacheElement> map;
  DoublyLinkedList *sentinel;
  int curSize, maxSize;
  u64 lookups, misses, evictions;
} LruCache;

/* Creates an empty LRU Cache */
LruCache lruCacheCreate(int maxSize);

/* Adds a key / value pair to the cache. May evict the LRU entry if full.
 * Assumes the key does not exist in the cache. */
void lruCachePut(LruCache *cache, u64 key, void *value);

/* Looks up the given key, returns the corresponding value and updates the LRU access order */
void* lruCacheGet(LruCache *cache, u64 key);

/* Print cache statistics */
void printCacheStats(LruCache *cache, const char *msg);

#endif
