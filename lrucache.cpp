#include <stdio.h>
#include <stdlib.h>
#include "logging.h"
#include "lrucache.h"

LruCache lruCacheCreate(int maxSize) {
  LruCache l;
  l.sentinel = (DoublyLinkedList*)malloc(sizeof(DoublyLinkedList));
  l.sentinel->next = l.sentinel;
  l.sentinel->prev = l.sentinel;
  l.curSize = 0;
  l.maxSize = maxSize;
  l.lookups = l.misses = l.evictions = 0;
  return l;
}

void lruCachePut(LruCache *cache, u64 key, void *value) {
  DoublyLinkedList *sent = cache->sentinel;

  if (cache->curSize == cache->maxSize) {
    // Delete the oldest entry
    DoublyLinkedList *dead = sent->next;
    free(cache->map[dead->key].data);
    cache->map.erase(dead->key);
    dead->prev->next = dead->next;
    dead->next->prev = dead->prev;
    free(dead);
    cache->curSize--;
    cache->evictions++;
  }

  DoublyLinkedList *l = (DoublyLinkedList*)malloc(sizeof(DoublyLinkedList));
  l->key = key;
  l->prev = sent->prev;
  l->next = sent;
  sent->prev->next = l;
  sent->prev = l;
  LruCacheElement elem;
  elem.data = value;
  elem.listPtr = l;
  cache->map[key] = elem;
  cache->curSize++;
}

void* lruCacheGet(LruCache *cache, u64 key) {
  LruCacheElement elem = cache->map[key];
  if (elem.data) {
    // Delete the list pointer from its current position...
    DoublyLinkedList *l = elem.listPtr, *sent = cache->sentinel;
    l->prev->next = l->next;
    l->next->prev = l->prev;
    // ... and move it to the end of the list
    l->prev = sent->prev;
    l->next = sent;
    sent->prev->next = l;
    sent->prev = l;
  } else {
    cache->misses++;
  }
  cache->lookups++;
  return elem.data;
}

void logCacheStats(LruCache *cache, const char *msg) {
  log(LOG_INFO, "%s cache stats: %llu lookups / %llu misses / %llu evictions", msg, cache->lookups, cache->misses, cache->evictions);
}
