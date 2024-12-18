/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 23.07.2024 */
/* (c) Marcel Simader 2024, Johannes Kepler Universität Linz */

#include "common.h"

#include "arraylist.h"
#include "sorting.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Private Macros ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define SWAP_PTR_VALUES(tmp, p0, p1) \
    do {                             \
        (tmp) = *(p0);               \
        *(p0) = *(p1);               \
        *(p1) = (tmp);               \
    } while (0)

#define APPLY_PARTIAL(patial, arg) ((partial)->fn((partial)->context, (arg)))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Quicksort Function ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void
iterative_inplace_quickort(ArrayList_uint32_t **stack,
                           struct Partial const *const partial, uint32_t *const values,
                           size_t size) {
    assert(values != NULL);
    if (size < 2) return;
    // Push initial low, and high values onto stack
    *stack = al32_append(*stack, 0);
    *stack = al32_append(*stack, size - 1);
    // As long as we have partitions, continue performing quicksort
    uint32_t low, high, pivot;
    int64_t i;
    size_t j;
    uint32_t tmp;
    while ((*stack)->size > 0) {
        // WARNING: Pop off in reverse order!
        high = al32_pop(*stack);
        low = al32_pop(*stack);
        // Sort given length into two partitions, and get pivot index
        // Get value of middle of partition from directly after its end
        uint32_t central = APPLY_PARTIAL(partial, values[high]);
        // Go over each element and partition it into the two sections
        i = (int64_t)low - 1;
        for (j = low; j < high; ++j) {
            if (APPLY_PARTIAL(partial, values[j]) > central) continue;
            i += 1;
            SWAP_PTR_VALUES(tmp, values + i, values + j);
        }
        // Finally, swap the element after the middle of the partition with the one
        // directly after the end of the partition
        if (UNLIKELY(i + 1 < high)) {
            SWAP_PTR_VALUES(tmp, values + i + 1, values + high);
        }
        pivot = i + 1;
        // Now, push new runlengths onto the stack, if they are not empty
        if (low + 1 < pivot) {
            *stack = al32_append(*stack, low);
            *stack = al32_append(*stack, pivot - 1);
        }
        if (high - 1 > pivot) {
            *stack = al32_append(*stack, pivot + 1);
            *stack = al32_append(*stack, high);
        }
    }
}

