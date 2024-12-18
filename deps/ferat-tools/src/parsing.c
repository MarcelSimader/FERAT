/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 19.12.2023 */
/* (c) Marcel Simader 2023, Johannes Kepler Universität Linz */

#include "common.h"

#include "arraylist.h"
#include "parsing.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#define ARRAYLIST_WORD_DEFAULT_CAPACITY     (1 << 5)
#define ARRAYLIST_VAR_LIST_DEFAULT_CAPACITY (1 << 5)
#define ARRAYLIST_LIT_LIST_DEFAULT_CAPACITY (1 << 4)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Parsing ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** @brief Returns a static string for each ParseState.
 */
char const *
parse_state_name(ParseState state) {
    switch (state) {
    case PARSE_STATE_NONE: return "State_None";
    // QBF
    case PARSE_STATE_PROBLEM: return "State_Problem";
    case PARSE_STATE_QUANTIFIER: return "State_Quantifier";
    case PARSE_STATE_CLAUSE: return "State_Clause";
    // CNF Expansion
    case PARSE_STATE_COMMENT: return "State_Comment";
    case PARSE_STATE_PLAIN_COMMENT: return "State_Plain-Comment";
    case PARSE_STATE_MAPPING_COMMENT: return "State_Mapping-Comment";
    default: assert(false); exit(EXIT_FAILURE);
    }
}

/** @brief Prints the given warning in "printf format" to stdout.
 * @param parser A Parser struct
 */
