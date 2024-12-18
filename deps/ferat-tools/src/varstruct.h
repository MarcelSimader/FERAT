// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 17.05.2023
// (c) Marcel Simader 2023, Johannes Kepler Universität Linz

#ifndef SATIATE_VARSTRUCT_INCLUDED
#define SATIATE_VARSTRUCT_INCLUDED

#include "common.h"

#include <memory.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Macros ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define VARSTRUCT_DEFAULT_CAP (1 << 3)

/** Defines how the array list should be resized when the capacity is exceeded.
 *  By default, we just 2x the size, as the Java ArrayList does.
 */
#define VARSTRUCT_RESIZE_STRAT(old) ((old) << 1)

/** Changes the capacity of a variable-sized struct.
 *
 * @note Take care to maintain the updated 'struct_ptr' if a size change occurs.
 * @note This assumes the struct looks like this:
 *
 *     struct {
 *         ...
 *
 *         uint32_t size;
 *         uint32_t cap;
 *         E ...[];
 *     }
 *
 *     TODO: See https://stackoverflow.com/a/9575288.
 */
#define VARSTRUCT_CHSIZE(S, E, struct_ptr, elem_ptr, newcap)                             \
    do {                                                                                 \
        uint32_t __oldcap = (struct_ptr)->cap;                                           \
        (struct_ptr)->cap = (newcap);                                                    \
        (struct_ptr) = realloc((struct_ptr), sizeof(S) + sizeof(E) * (struct_ptr)->cap); \
        assert((struct_ptr) != NULL);                                                    \
        /* Fill with 0s in case we increased the size */                                 \
        if (struct_ptr->cap > __oldcap) {                                                \
            memset((elem_ptr) + __oldcap, 0,                                             \
                   sizeof(E) * ((struct_ptr)->cap - __oldcap));                          \
        }                                                                                \
    } while (0)

/** Appends an element to a variable-sized struct.
 *
 * @note Take care to maintain the updated 'struct_ptr' if a size change occurs.
 * @see VARSTRUCT_CHSIZE() for struct shape.
 */
#define VARSTRUCT_APPEND(S, E, struct_ptr, elem_ptr, new_elem)           \
    do {                                                                 \
        /* Change size first to satisfy assertion 'index + 1 <= size' */ \
        uint32_t __index = (struct_ptr)->size;                           \
        (struct_ptr)->size += 1;                                         \
        if (UNLIKELY((struct_ptr)->size > (struct_ptr)->cap)) {          \
            VARSTRUCT_CHSIZE(S, E, (struct_ptr), (elem_ptr),             \
                             VARSTRUCT_RESIZE_STRAT((struct_ptr)->cap)); \
        }                                                                \
        ((E *)(elem_ptr))[__index] = (new_elem);                         \
    } while (0)

/** Inserts an element into a variable-sized struct.
 *
 * @note Take care to maintain the updated 'struct_ptr' if a size change occurs.
 * @see VARSTRUCT_CHSIZE() for struct shape.
 */
#define VARSTRUCT_INSERT(S, E, struct_ptr, elem_ptr, new_elem, index)    \
    do {                                                                 \
        /* Change size first to satisfy assertion 'index + 1 <= size' */ \
        (struct_ptr)->size += 1;                                         \
        if (UNLIKELY((struct_ptr)->size > (struct_ptr)->cap)) {          \
            VARSTRUCT_CHSIZE(S, E, (struct_ptr), (elem_ptr),             \
                             VARSTRUCT_RESIZE_STRAT((struct_ptr)->cap)); \
        }                                                                \
        memmove((elem_ptr) + (index) + 1, (elem_ptr) + (index),          \
                sizeof(E) * ((struct_ptr)->size - (index) - 1));         \
        ((E *)(elem_ptr))[index] = (new_elem);                           \
    } while (0)

/** Removes a chunk of elements at index 'index' to 'index + num' from the
 * variable-sized struct.
 *
 * @note Take care to maintain the updated 'struct_ptr' if a size change occurs.
 * @see VARSTRUCT_CHSIZE() for struct shape.
 */
#define VARSTRUCT_REMOVE(S, E, struct_ptr, elem_ptr, index, num)     \
    do {                                                             \
        memmove((elem_ptr) + (index), (elem_ptr) + (index) + (num),  \
                sizeof(E) * ((struct_ptr)->size - (index) - (num))); \
        (struct_ptr)->size -= (num);                                 \
    } while (0)

#endif
