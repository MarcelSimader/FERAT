// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 23.07.2024
// (c) Marcel Simader 2024, Johannes Kepler Universität Linz

#include "arraylist.h"

#include <stdint.h>

#ifndef FORALL_EXP_RAT_SORTING_INCLUDED
#define FORALL_EXP_RAT_SORTING_INCLUDED

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct Partial {
    uint32_t (*fn)(void *const context, uint32_t value);
    void *context;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void
iterative_inplace_quickort(ArrayList_uint32_t **stack,
                           struct Partial const *const partial, uint32_t *const values,
                           size_t size);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Convenience Definitions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline uint32_t __attribute__((always_inline, unused))
sort_identity(void *const context, uint32_t value) {
    (void)context;
    return value;
}

static struct Partial const sort_identity_partial
    = { .fn = sort_identity, .context = NULL };

#endif
