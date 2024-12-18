/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 19.12.2023 */
/* (c) Marcel Simader 2023, Johannes Kepler Universität Linz */

#include "qbf.h"

#include "arraylist.h"
#include "hashtable.h"
#include "parsing.h"
#include "sorting.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#define ARRAYLIST_PREFIX_DEFAULT_CAP (1 << 7)
#define ARRAYLIST_MATRIX_DEFAULT_CAP (1 << 15)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Private Wrapper Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// This is a wrapper to get the quantifier ordering from a literal, given some context
// (here, of type QBF) so it can be applied as partial.
uint32_t
qbf_get_quant_ordering_from_index_partial(void *const qbf, uint32_t const lit) {
    Result const result = ht_get(((QBF *)qbf)->prefix_mapping, hash_fnv1a(LIT2VAR(lit)));
    // When a variable is free, we assume it is existentially-quantified at the very
    // beginning
    if (!result.ok) {
        qbf_warn_free(qbf, LIT2VAR(lit));
        return 0;
    }
    return ((Quantifier *)result.value.ptr)->ordering;
}

// // This is a wrapper for the original function, so we can store values with clauses as
// // keys in a hash table.
// bool
// wrapped_qbf_clauses_match(uint64_t qbf_clause_0, uint64_t qbf_clause_1) {
//     return qbf_clauses_match(((struct Wrapped_QBFClause *)qbf_clause_0)->clause,
//                              ((struct Wrapped_QBFClause *)qbf_clause_1)->clause);
// }

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ QBF Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

QBF *
qbf_new(void) {
    QBF *qbf = malloc(sizeof(QBF));
    assert(qbf != NULL);
    qbf->max_var = 0;
    qbf->num_alternations = 0;
    qbf->prefix_mapping = ht_new(HASHTABLE_DEFAULT_NUM_SLOTS);
    qbf->warned_free = ht_new(HASHTABLE_DEFAULT_NUM_SLOTS);
    qbf->prefix = alptr_new(ARRAYLIST_PREFIX_DEFAULT_CAP);
    qbf->matrix = alptr_new(ARRAYLIST_MATRIX_DEFAULT_CAP);
    // qbf->num_existential_cache = ht_new(HASHTABLE_DEFAULT_NUM_SLOTS);
    return qbf;
}

void
qbf_free(QBF *const qbf) {
    assert(qbf != NULL);
    // NOTE: The Quantifier structs are taken care of by the array list below
    if (qbf->prefix_mapping != NULL) ht_free(qbf->prefix_mapping);
    if (qbf->warned_free != NULL) ht_free(qbf->warned_free);
    if (qbf->prefix != NULL) {
        for (size_t i = 0; i < qbf->prefix->size; ++i) free(alptr_get(qbf->prefix, i));
        alptr_free(qbf->prefix);
    }
    if (qbf->matrix != NULL) {
        for (size_t i = 0; i < qbf->matrix->size; ++i) free(alptr_get(qbf->matrix, i));
        alptr_free(qbf->matrix);
    }
    // if (qbf->num_existential_cache != NULL) ht_free(qbf->num_existential_cache);

    free(qbf);
}

void
qbf_warn_free(QBF *const qbf, Variable var) {
    if (ht_get(qbf->warned_free, hash_fnv1a(var)).ok) return;
    WARN_COMMENT("Variable %u not found in QBF prefix, assuming existentially "
                 "quantified\n",
                 var);
    ht_insert(qbf->warned_free, hash_fnv1a(var), true);
}

void
qbf_print(QBF const *const qbf) {
    assert(qbf != NULL);
    assert(qbf->matrix != NULL);
    assert(qbf->prefix != NULL);
    COMMENT("QBF {\n");
    COMMENT("  max_var=%u\n", qbf->max_var);
    COMMENT("  num_alternations=%u\n", qbf->num_alternations);
    COMMENT("  prefix:\n");
    Quantifier *quant;
    for (size_t i = 0; i < qbf->prefix->size; ++i) {
        quant = alptr_get(qbf->prefix, i);
        COMMENT("    %s", (quant->type == QUANT_TYPE_EXISTENTIAL) ? "e" : "a");
        for (size_t j = 0; j < quant->num_vars; ++j) printf(" %d", quant->variables[j]);
        printf("\n");
    }
    COMMENT("  matrix:\n");
    QBFClause *qbf_clause;
    for (size_t i = 0; i < qbf->matrix->size; ++i) {
        qbf_clause = alptr_get(qbf->matrix, i);
        COMMENT("    ");
        for (size_t j = 0; j < qbf_clause->num_literals; ++j) {
            if (j != 0) printf(" ");
            printf(LIT_FMT, LIT_FMT_ARGS(qbf_clause->lits[j]));
        }
        printf("\n");
    }
    COMMENT("}\n");
}
/** @brief Sorts all clauses given in the QBF formula by their respective quantifier
 * index.
 *
 * @param[out] qbf the resulting QBF sorted in-place
 */