void
parse_warning(Parser const *const parser, char const *fmt, ...) {
    if (parser->silent) return;
    va_list ap;
    va_start(ap, fmt);
    COMMENT("[Parser warning %u:%d] ", parser->line, parser->col);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

/** @brief Prints the given error in "printf format" to stderr, and aborts execution.
 * @param parser A Parser struct
 * @param exit_code The exit code with which the program aborts
 */
void
fatal_parse_error(Parser const *const parser, unsigned int exit_code, char const *fmt,
                  ...) {
    va_list ap;
    va_start(ap, fmt);
    ERR_COMMENT("[Parser error %u:%d] ", parser->line, parser->col);
    vfprintf(stderr, fmt, ap);
    ERR_COMMENT("[Parser error %u:%d] FATAL with code %u\n", parser->line, parser->col,
                exit_code);
    va_end(ap);
    exit(exit_code);
}

/** @brief Reads a single character into the Parser.
 */
void
read_one_char(Parser *const parser) {
    assert(parser != NULL);
    assert(parser->stream != NULL);
    parser->prev = parser->la;
    if (parser->prev == '\n') {
        parser->col = 0;
        parser->line++;
    }
    parser->col++;
    gzread(parser->stream, &parser->la, 1);
    if (gzeof(parser->stream)) {
        parser->eof = true;
    }
}

/** @brief Skips all non-newline white space (space, tab, v-tab, carriage return) ahead.
 * @returns the number of skipped characters
 */
uint32_t
skip_white(Parser *const parser) {
    assert(parser != NULL);
    uint32_t num_read = 0;
    while (parser->la == ' ' || parser->la == '\t' || parser->la == '\v'
           || parser->la == '\r') {
        if (parser->eof) break;
        read_one_char(parser);
        num_read++;
    }
    return num_read;
}

/** @brief Reads in an entire word, until a delimiter is hit (space, tab, v-tab, carriage
 * return, new line)
 * @returns a pointer to the word without delimiters as NULL-terminated string
 */
char *
expect_word(Parser *const parser) {
    assert(parser != NULL);
    skip_white(parser);
    ArrayList_uint8_t *al = al8_new(ARRAYLIST_WORD_DEFAULT_CAPACITY);
    while (parser->la != ' ' && parser->la != '\t' && parser->la != '\v'
           && parser->la != '\r' && parser->la != '\n') {
        if (parser->eof) break;
        al = al8_append(al, parser->la);
        read_one_char(parser);
    }
    char *word = al8_to_str(al);
    al8_free(al);
    return word;
}

/** @brief Reads in a number in decimal.
 * @param expect_positive when true, negative numbers will abort the execution
 * @returns the number as signed, 64 bit integer
 */
int64_t
expect_number(Parser *const parser, bool expect_positive) {
    assert(parser != NULL);
    skip_white(parser);
    uint32_t num = 0;
    bool is_neg = (parser->la == '-');
    if (is_neg) {
        if (expect_positive)
            fatal_parse_error(parser, EXIT_PARSING_FAILURE,
                              "Expected a positive number, but received '-'\n");
        read_one_char(parser);
    }
    while (!parser->eof) {
        switch (parser->la) {
        case '0': num = (num * 10) + 0; break;
        case '1': num = (num * 10) + 1; break;
        case '2': num = (num * 10) + 2; break;
        case '3': num = (num * 10) + 3; break;
        case '4': num = (num * 10) + 4; break;
        case '5': num = (num * 10) + 5; break;
        case '6': num = (num * 10) + 6; break;
        case '7': num = (num * 10) + 7; break;
        case '8': num = (num * 10) + 8; break;
        case '9': num = (num * 10) + 9; break;
        default: goto expect_number_end_loop;
        }
        read_one_char(parser);
    }
expect_number_end_loop:
    return is_neg ? -((int64_t)num) : num;
}

/** @brief Reads in a number, and crashes if the number does not match the given literal.
 * @param literal the literal to check against
 */
int64_t
expect_number_literal(Parser *const parser, int64_t literal) {
    assert(parser != NULL);
    int64_t num = expect_number(parser, false);
    if (num != literal)
        fatal_parse_error(parser, EXIT_PARSING_FAILURE, "Expected %ld, received %ld\n",
                          literal, num);
    return num;
}

/** @brief Reads in a number, which is expected to be positive, and returns it as
 * variable, with relevant bound checks.
 * @param accept_zero when this is false, a 0 will abort the execution
 * @returns the variable
 */
Variable
expect_variable(Parser *const parser, bool accept_zero) {
    assert(parser != NULL);
    int64_t var = expect_number(parser, true);
    assert((var >= VARIABLE_MIN) || ((var == 0) && accept_zero));
    assert(var <= VARIABLE_MAX);
    return (Variable)var;
}

/** @brief Reads in a number and returns it as literal, with relevant bound checks.
 * @param accept_zero when this is false, a 0 will abort the execution
 * @returns the literal
 */
Literal
expect_literal(Parser *const parser, bool accept_zero) {
    assert(parser != NULL);
    int64_t num = expect_number(parser, false);
    assert(num <= INT32_MAX);
    assert(num >= INT32_MIN);
    Literal lit = SIGNED_LIT2LIT(((int32_t)num));
    assert((LIT2VAR(lit) >= VARIABLE_MIN) || ((LIT2VAR(lit) == 0) && accept_zero));
    assert(LIT2VAR(lit) <= VARIABLE_MAX);
    return lit;
}

/** @brief Reads in a list of variables, using expect_variable(). The list is terminated
 * when a @c 0 literal, a newline, or the end of file is reached. When @c 0 is found,
 * it is not included in the result.
 * @returns a pointer to an ArrayList_Variable_t of variables
 */
ArrayList_Variable_t *
expect_variable_list(Parser *const parser) {
    assert(parser != NULL);
    ArrayList_Variable_t *al = alvar_new(ARRAYLIST_VAR_LIST_DEFAULT_CAPACITY);
    bool got_zero = false;
    Variable var;
    while (!parser->eof && parser->la != '\n') {
        var = expect_variable(parser, true);
        if (var == 0) {
            // Terminate list, do not make a node for 0
            got_zero = true;
            break;
        }
        al = alvar_append(al, var);
    }
    if (!got_zero) parse_warning(parser, "Expected '0' delimiter, not %u\n", var);
    return al;
}

/** @brief Reads in a list of literals, using expect_literal(). The list is terminated
 * when a @c 0 literal, a newline, or the end of file is reached. When @c 0 is found,
 * it is not included in the result.
 * @returns a pointer to an ArrayList_Literal_t of literals
 */
ArrayList_Literal_t *
expect_literal_list(Parser *const parser) {
    assert(parser != NULL);
    ArrayList_Literal_t *al = allit_new(ARRAYLIST_LIT_LIST_DEFAULT_CAPACITY);
    bool got_zero = false;
    Literal lit;
    while (!parser->eof && parser->la != '\n') {
        lit = expect_literal(parser, true);
        if (lit == 0) {
            // Terminate list, do not make a node for 0
            got_zero = true;
            break;
        }
        al = allit_append(al, lit);
    }
    if (!got_zero)
        parse_warning(parser, "Expected '0' delimiter, not " LIT_FMT "\n",
                      LIT_FMT_ARGS(lit));
    return al;
}

/** @brief Handles reading in, and skipping newlines. This is used to conveniently handle
 * states in a line-based syntax.
 * @returns @c true if a newline was found, @c false otherwise
 */
bool
handle_newline(Parser *const parser) {
    assert(parser != NULL);
    skip_white(parser);
    if (parser->la != '\n') return false;
    read_one_char(parser);
    parser->state = PARSE_STATE_NONE;
    return true;
}
