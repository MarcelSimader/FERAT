// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 19.12.2023
// (c) Marcel Simader 2023, Johannes Kepler Universität Linz

#include "common.h"

#include "arraylist.h"

#include <stdbool.h>
#include <stdint.h>
#include <zlib.h>

#ifndef FORALL_EXP_RAT_PARSING_INCLUDED
#define FORALL_EXP_RAT_PARSING_INCLUDED

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** @brief Various parser states for both QBF and CNF expansion parsing.
 */
typedef enum ParseState {
    PARSE_STATE_NONE = 0,
    // QBF
    PARSE_STATE_PROBLEM = 1,
    PARSE_STATE_COMMENT = 2,
    PARSE_STATE_QUANTIFIER = 3,
    PARSE_STATE_CLAUSE = 4,
    // CNF Expansion
    // PARSE_STATE_COMMENT = 2, reused
    // PARSE_STATE_CLAUSE = 4, reused
    PARSE_STATE_PLAIN_COMMENT = 5,
    PARSE_STATE_MAPPING_COMMENT = 6,
    PARSE_STATE_ORIGIN_COMMENT = 7,
} ParseState;

/** @brief A parser holds a (gzip) stream, an EOF flag, the current line and column, the
 * previous and look-ahead (LL(1)) char, and a ParseState.
 */
typedef struct Parser {
    gzFile stream;
    bool eof, silent;
    uint32_t line, col;
    u_char prev, la;
    ParseState state;
} Parser;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char const *
parse_state_name(ParseState state) __attribute__((returns_nonnull));

void
parse_warning(Parser const *const parser, char const *fmt, ...)
    __attribute__((format(printf, 2, 3)));

void
fatal_parse_error(Parser const *const parser, unsigned int exit_code, char const *fmt,
                  ...) __attribute__((format(printf, 3, 4), noreturn));

void
read_one_char(Parser *const parser);

uint32_t
skip_white(Parser *const parser);

char *
expect_word(Parser *const parser) __attribute__((warn_unused_result, returns_nonnull));

int64_t
expect_number(Parser *const parser, bool expect_positive);

int64_t
expect_number_literal(Parser *const parser, int64_t literal);

Variable
expect_variable(Parser *const parser, bool accept_zero);

Literal
expect_literal(Parser *const parser, bool accept_zero);

ArrayList_Literal_t *
expect_literal_list(Parser *const parser)
    __attribute__((warn_unused_result, returns_nonnull));

ArrayList_Variable_t *
expect_variable_list(Parser *const parser)
    __attribute__((warn_unused_result, returns_nonnull));

bool
handle_newline(Parser *const parser);

#endif