void
qbf_sort_clauses_in_matrix(QBF *const qbf) {
    assert(qbf != NULL);
    assert(qbf->prefix_mapping != NULL);
    assert(qbf->prefix != NULL);
    assert(qbf->matrix != NULL);
    ArrayList_uint32_t *stack = al32_new(2048);
    struct Partial partial
        = { .fn = qbf_get_quant_ordering_from_index_partial, .context = qbf };
    for (size_t i = 0; i < qbf->matrix->size; ++i) {
        QBFClause *const clause = alptr_get(qbf->matrix, i);
        iterative_inplace_quickort(&stack, &partial, clause->lits, clause->num_literals);
    }
    al32_free(stack);
}

// /** @brief Compares one QBFClause to another. Note, that the clauses <b>MUST BE
//  * SORTED</b>.
//  */
// bool
// qbf_clauses_match(QBFClause const *const qbf_clause_0,
//                   QBFClause const *const qbf_clause_1) {
//     if (qbf_clause_0 == qbf_clause_1)
//         return true;
//     else if (qbf_clause_0 == NULL || qbf_clause_1 == NULL)
//         return false;
//     else if (qbf_clause_0->num_literals != qbf_clause_1->num_literals)
//         return false;
//     // XXX: We expect the clauses to be sorted, so we can compare in (sub)linear time.
//     for (size_t i = 0; i < qbf_clause_0->num_literals; ++i)
//         if (allit_get(qbf_clause_0->lits, i) != allit_get(qbf_clause_1->lits, i))
//             return false;
//     return true;
// }

/** @brief Parses a QBF stream.
 *
 * @note The resulting QBF struct will contain QBFClause structs which all have
 * sorted arrays. This is important for performance in check.c.
 *
 * @param stream the (gZip) stream to read from
 * @param qbf the QBF struct to fill
 * @param silent if @c true, do not emit any warning output, fatal errors still produce
 *     output
 * @param[out] qbf a pointer to an existing QBF struct to write into
 */
