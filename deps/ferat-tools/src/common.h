// Author: Marcel Simader (marcel0simader@gmail.com)
// Date: 19.03.2023
// (c) Marcel Simader 2023

#ifndef FORALL_EXP_RAT_COMMON_INCLUDED
#define FORALL_EXP_RAT_COMMON_INCLUDED

// This is dumb, but it's a workaround for Clangd not reporting macros in the preamble.
static int x __attribute__((unused));

#include <stdbool.h>
#include <stdint.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Macros ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define DIMACS_COMMENT_PREFIX "c "
#define DIMACS_RESULT_PREFIX  "s "

#ifndef VERBOSE
// @c VERBOSE is defined as @c 1 when compiling for special debug purposes.
#define VERBOSE (0)
#endif

// Prints verbose information, when VERBOSE is defined, and set to @c 1.
#if VERBOSE
#define INFO(fmt, ...) fprintf(stdout, DIMACS_COMMENT_PREFIX fmt, ##__VA_ARGS__)
#else
#define INFO(...) ((void)0)
#endif
#define COMMENT(fmt, ...) fprintf(stdout, DIMACS_COMMENT_PREFIX fmt, ##__VA_ARGS__)
#define WARN_COMMENT(fmt, ...) \
    fprintf(stdout, DIMACS_COMMENT_PREFIX "[Warning] " fmt, ##__VA_ARGS__)
#define ERR_COMMENT(fmt, ...) fprintf(stderr, DIMACS_COMMENT_PREFIX fmt, ##__VA_ARGS__)
#define RESULT(fmt, ...)      fprintf(stdout, DIMACS_RESULT_PREFIX fmt, ##__VA_ARGS__)
#define FLUSH()         \
    do {                \
        fflush(stdout); \
        fflush(stderr); \
    } while (0)

#ifdef __has_builtin
#define LIKELY(x)   (__builtin_expect(!!(x), true))
#define UNLIKELY(x) (__builtin_expect(!!(x), false))
#else
#warning "No builtins available"
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

// EXIT_SUCCESS (0)
// EXIT_FAILURE (1)
#define EXIT_CLI_FAILURE     (2)
#define EXIT_VERIFIED        (10)
#define EXIT_NOT_VERIFIED    (20)
#define EXIT_PARSING_FAILURE (80)

#define INIT_TIME()               \
    useconds_t __start_time = 0u; \
    struct timeval __tval
#define START_TIME(name)                        \
    useconds_t name = 0u;                       \
    do {                                        \
        gettimeofday(&__tval, NULL);            \
        __start_time = TIMEVAL_TO_USEC(__tval); \
    } while (0)
#define END_TIME(name)                                 \
    do {                                               \
        gettimeofday(&__tval, NULL);                   \
        name = TIMEVAL_TO_USEC(__tval) - __start_time; \
    } while (0)
#define TIMEVAL_TO_USEC(t)   ((useconds_t)(1000000u * (t).tv_sec + (t).tv_usec))
#define USEC_TO_HUM_RDBL_FMT "%u us  (%.3f %s)"
#define USEC_TO_HUM_RDBL_FMT_ARGS(us)              \
    (us),                                          \
        (((us) >= 60 * 1e6) ? ((1e-6 / 60) * (us)) \
         : ((us) >= 9e5)    ? (1e-6 * (us))        \
         : ((us) >= 9e2)    ? (1e-3 * (us))        \
                            : (us)),                  \
        (((us) >= 60 * 1e6) ? "m"                  \
         : ((us) >= 9e5)    ? "s"                  \
         : ((us) >= 9e2)    ? "ms"                 \
                            : "us")

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Taken from Satiate Solver Project ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** Our variables and literals are based on KISSAT's source code, thanks Armin!
 * Variables are simply 32-bit unsigned integers with a maximum value of
 * @c 0x7FFFFFFF (aka INT32_MAX -- signed!).
 */
typedef uint32_t Variable;

/** Literals are also 32-bit unsigned integers, but shifted one bit to the left.
 * This leaves the first bit to indicate polarity. This is why plain variables,
 * despite being unsigned, have a signed maximum value.
 */
typedef uint32_t Literal;

/* @brief Converts an external signed literal (-3, 5, etc.) to an internal literal (all
 * positive with first LSB set to polarity).
 */
#define SIGNED_LIT2LIT(s_lit) (VAR2LIT(abs(s_lit), ((uint32_t)(s_lit) >> 31)))

/** @brief Returns the sign bit of a literal, aka 0 for positive, and 1 for negative
 * polarity.
 */
#define LITSIGNBIT(lit) ((lit) & 1)

/** @brief Converts a literal to its corresponding variable.
 */
#define LIT2VAR(lit) ((lit) >> 1)

/** @brief Converts a variable to its index representation. This is a sequence like:
 *     0 := 1
 *     1 := 2
 *     2 := 3
 *       ...
 *     i := var - 1
 */
#define VAR2IDX(var) ((var) - 1)

/** @brief Converts a variable to a literal by adding on a sign bit.
 */
#define VAR2LIT(var, signb) ((((Variable)(var)) << 1) | (signb))

/** @brief Negates a literal.
 */
#define LIT_NEG(lit) ((lit) ^ 1)

/** @brief Changes the sign of a literal to the given sign bit.
 */
#define LIT_ABS(lit, signb) (VAR2LIT(LIT2VAR(lit), signb))

/** @brief The format string to print out our internal literals as human-readable signed
 * literals. For format string arguments see LIT_FMT_ARGS.
 */
#define LIT_FMT "%s%u"

/** @brief The macro turning a literal into the two format string arguments needed by
 * LIT_FMT. For format string see LIT_FMT.
 */
#define LIT_FMT_ARGS(lit) (LITSIGNBIT(lit) ? "-" : ""), LIT2VAR(lit)

#define VARIABLE_MIN (1)
#define VARIABLE_MAX (INT32_MAX)
#define LITERAL_MIN  (VAR2LIT(VARIABLE_MAX, 1))
#define LITERAL_MAX  (VAR2LIT(VARIABLE_MAX, 0))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOTE: I thought this was part of 'stdint.h'? Maybe not on all systems.
#ifndef u_char
typedef unsigned char u_char;
#endif

/** @brief A struct of a result integer, and a flag showing whether whatever operation
 * returned it was successful.
 */
typedef struct Result {
    union Value {
        bool b;
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        intmax_t imax;
        uint8_t ui8;
        uint16_t ui16;
        uint32_t ui32;
        uint64_t ui64;
        uintmax_t uimax;
        char *str;
        void *ptr;
    } value;
    bool ok;
} Result;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Simple Static Memory ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** @brief Static memory containing 8 spaces encoded as C-string. */
static char const *const STR_8SPACE = "        ";
/** @brief Static memory containing 6 spaces encoded as C-string. */
static char const *const STR_6SPACE = STR_8SPACE + 2;
/** @brief Static memory containing 4 spaces encoded as C-string. */
static char const *const STR_4SPACE = STR_6SPACE + 2;
/** @brief Static memory containing 2 spaces encoded as C-string. */
static char const *const STR_2SPACE = STR_4SPACE + 2;
/** @brief Static memory containing an empty C-string. */
static char const *const STR_0SPACE = STR_2SPACE + 2;

#endif
