// Author: Marcel Simader (marcel0simader@gmail.com)
// Date: 10.06.2023
// (c) Marcel Simader 2023

#ifndef SATIATE_ARRAYLIST_INCLUDED
#define SATIATE_ARRAYLIST_INCLUDED

#include "common.h"

#include "varstruct.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Macros ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define ARRAYLIST_DEFAULT_CAP (1 << 5)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define _TYPE_AL(ET, AT)                                          \
    /* An array list is just a variable-sized struct with more */ \
    /* concrete implementation, instead of just using macros.  */ \
    typedef struct AT {                                           \
        /* The logical size. */                                   \
        uint32_t size;                                            \
        /* The real size. */                                      \
        uint32_t cap;                                             \
        /* The stored values. */                                  \
        ET array[0];                                              \
    } AT;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Default Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define _STUB_STATIC_IMPL_AL(ET, AT, methprefix)                                       \
                                                                                       \
    _TYPE_AL(ET, AT)                                                                   \
                                                                                       \
    /** @brief Creates a new array list struct. */                                     \
    static inline AT *methprefix##_new(uint32_t capacity) {                            \
        assert(capacity != 0);                                                         \
        AT *al = malloc(sizeof(AT) + sizeof(ET) * capacity);                           \
        assert(al != NULL);                                                            \
        al->size = 0;                                                                  \
        al->cap = capacity;                                                            \
        return al;                                                                     \
    }                                                                                  \
                                                                                       \
    /** @brief Frees the given array list struct. */                                   \
    static inline void methprefix##_free(AT *al) {                                     \
        assert(al != NULL);                                                            \
        free(al);                                                                      \
    }                                                                                  \
                                                                                       \
    static inline void methprefix##_set(AT *const al, ET element, uint32_t index) {    \
        assert(al != NULL);                                                            \
        assert(index < al->size);                                                      \
        al->array[index] = element;                                                    \
    }                                                                                  \
                                                                                       \
    static inline ET methprefix##_get(AT const *const al, uint32_t index) {            \
        assert(al != NULL);                                                            \
        assert(index < al->size);                                                      \
        return al->array[index];                                                       \
    }                                                                                  \
                                                                                       \
    /** @brief Appends an element to the array list. */                                \
    static inline __attribute__((warn_unused_result))                                  \
    AT *methprefix##_append(AT *al, ET element) {                                      \
        assert(al != NULL);                                                            \
        VARSTRUCT_APPEND(AT, ET, al, al->array, element);                              \
        return al;                                                                     \
    }                                                                                  \
                                                                                       \
    /** @brief Inserts an element into the array list. */                              \
    static inline __attribute__((warn_unused_result))                                  \
    AT *methprefix##_insert(AT *al, ET element, uint32_t index) {                      \
        assert(al != NULL);                                                            \
        VARSTRUCT_INSERT(AT, ET, al, al->array, element, index);                       \
        return al;                                                                     \
    }                                                                                  \
                                                                                       \
    __attribute__((warn_unused_result))                                                \
    AT *methprefix##_insert_sorted(AT *al, ET element);                                \
                                                                                       \
    /** @brief Retrieves the top-most element from the array list and removes it. */   \
    static inline ET methprefix##_pop(AT *const al) {                                  \
        assert(al != NULL);                                                            \
        assert(al->size > 0);                                                          \
        al->size -= 1;                                                                 \
        return al->array[al->size];                                                    \
    }                                                                                  \
                                                                                       \
    /** @brief Removes one element of the array list at the given index. */            \
    static inline void methprefix##_remove(AT *const al, uint32_t index) {             \
        assert(al != NULL);                                                            \
        assert(index < al->size);                                                      \
        VARSTRUCT_REMOVE(AT, ET, al, al->array, index, 1);                             \
    }                                                                                  \
                                                                                       \
    /** @brief Removes multiple elements of the array list at the given */             \
    /** index at once. */                                                              \
    static inline void methprefix##_remove_chunk(AT *const al, uint32_t index,         \
                                                 uint32_t size) {                      \
        assert(al != NULL);                                                            \
        assert(index < al->size);                                                      \
        VARSTRUCT_REMOVE(AT, ET, al, al->array, index, size);                          \
    }                                                                                  \
                                                                                       \
    /** @brief Returns the index of the given element in the array list, or '-1' if */ \
    /** it is not found. */                                                            \
    int64_t methprefix##_index(AT const *al, ET element);                              \
                                                                                       \
    int64_t methprefix##_binary_search_index(AT const *al, ET element);                \
                                                                                       \
    /** @brief Returns 'true' if the given element is contained in the array list. */  \
    bool methprefix##_contains(AT const *al, ET element);                              \
                                                                                       \
    bool methprefix##_binary_search_contains(AT const *al, ET element);                \
                                                                                       \
    /** @brief Prints the array list to <stdout>. */                                   \
    void methprefix##_print(AT const *al, char const *const prefix);

/*
 * [>* @brief Returns the largest value in the list. <]                               \
 * ET methprefix##_max(AT const *const al);                                           \
 *                                                                                    \
 * [>* @brief Returns the smallest value in the list. <]                              \
 * ET methprefix##_min(AT const *const al);                                           \
 *                                                                                    \
 */

// ~~~~~~~~~~~~~~~~~~~~ Basic Types ~~~~~~~~~~~~~~~~~~~~

_STUB_STATIC_IMPL_AL(uint8_t, ArrayList_uint8_t, al8)
_STUB_STATIC_IMPL_AL(Variable, ArrayList_Variable_t, alvar)
_STUB_STATIC_IMPL_AL(Literal, ArrayList_Literal_t, allit)
_STUB_STATIC_IMPL_AL(uint32_t, ArrayList_uint32_t, al32)
_STUB_STATIC_IMPL_AL(void *, ArrayList_ptr_t, alptr)

// ~~~~~~~~~~~~~~~~~~~~ Custom Types ~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Special Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *
al8_to_str(ArrayList_uint8_t *al);

#endif
