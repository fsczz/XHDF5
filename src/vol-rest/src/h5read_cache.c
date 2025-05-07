#include "h5read_cache.h"

Cache* cache_init(size_t capacity,size_t max_size) {
    Cache *cache = (Cache*)malloc(sizeof(Cache));
    if (!cache) {
        return NULL;
    }
    cache->capacity = capacity;
    cache->size = 0;
    cache->total_size = 0;
    cache->max_size = max_size;
    cache->head = cache->tail = NULL;
    pthread_mutex_init(&cache->lock, NULL);
    return cache;
}

void cache_destroy(Cache *cache) {
    pthread_mutex_lock(&cache->lock);
    CacheItem *current = cache->head;
    while (current) {
        CacheItem *next = current->next;
        free(current->data); // 确保释放数据内存
        free(current);
        current = next;
    }
    pthread_mutex_unlock(&cache->lock);
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

void move_to_head(Cache *cache, CacheItem *item) {
    if (item == cache->head) return;

    if (item->prev) item->prev->next = item->next;
    if (item->next) item->next->prev = item->prev;
    if (item == cache->tail) cache->tail = item->prev;

    item->next = cache->head;
    item->prev = NULL;
    if (cache->head) cache->head->prev = item;
    cache->head = item;

    if (!cache->tail) cache->tail = item;
}

void* cache_get(Cache *cache, const char *key) {
    pthread_mutex_lock(&cache->lock);
    CacheItem *item = cache->head;
    while (item) {
        if (strcmp(item->key, key) == 0) {
            move_to_head(cache, item);
            pthread_mutex_unlock(&cache->lock);
            return item->data;
        }
        item = item->next;
    }
    pthread_mutex_unlock(&cache->lock);
    return NULL;
}

void cache_set(Cache *cache, const char *key, void *data, size_t size) {
    pthread_mutex_lock(&cache->lock);
    CacheItem *item = cache->head;
    while (item) {
        if (strcmp(item->key, key) == 0) {
            cache->total_size -= item->size;
            item->data = data;
            item->size = size;
            cache->total_size += size;
            move_to_head(cache, item);
            pthread_mutex_unlock(&cache->lock);
            return;
        }
        item = item->next;
    }

    item = (CacheItem*)malloc(sizeof(CacheItem));
    if (!item) {
        pthread_mutex_unlock(&cache->lock);
        fprintf(stderr, "Failed to allocate memory for new cache item\n");
        return;
    }
    strcpy(item->key, key);
    item->data = data;
    item->size = size;
    item->prev = NULL;
    item->next = cache->head;
    if (cache->head) cache->head->prev = item;
    cache->head = item;

    if (!cache->tail) cache->tail = item;

    cache->size++;
    cache->total_size += size;

    while (cache->total_size > cache->capacity) {
        cache_evict(cache);
    }

    pthread_mutex_unlock(&cache->lock);
}

void cache_delete(Cache *cache, const char *key) {
    pthread_mutex_lock(&cache->lock);
    CacheItem *item = cache->head;
    while (item) {
        if (strcmp(item->key, key) == 0) {
            if (item->prev) item->prev->next = item->next;
            if (item->next) item->next->prev = item->prev;
            if (item == cache->head) cache->head = item->next;
            if (item == cache->tail) cache->tail = item->prev;
            cache->total_size -= item->size;
            free(item->data); // 确保释放数据内存
            free(item);
            cache->size--;
            pthread_mutex_unlock(&cache->lock);
            return;
        }
        item = item->next;
    }
    pthread_mutex_unlock(&cache->lock);
}

void cache_evict(Cache *cache) {
    if (!cache->tail) return;

    CacheItem *item = cache->tail;
    if (item->prev) item->prev->next = NULL;
    cache->tail = item->prev;

    if (item == cache->head) cache->head = NULL;

    cache->total_size -= item->size;
    free(item->data); // 确保释放数据内存
    free(item);
    cache->size--;
}