// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 19.03.2024
// (c) Marcel Simader 2024, Johannes Kepler Universität Linz

#ifndef FORALL_EXP_RAT_TEST_COMMON_INCLUDED
#define FORALL_EXP_RAT_TEST_COMMON_INCLUDED

#include "../../src/arraylist.h"
#include "../../src/hashtable.h"

// This is dumb, but it's a workaround for Clangd not reporting macros in the preamble.
static int x __attribute__((unused));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Macros ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define DECLARE_GZ(i)       \
    static FILE *fout_##i;  \
    static char *fname_##i; \
    static gzFile gz_##i

#define TMP_WRITE(i, content)            \
    fname_##i = tmpnam(NULL);            \
    fout_##i = fopen(fname_##i, "w");    \
    do {                                 \
        fprintf(fout_##i, content);      \
        fflush(fout_##i);                \
        gz_##i = gzopen(fname_##i, "r"); \
    } while (0)

#define GZ(i) gz_##i

#define GZCLOSE(i)    \
    fclose(fout_##i); \
    gzclose(gz_##i)

#define ARRAYLIST_EQUAL(AT, al, expected_size, expected_arr) \
    do {                                                     \
        asserteq((expected_size), (al)->size);               \
        for (size_t i = 0; i < (expected_size); ++i)         \
            asserteq((expected_arr)[i], AT##_get((al), i));  \
    } while (0)

#define HASHTABLE_SUBSET(ht, expected_size, expected_keys, expected_values, cmp) \
    do {                                                                         \
        assert(expected_size <= ht->num_stored);                                 \
        Result result;                                                           \
        for (size_t i = 0; i < (expected_size); ++i) {                           \
            result = ht_get((ht), (expected_keys)[i]);                           \
            assert(result.ok);                                                   \
            if (cmp((expected_values)[i], result.value) == EXIT_FAIL) fail();    \
        }                                                                        \
    } while (0)

#endif