void
qbf_parse(gzFile stream, QBF *const qbf, bool silent) {
    assert(stream != NULL);
    assert(qbf != NULL);
    assert(qbf->prefix_mapping != NULL);
    assert(qbf->prefix != NULL);
    assert(qbf->matrix != NULL);

    // Parsing state-machine
    Parser parser = { .state = PARSE_STATE_NONE,
                      .line = 1,
                      .col = 1,
                      .eof = false,
                      .silent = silent,
                      .la = '\0',
                      .prev = '\0',
                      .stream = stream };
    read_one_char(&parser);
    bool parsed_problem = false;
    bool saw_quantifier = false, last_is_existential = false, is_existential = false;
    uint32_t p_max_var = 0, p_num_clauses = 0;
    uint32_t quantifier_index = 0;
    uint32_t num_clauses = 0;
    u_char not_exists_ch;
    char *word;
    while (!parser.eof) {
        // This is identical for all cases, since this is a line-based
        // format
        if (handle_newline(&parser)) continue;
        switch (parser.state) {
        case PARSE_STATE_NONE:
            switch (parser.la) {
            case 'p':
                parser.state = PARSE_STATE_PROBLEM;
                skip_white(&parser);
                read_one_char(&parser);
                break;
            case 'c':
                parser.state = PARSE_STATE_COMMENT;
                skip_white(&parser);
                read_one_char(&parser);
                break;
            case 'e':
                parser.state = PARSE_STATE_QUANTIFIER;
                not_exists_ch = parser.la;
                skip_white(&parser);
                read_one_char(&parser);
                is_existential = true;
                break;
            case 'a':
                parser.state = PARSE_STATE_QUANTIFIER;
                not_exists_ch = parser.la;
                skip_white(&parser);
                read_one_char(&parser);
                is_existential = false;
                break;
            default: parser.state = PARSE_STATE_CLAUSE; break;
            }
            break;

        case PARSE_STATE_PROBLEM:;
            if (parsed_problem)
                fatal_parse_error(&parser, EXIT_PARSING_FAILURE,
                                  "Found second, or duplicate 'p ...' header\n");

            word = expect_word(&parser);
            if (strcmp(word, "cnf") != 0)
                fatal_parse_error(&parser, EXIT_PARSING_FAILURE,
                                  "Only 'cnf' option is supported, not '%s'\n", word);

            p_max_var = expect_number(&parser, true);
            p_num_clauses = expect_number(&parser, true);

            expect_number_literal(&parser, 0);

            free(word);
            parsed_problem = true;
            parser.state = PARSE_STATE_NONE;
            break;

        case PARSE_STATE_COMMENT:
            // Parse until the end of the line
            skip_white(&parser);
            read_one_char(&parser);
            break;

        case PARSE_STATE_CLAUSE:;
            ArrayList_Literal_t *const clause_literals = expect_literal_list(&parser);

            QBFClause *qbf_clause
                = malloc(sizeof(QBFClause) + sizeof(Literal) * clause_literals->size);
            assert(qbf_clause != NULL);
            qbf_clause->num_literals = clause_literals->size;
            Literal lit;
            Variable var;
            for (size_t i = 0; i < clause_literals->size; ++i) {
                lit = allit_get(clause_literals, i);
                if ((var = LIT2VAR(lit)) > qbf->max_var) qbf->max_var = var;
                qbf_clause->lits[i] = lit;
            }

            num_clauses += 1;
            qbf->matrix = alptr_append(qbf->matrix, qbf_clause);

            parser.state = PARSE_STATE_NONE;
            break;

        case PARSE_STATE_QUANTIFIER:;
            if (!is_existential && not_exists_ch != 'a')
                fatal_parse_error(&parser, EXIT_PARSING_FAILURE,
                                  "Expected 'a' or 'e' for quantifiers, not '%c'\n",
                                  not_exists_ch);

            ArrayList_Variable_t *const quantifier_variables
                = expect_variable_list(&parser);

            Quantifier *quantifier = malloc(
                sizeof(Quantifier) + sizeof(Variable) * quantifier_variables->size);
            assert(quantifier != NULL);
            quantifier->type
                = is_existential ? QUANT_TYPE_EXISTENTIAL : QUANT_TYPE_UNIVERSAL;
            quantifier->num_vars = quantifier_variables->size;
            quantifier->ordering = quantifier_index++;

            Variable qbf_var;
            size_t i;
            for (i = 0; i < quantifier_variables->size;) {
                qbf_var = alvar_get(quantifier_variables, i);
                // Check if each variable was already found in the prefix
                if (ht_get(qbf->prefix_mapping, hash_fnv1a(qbf_var)).ok) {
                    parse_warning(&parser,
                                  "Found duplicate variable %u in prefix, keeping its"
                                  " first appearance\n",
                                  qbf_var);
                    quantifier->num_vars -= 1;
                    if (quantifier->num_vars == 0) break;
                } else {
                    // NOTE: ONLY increment i here, otherwise we end up with holes in the
                    //       array!
                    quantifier->variables[i++] = qbf_var;
                    if (qbf_var > qbf->max_var) qbf->max_var = qbf_var;
                    ht_insert(qbf->prefix_mapping, hash_fnv1a(qbf_var),
                              (uint64_t)quantifier);
                }
            }

            // It could occurr that a duplicated quantifier ends up with 0 variables. In
            // this case we just delete it.
            if (quantifier->num_vars == 0) {
                free(quantifier);
            } else {
                if (saw_quantifier) {
                    if (last_is_existential != is_existential)
                        qbf->num_alternations += 1;
                    else
                        parse_warning(&parser, "Two quantifiers of same type in a row");
                }
                saw_quantifier = true;
                last_is_existential = is_existential;
                qbf->prefix = alptr_append(qbf->prefix, quantifier);
            }

            alvar_free(quantifier_variables);
            parser.state = PARSE_STATE_NONE;
            break;

        default:
            fatal_parse_error(&parser, EXIT_PARSING_FAILURE,
                              "Reached illegal state: %s (%u)\n",
                              parse_state_name(parser.state), parser.state);
        }
    }

    if (!parsed_problem)
        fatal_parse_error(&parser, EXIT_PARSING_FAILURE,
                          "Expected a 'p ...' header but reached EOF\n");
    if (num_clauses != p_num_clauses)
        parse_warning(&parser, "Expected %u clause[s], but received %u\n", p_num_clauses,
                      num_clauses);
    if (qbf->max_var != p_max_var) {
        parse_warning(&parser,
                      "Expected maximum variable to be %u, but maximum variable is "
                      "actually %u in quantifiers and clauses\n",
                      p_max_var, qbf->max_var);
        // Always just pick bigger one, I guess
        if (p_max_var > qbf->max_var) qbf->max_var = p_max_var;
    }
}
