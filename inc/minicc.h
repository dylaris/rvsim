/*
 * Mini C Container (array/hash_table)
 *
 * Ref:
 * -  https://www.gamedeveloper.com/programming/minimalist-container-library-in-c-part-1-
 * -  https://www.gamedeveloper.com/programming/minimalist-container-library-in-c-part-2-
 */

#ifndef MINICC_H
#define MINICC_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#ifndef MINICCDEF
#define MINICCDEF
#endif

#ifndef INIT_CAP
#define INIT_CAP 64
#endif

typedef struct {
    uint32_t len;
    uint32_t cap;
} ArrayHeader;

#define __Array

#define array_header(arr) ((ArrayHeader *) ((char *)(arr) - sizeof(ArrayHeader)))
#define array_len(arr) ((arr) ? array_header(arr)->len : 0)
#define array_cap(arr) ((arr) ? array_header(arr)->cap : 0)

#define array_push(arr, item) \
    do { \
        ArrayHeader *hdr = (arr) ? array_header(arr) : NULL; \
        uint32_t old_cap = array_cap(arr); \
        uint32_t old_len = array_len(arr); \
        if (old_len + 1 > old_cap) { \
            uint32_t new_cap = old_cap < INIT_CAP ? INIT_CAP : old_cap * 2; \
            hdr = (ArrayHeader *) realloc(hdr, sizeof(ArrayHeader) + sizeof(*(arr)) * new_cap); \
            assert(hdr && "run out of memory"); \
            hdr->cap = new_cap; \
            hdr->len = old_len; \
            arr = (void *) ((char *) hdr + sizeof(ArrayHeader)); \
        } \
        arr[hdr->len++] = (item); \
    } while (0)

#define array_pop(arr) \
    do { \
        if (array_len(arr) == 0) \
            break; \
        ArrayHeader *hdr = array_header(arr); \
        hdr->len--; \
    } while (0)

#define array_free(arr) \
    do { \
        if (arr) { \
            ArrayHeader *hdr = array_header(arr); \
            free(hdr); \
            (arr) = NULL; \
        } \
    } while (0)

#define array_tail(arr) ((arr) ? ((arr) + array_len(arr) - 1) : NULL)
#define array_end(arr) ((arr) ? ((arr) + array_len(arr)) : NULL)

#define array_foreach(arr, T, it) for (T *it = (arr); it < array_end(arr); it++)

#define array_ensure(arr, min_cap) \
    do { \
        ArrayHeader *hdr = (arr) ? array_header(arr) : NULL; \
        uint32_t old_cap = array_cap(arr); \
        uint32_t old_len = array_len(arr); \
        if (old_cap < min_cap) { \
            uint32_t new_cap = min_cap < old_cap * 2 ? old_cap * 2 : min_cap; \
            if (new_cap < INIT_CAP) \
                new_cap = INIT_CAP; \
            hdr = (ArrayHeader *) realloc(hdr, sizeof(ArrayHeader) + sizeof(*(arr)) * new_cap); \
            assert(hdr && "run out of memory"); \
            hdr->cap = new_cap; \
            hdr->len = old_len; \
            arr = (void *) ((char *) hdr + sizeof(ArrayHeader)); \
        } \
    } while (0)

#define array_resize(arr, new_len) \
    do { \
        if (arr) { \
            ArrayHeader *hdr = array_header(arr); \
            hdr->len = (new_len); \
        } \
    } while (0)

#define array_reset(arr) \
    do { \
        if (arr) { \
            ArrayHeader *hdr = array_header(arr); \
            hdr->len = 0; \
        } \
    } while (0)

#define array_swap(arr, i, j) \
    do { \
        if (array_len(arr) == 0) \
            break; \
        uint8_t *barr = (uint8_t *) (arr); \
        uint64_t size = sizeof(*(arr)); \
        uint64_t a = (i) * size; \
        uint64_t b = (j) * size; \
        for (uint64_t k = 0; k < size; k++) { \
            uint8_t tmp = barr[a + k]; \
            barr[a + k] = barr[b + k]; \
            barr[b + k] = tmp; \
        } \
    } while (0)

typedef struct {
    uint64_t __Array *keys;
    uint64_t __Array *values;
    uint32_t capacity;
    uint32_t count;
} Hash;

#define HASH_LOAD_FACTOR 0.75
#define HASH_UNUSED 0xffffffffffffffffULL

