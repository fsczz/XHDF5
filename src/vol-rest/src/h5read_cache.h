#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_KEY_LEN 256

typedef struct CacheItem {
    char key[MAX_KEY_LEN];
    void *data;
    size_t size;
    struct CacheItem *prev, *next;
} CacheItem;

typedef struct {
    CacheItem *head, *tail;
    int capacity;
    int size;
    size_t total_size;
    size_t max_size;
    pthread_mutex_t lock;
} Cache;

Cache* cache_init(size_t capacity,size_t max_size);
void cache_destroy(Cache *cache);
void* cache_get(Cache *cache, const char *key);
void cache_set(Cache *cache, const char *key, void *data, size_t size);
void cache_delete(Cache *cache, const char *key);
void cache_evict(Cache *cache);

#endif /* CACHE_H */
