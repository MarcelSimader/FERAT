/* Author: Marcel Simader (marcel0simader@gmail.com) */
/* Date: 19.03.2023 */
/* (c) Marcel Simader 2023 */

#include "common.h"

#include "hashtable.h"

#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Private Macros ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// THe number of bytes required to store 'cap' number of elements.
#define ELEM_SIZE(cap) ((cap) * sizeof(elem_t) * 2)
// THe number of bytes required to store 'cap' number of flags.
#define FLAG_SIZE(cap) \
    ((1 + ((((cap) > 0) ? ((cap) - 1) : 0) / (sizeof(flags_t) * 8))) * sizeof(flags_t))

// Key offset for some index in the buffer.
#define KEY(idx)      ((idx) << 1)
// Value offset for some index in the buffer.
#define VAL(idx)      (KEY(idx) + 1)
// Index from a given hash.
#define IDX(ht, hash) ((hash) % (ht)->num_slots)
// Flag WORD position from an index.
#define FLAGWORD(idx) ((idx) / (sizeof(flags_t) * 8))
// Flag BIT position from an index.
#define FLAGBIT(idx)  ((idx) % (sizeof(flags_t) * 8))

// Whether the slot at given index is occupied.
#define OCCUPIED_(flags, idx)  (!!((((flags)[FLAGWORD(idx)]) >> FLAGBIT(idx)) & 1))
#define OCCUPIED(ht, idx)      (OCCUPIED_((ht)->flags, idx))
// Sets the occupied flag to @c true for given index.
#define MARK_OCCUPIED(ht, idx) ((ht)->flags[FLAGWORD(idx)] |= (1 << FLAGBIT(idx)))
// Inverse of #MARK_OCCUPIED.
#define MARK_EMPTY(ht, idx)    ((ht)->flags[FLAGWORD(idx)] &= ~(1 << FLAGBIT(idx)))

// Load factor of a HashTable.
#define HTLOADFACTOR(ht) (((float)(ht)->num_stored) / (ht)->num_slots)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Private Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Computes the index from a given key using the open addressing strategy. The index is
// set by reference, and the function returns 'true' if the key was found, and 'false'
// otherwise.
static inline bool
hashidx_openaddr(HashTable *ht, elem_t key, size_t *index) {
    assert(index != NULL);
    *index = IDX(ht, key);
    // Try at most 'num_slots' times to increment the hash index. NOTE: 'i' is unused in
    // the loop body!
    size_t i = ht->num_slots;
    do {
        if (LIKELY(OCCUPIED(ht, *index)
                   && ht->cmp_fn((elem_t)ht->buffer[KEY(*index)], key)))
            return true;
        *index = IDX(ht, *index + 1);
    } while (i-- > 0);
    // we did not find the key
    *index = 0;
    return false;
}

