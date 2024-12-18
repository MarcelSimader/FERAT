// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 23.11.2021
// (c) Marcel Simader 2021, Johannes Kepler Universität Linz

#ifndef LIBSE_TESTRUNNER
#define LIBSE_TESTRUNNER

#include "ansi.h"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ~~~~~~~~~~~~~~~~~~~~ TEST INFRASTRUCTURE ~~~~~~~~~~~~~~~~~~~~

#define MAX_TESTS 128

#define EXIT_PASS     0
#define EXIT_FAIL     1
#define EXIT_UNTESTED 34

#define PASS_TEXT     ANSI_GREEN "PASS" ANSI_NORMAL
#define FAIL_TEXT     ANSI_RED "FAIL" ANSI_NORMAL
#define UNTESTED_TEXT ANSI_YELLOW "UNTESTED" ANSI_NORMAL

// Format strings
#define FMT_TEST_LISTING   "  (%zu) Test " ANSI_WHITE "'%s'" ANSI_NORMAL ": "
#define FMT_EXCEEDED_TESTS "Exceeded maximum number of tests: %d"
#define FMT_TEST_CASE      "\n    --------\n    %s\n"
// Assetion format strings
#define FMT_ASSERT_FAIL    "      - Expected truthy value\n        Received %lld ('%s')"
#define FMT_ASSERT_N_FAIL  "      - Expected falsy value\n        Received %lld ('%s')"
#define FMT_ASSERT_EQ_FAIL "      - Expected %lld ('%s')\n        Received %lld ('%s')"
#define FMT_ASSERT_NEQ_FAIL \
    "      - Expected other than %lld ('%s')\n        Received %lld ('%s')"
#define FMT_ASSERT_STREQ_FAIL "      - Expected '%s'\n        Recieved '%s'\n"
#define FMT_ASSERT_MEMEQ_FAIL                                                           \
    "      - Expected memory at location %p ('%s') to equal %p ('%s') for %lld ('%s') " \
    "bytes"

// Test cases
static int (*__tests[MAX_TESTS])(void);
static char *__testnames[MAX_TESTS];
static size_t __num = 0;
static void (*__before)(void) = NULL;
static void (*__after)(void) = NULL;

#define addbefore(func)  \
    do {                 \
        __before = func; \
    } while (0)
#define addafter(func)  \
    do {                \
        __after = func; \
    } while (0)

#define addtest(func, name)                        \
    do {                                           \
        if (__num > MAX_TESTS) {                   \
            printf(FMT_EXCEEDED_TESTS, MAX_TESTS); \
            fflush(stdout);                        \
        }                                          \
        __tests[__num] = func;                     \
        __testnames[__num++] = name;               \
    } while (0)

#define runtests(name)                                                \
    do {                                                              \
        printf("Running '%s':\n", name);                              \
        int result = 0;                                               \
        for (size_t i = 0; i < __num; ++i) {                          \
            if (__before != NULL) {                                   \
                __before();                                           \
            }                                                         \
            char *subname = __testnames[i];                           \
            printf(FMT_TEST_LISTING, i, subname);                     \
            fflush(stdout);                                           \
            int subresult = __tests[i]();                             \
            switch (subresult) {                                      \
            case EXIT_PASS: printf("%s\n", PASS_TEXT); break;         \
            case EXIT_FAIL: printf(FMT_TEST_CASE, FAIL_TEXT); break;  \
            case EXIT_UNTESTED: printf("%s\n", UNTESTED_TEXT); break; \
            }                                                         \
            fflush(stdout);                                           \
            if (__after != NULL) {                                    \
                __after();                                            \
            }                                                         \
            result |= (subresult == EXIT_UNTESTED) ? 0 : subresult;   \
        }                                                             \
        return result;                                                \
    } while (0)

// ~~~~~~~~~~~~~~~~~~~~ Exits ~~~~~~~~~~~~~~~~~~~~

// Posix 1003.3 compliant
#define pass()            \
    do {                  \
        return EXIT_PASS; \
    } while (0)
#define fail()            \
    do {                  \
        return EXIT_FAIL; \
    } while (0)
#define untested()            \
    do {                      \
        return EXIT_UNTESTED; \
    } while (0)

// ~~~~~~~~~~~~~~~~~~~~ ASSERTS ~~~~~~~~~~~~~~~~~~~~

#define _assert_msg(fmt, ...)                                                     \
    fprintf(stderr, "\n    %s:%d(%s)\n" fmt "    ", __FILE__, __LINE__, __func__, \
            __VA_ARGS__);                                                         \
    fflush(stderr);                                                               \
    fail();

#define _assert(o, fmt, ...)          \
    if (!(o)) {                       \
        _assert_msg(fmt, __VA_ARGS__) \
    }

#define assert(o)                              \
    do {                                       \
        long long llo = (long long)(o);        \
        _assert(llo, FMT_ASSERT_FAIL, llo, #o) \
    } while (0)

#define assertn(o)                                               \
    do {                                                         \
        long long llo = (long long)(o);                          \
        _assert(!(llo), FMT_ASSERT_N_FAIL, (long long)(llo), #o) \
    } while (0)

#define asserteq(o1, o2)                                                \
    do {                                                                \
        long long llo1 = (long long)(o1), llo2 = (long long)(o2);       \
        _assert(llo1 == llo2, FMT_ASSERT_EQ_FAIL, llo1, #o1, llo2, #o2) \
    } while (0)

#define assertneq(o1, o2)                                                \
    do {                                                                 \
        long long llo1 = (long long)(o1), llo2 = (long long)(o2);        \
        _assert(llo1 != llo2, FMT_ASSERT_NEQ_FAIL, llo1, #o1, llo2, #o2) \
    } while (0)

#define assertnull(o) asserteq(NULL, o)

#define assertnnull(o) assertneq(NULL, o)

#define assertstreq(o1, o2)                                         \
    do {                                                            \
        char *s1 = (o1), *s2 = (o2);                                \
        _assert(strcmp(s1, s2) == 0, FMT_ASSERT_STREQ_FAIL, s1, s2) \
    } while (0)

#define assertmemeq(exp_size, exp_ptr, ptr)                                              \
    _assert(memcmp((ptr), (exp_ptr), (exp_size)) == 0, FMT_ASSERT_MEMEQ_FAIL, ptr, #ptr, \
            exp_ptr, #exp_ptr, exp_size, #exp_size)

#define assertarrayeq(exp_size, exp_arr, cmp, arr)                           \
    do {                                                                     \
        asserteq(exp_size, sizeof(arr) / sizeof(*(arr)));                    \
        for (size_t i = 0; i < (exp_size); ++i) cmp((exp_arr)[i], (arr)[i]); \
    } while (0)

#define assertarrayliteq(exp_size, exp_arr, arr) \
    assertarrayeq(exp_size, exp_arr, asserteq, arr)

#endif