MINICCDEF Hash hash_create(uint32_t init_cap);
MINICCDEF void hash_destroy(Hash *hash);
MINICCDEF bool hash_grow(Hash *hash, uint32_t new_cap);
MINICCDEF bool hash_set(Hash *hash, uint64_t key, uint64_t value);
MINICCDEF uint64_t hash_get(const Hash *hash, uint64_t key, uint64_t invalid_value);
MINICCDEF void hash_clear(Hash *hash);
MINICCDEF uint64_t murmurhash64(const void *key, size_t len, uint64_t seed);
#define hash_full(h) ((h)->count == (h)->capacity)
#define hash_mem(p, sz) murmurhash64(p, sz, 0)
#define hash_str(s) murmurhash64(s, strlen(s), 0)

#endif // MINICC_H

#ifdef MINICC_IMPLEMENTATION

#include <string.h>

MINICCDEF Hash hash_create(uint32_t init_cap)
{
    if (init_cap < INIT_CAP)
        init_cap = INIT_CAP;

    uint64_t *keys = NULL, *values = NULL;
    array_ensure(keys, init_cap);
    array_ensure(values, init_cap);
    memset(keys, 0xff, sizeof(*keys) * array_cap(keys));

    return (Hash) {
        .keys     = keys,
        .values   = values,
        .capacity = array_cap(keys),
        .count    = 0,
    };
}

MINICCDEF void hash_destroy(Hash *hash)
{
    array_free(hash->keys);
    array_free(hash->values);
    hash->capacity = 0;
    hash->count = 0;
}

MINICCDEF bool hash_grow(Hash *hash, uint32_t new_cap)
{
    if (new_cap <= hash->capacity)
        return false;

    Hash new_hash = hash_create(new_cap);
    for (uint32_t i = 0; i < hash->capacity; i++) {
        if (hash->keys[i] == HASH_UNUSED)
            continue;
        hash_set(&new_hash, hash->keys[i], hash->values[i]);
    }
    hash_destroy(hash);
    *hash = new_hash;
    return true;
}

MINICCDEF bool hash_set(Hash *hash, uint64_t key, uint64_t value)
{
    if (hash_full(hash))
        return false;

    uint32_t index = key % hash->capacity;
    while (hash->keys[index] != key && hash->keys[index] != HASH_UNUSED)
        index = (index + 1) % hash->capacity;

    if (hash->keys[index] == HASH_UNUSED)
        hash->count++;
    hash->keys[index] = key;
    hash->values[index] = value;

    return true;
}

MINICCDEF uint64_t hash_get(const Hash *hash, uint64_t key, uint64_t invalid_value)
{
    uint32_t index = key % hash->capacity;
    uint32_t start = index;
    while (hash->keys[index] != key && hash->keys[index] != HASH_UNUSED) {
        index = (index + 1) % hash->capacity;
        if (index == start)
            return invalid_value;
    }
    return hash->keys[index] == HASH_UNUSED ? invalid_value : hash->values[index];
}

MINICCDEF void hash_clear(Hash *hash)
{
    memset(hash->keys, 0xff, sizeof(*hash->keys) * hash->capacity);
    array_reset(hash->keys);
    array_reset(hash->values);
    hash->count = 0;
}

#define FALLTHROUGH __attribute__((fallthrough))

MINICCDEF uint64_t murmurhash64(const void *key, size_t len, uint64_t seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;
    uint64_t h = seed ^ (len * m);
    const uint8_t *data = (const uint8_t *)key;
    const uint8_t *end = data + (len - (len & 7));

    while (data != end) {
        uint64_t k = *(uint64_t *)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h ^= k;
        h *= m;
        data += 8;
    }

    switch (len & 7) {
    case 7: h ^= (uint64_t)data[6] << 48; FALLTHROUGH;
    case 6: h ^= (uint64_t)data[5] << 40; FALLTHROUGH;
    case 5: h ^= (uint64_t)data[4] << 32; FALLTHROUGH;
    case 4: h ^= (uint64_t)data[3] << 24; FALLTHROUGH;
    case 3: h ^= (uint64_t)data[2] << 16; FALLTHROUGH;
    case 2: h ^= (uint64_t)data[1] << 8;  FALLTHROUGH;
    case 1: h ^= (uint64_t)data[0];
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

#endif // MINICC_IMPLEMENTATION