// Changes the number of slots for the HashTable while rehashing all of its keys.
static inline void
chsize(HashTable *ht, size_t new_num_slots) {
    size_t old_num_slots = ht->num_slots;
    // Allocate new buffers, ...
    elem_t *new_buffer = calloc(ELEM_SIZE(1), new_num_slots);
    assert(new_buffer != NULL);
    flags_t *new_flags = calloc(FLAG_SIZE(1), new_num_slots);
    assert(new_flags != NULL);
    // but keep references to the old ones
    elem_t *old_buffer = ht->buffer;
    flags_t *old_flags = ht->flags;
    // Now, update the hashtable buffers and reinsert the old elements
    ht->buffer = new_buffer;
    ht->flags = new_flags;
    ht->num_slots = new_num_slots;
    ht->num_stored = 0;
    for (size_t i = 0; i < old_num_slots; ++i) {
        if (OCCUPIED_(old_flags, i))
            ht_insert(ht, old_buffer[KEY(i)], old_buffer[VAL(i)]);
    }
    free(old_buffer);
    free(old_flags);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Hash Table ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** @brief Creates a new, empty HashTable.
 */
HashTable *
ht_new(size_t num_slots) {
    HashTable *ht = malloc(sizeof(HashTable));
    assert(ht != NULL);
    ht->num_slots = num_slots;
    ht->num_stored = 0u;
    ht->cmp_fn = cmp_identity;
    ht->buffer = calloc(ELEM_SIZE(1), num_slots);
    assert(ht->buffer != NULL);
    ht->flags = calloc(FLAG_SIZE(1), num_slots);
    assert(ht->flags != NULL);
    return ht;
}

/** @brief Frees a HashTable and its underlying buffers.
 */
void
ht_free(HashTable *ht) {
    assert(ht != NULL);
    if (ht->buffer != NULL) free(ht->buffer);
    if (ht->flags != NULL) free(ht->flags);
    free(ht);
}

/** @brief Clears a HashTable without changing the number of @em total slots.
 */
void
ht_clear(HashTable *ht) {
    assert(ht != NULL);
    memset(ht->buffer, 0, ELEM_SIZE(ht->num_slots));
    memset(ht->flags, 0, FLAG_SIZE(ht->num_slots));
    ht->num_stored = 0;
}

/** @brief Inserts a key and a value into the HashTable. <b>Keys that already exist are
 * overwritten!</b>
 */
void
ht_insert(HashTable *ht, elem_t key, elem_t value) {
    assert(ht != NULL);
    if (UNLIKELY(HTLOADFACTOR(ht) >= HASHTABLE_LOAD_FACTOR_LIMIT)) {
        size_t prev_num_slots = ht->num_slots;
        chsize(ht, prev_num_slots * HASHTABLE_GROWTH_FACTOR);
    }
    size_t idx = IDX(ht, key);
    while (OCCUPIED(ht, idx)) idx = IDX(ht, idx + 1);
    MARK_OCCUPIED(ht, idx);
    ht->buffer[KEY(idx)] = key;
    ht->buffer[VAL(idx)] = value;
    ht->num_stored += 1;
}

/** @brief Retrieves a value from a given key in this HashTable.
 * @returns a Result object, with the Result.ok and Result.value fields set, if was found
 */
Result
ht_get(HashTable *ht, elem_t key) {
    assert(ht != NULL);
    size_t idx;
    bool found_key = hashidx_openaddr(ht, key, &idx);
    Result result = { .value.uimax = 0, .ok = false };
    if (found_key && OCCUPIED(ht, idx)) {
        result.value.uimax = (uintmax_t)ht->buffer[VAL(idx)];
        result.ok = true;
    }
    return result;
}

/** @brief Removes a value from a given key in this HashTable.
 * @returns a Result object, with the Result.ok and Result.value fields set, if the key
 * was found
 */
Result
ht_remove(HashTable *ht, elem_t key) {
    assert(ht != NULL);
    size_t idx;
    bool found_key = hashidx_openaddr(ht, key, &idx);
    Result result = { .value.uimax = 0, .ok = false };
    if (found_key && OCCUPIED(ht, idx)) {
        MARK_EMPTY(ht, idx);
        result.value.uimax = (uintmax_t)ht->buffer[VAL(idx)];
        result.ok = true;
        ht->num_stored -= 1;
    }
    return result;
}

/** @brief Prints the contents of this HashTable to the console. Debug.
 */
void
ht_print(HashTable *ht, char const *const prefix) {
    COMMENT("%sHashTable <%luB + %luB> {", prefix, ELEM_SIZE(ht->num_slots),
            FLAG_SIZE(ht->num_slots));
    if (ht->num_stored < 1) {
        printf("/}");
        return;
    }
    printf("\n");
    for (size_t i = 0; i < ht->num_slots; ++i) {
        COMMENT("  %s%p:", prefix, ht->buffer + KEY(i));
        if (OCCUPIED(ht, i))
            printf(" %zu -> %zu\n", ht->buffer[KEY(i)], ht->buffer[VAL(i)]);
        else
            printf(" -\n");
    }
    COMMENT("%s}", prefix);
}
