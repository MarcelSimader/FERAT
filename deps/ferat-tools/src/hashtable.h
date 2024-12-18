// Author: Marcel Simader (marcel0simader@gmail.com)
// Date: 19.03.2023
// (c) Marcel Simader 2023

#include "common.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef SATIATE_HASH_TABLE_INCLUDED
#define SATIATE_HASH_TABLE_INCLUDED

#define HASHTABLE_DEFAULT_NUM_SLOTS (1 << 12)
#define HASHTABLE_LOAD_FACTOR_LIMIT (0.8)
#define HASHTABLE_GROWTH_FACTOR     (2)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Inline Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline bool __attribute__((always_inline, unused, pure))
cmp_identity(uint64_t k, uint64_t l) {
    return k == l;
}

static inline uint64_t __attribute__((always_inline, unused, pure))
hash_fnv1a(uint64_t v) {
    // We use the 32-bit FNV prime, with the 1a variant of the algorithm. Since we only
    // have one data qword, we can save the loop.
    return (0x811C9DC5 ^ v) * 0x01000193;
}

static inline uint64_t __attribute__((always_inline, unused, pure))
hash_symmetric_adrian(size_t n, uint32_t const *const ptr) {
    uint64_t tmp, s = 0, p = 1, x = 0;
    for (size_t i = 0; i < n; ++i) {
        tmp = (uint64_t)(ptr[i]);
        s += tmp;
        // XXX: Always avoid even numbers
        p *= (tmp | 1);
        x ^= tmp;
    }
    // We use some large prime number for this step.
    tmp = (((s ^ p) << 1) * 0xC96C5795D7870F42) - x;
    return (uint64_t)tmp;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef uint64_t elem_t;
typedef uint32_t flags_t;

/** @brief A generic hash table.
 * @note By default, the hash is @b NOT calculated automatically. Each key needs to be
 * hashed manually!
 */
typedef struct HashTable {
    size_t num_slots;               ///< The number of slots in total.
    size_t num_stored;              ///< The number of elements stored in the slots.
    bool (*cmp_fn)(elem_t, elem_t); ///< An equality comparison function.
    elem_t *buffer;                 ///< Internal slots buffer.
    flags_t *flags;                 ///< Internal flag buffer.
} HashTable;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HashTable *
ht_new(size_t num_slots);

void
ht_free(HashTable *ht);

void
ht_clear(HashTable *ht);

void
ht_insert(HashTable *ht, elem_t key, elem_t value);

Result
ht_get(HashTable *ht, elem_t key);

Result
ht_remove(HashTable *ht, elem_t key);

void
ht_print(HashTable *ht, char const *const prefix);

#endif
