/* Author: Marcel Simader (marcel0simader@gmail.com) */
/* Date: 03.01.2024 */
/* (c) Marcel Simader 2024 */

#include "check.h"

#include "arraylist.h"
#include "expansion.h"
#include "parsing.h"
#include "qbf.h"
#include "sorting.h"

#include <stdio.h>

#define ARRAYLIST_CHECK_RESULT_DEFAULT_CAP (1 << 7)
#define ARRAYLIST_V_DEFAULT_CAP            (1 << 5)
#define ARRAYLIST_U_DEFAULT_CAP            (1 << 3)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Debug Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** @brief Prints a ::Literal array.
 */
void
ferat_print_lits(Literal const *const lits, size_t num_lits) {
    assert(lits != NULL);
    printf("Literals: ");
    for (size_t i = 0; i < num_lits; ++i) {
        if (i != 0) printf(" ");
        printf(LIT_FMT, LIT_FMT_ARGS(lits[i]));
    }
    printf("\n");
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Private Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** @brief Returns a static string description for each FERATCheckResultType.
 */
char const *
ferat_check_result_type_description(FERATCheckResultType type) {
    switch (type) {
    case FERAT_CHECK_RESULT_INCORRECT_LITERALS:
        return "No QBF clause matches the literals found";
    case FERAT_CHECK_RESULT_INCORRECT_ANNOTATION:
        return "Annotations in expansion are incorrect";
    default: assert(false); exit(EXIT_FAILURE);
    }
}

/** @brief Adds a result wiht a given #FERATCheckResultType and clause index to the
 * FERATCheckResult struct.
 * @param[out] result a pointer to an existing FERATCheckResult struct
 */
void
ferat_insert_check_result(FERATCheckResult *const result, FERATCheckResultType type,
                          uint32_t clause_index) {
    assert(result != NULL);
    result->num_results += 1;
    result->types = al8_append(result->types, type);
    result->clause_indices = al32_append(result->clause_indices, clause_index);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Private Checking Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** @brief Checks that the given Expansion clause ::Literal%s could have originated from
 * the given QBF clause.
 *
 * This essentially checks the following:
 *
 *   1. Each expansion Literal needs to exist in the given QBF clause
 *   2. The QBF clause must contain no extra existential literals (i.e. literals not found
 *      in the stripped expansion clause.)
 *
 * @param qbf_lits the QBF literals to compare against
 * @param num_qbf_lits the number of QBF literals
 * @param exp_lits the Expansion literals to strip and check
 * @param num_exp_lits the number of Expansion literals
 * @param qbf the QBF formula
 * @param expansion the Expansion formula
 * @returns @c true if the conditions described above are met
 */
bool
ferat_test_expansion_origin_in_QBF(QBFClause const *const qbf_clause,
                                   ExpClause const *const exp_clause, QBF *const qbf,
                                   Expansion const *const expansion) {
    assert(qbf_clause != NULL);
    assert(exp_clause != NULL);
    assert(qbf != NULL);
    assert(expansion != NULL);
    // First, make sure that all expansion literals exist as existentially-quantified
    // literals in the given QBF clause.
    size_t i, j;
    Literal exp_lit, qbf_lit;
    Result result;
    ExpVarMapping *mapping;
    for (i = 0; i < exp_clause->num_literals; ++i) {
        exp_lit = exp_clause->lits[i];
        result = ht_get(expansion->exp_var_mappings, hash_fnv1a(LIT2VAR(exp_lit)));
        // Mapping needs to exist
        assert(result.ok);
        mapping = result.value.ptr;
        qbf_lit = VAR2LIT(mapping->qbf_var, LITSIGNBIT(exp_lit));
        // Check, that the translated literal exists in the given QBF
        for (j = 0; j < qbf_clause->num_literals; ++j)
            if (qbf_lit == qbf_clause->lits[j]) goto check_next_lit;
        // Not found
        return false;
    check_next_lit:;
    }
    // Now, we need to make sure that there aren't any more existentially-quantified
    // literals in the QBF clause
    uint32_t num_exist_qbf_lits = 0;
    Quantifier *quant;
    Variable var;
    for (i = 0; i < qbf_clause->num_literals; ++i) {
        var = LIT2VAR(qbf_clause->lits[i]);
        result = ht_get(qbf->prefix_mapping, hash_fnv1a(var));
        if (result.ok) {
            // Literal needs to be quantified
            quant = result.value.ptr;
            if (quant->type == QUANT_TYPE_EXISTENTIAL) num_exist_qbf_lits += 1;
        } else {
            qbf_warn_free(qbf, var);
            num_exist_qbf_lits += 1;
        }
    }
    return exp_clause->num_literals == num_exist_qbf_lits;
}

/** @brief Checks that the mapping found for the given Expansion ::Literal%s is possible
 * to reproduce given the QBF literals of some clause, and the prefix of the QBF.
 *
 * @note This check only make sense, if ferat_test_expansion_origin_in_QBF() returns @c
 * true
 *
 * TODO(Marcel): Detailed description.
 *
 * @param qbf_lits the QBF literals to compare against
 * @param num_qbf_lits the number of QBF literals
 * @param exp_lits the Expansion literals to check the annotations of
 * @param num_exp_lits the number of Expansion literals
 * @param qbf the QBF formula
 * @param expansion the Expansion formula
 * @returns @c true if the conditions described above are met
 */
bool
ferat_check_annotations_against_expansion(QBFClause const *const qbf_clause,
                                          ExpClause const *const exp_clause,
                                          ArrayList_Literal_t **U,
                                          ArrayList_Literal_t **V, QBF *const qbf,
                                          Expansion const *const expansion) {
    assert(qbf_clause != NULL);
    assert(exp_clause != NULL);
    assert(U != NULL);
    assert(V != NULL);
    assert(*U != NULL);
    assert(*V != NULL);
    assert(qbf != NULL);
    assert(expansion != NULL);
    // We iteratively go over each literal in the expansion clause, and handle all
    // quantifiers leading up to the translated literal's index in the QBF prefix
    // There are two sets of literals we keep track of:
    //    - 'U', literals that appear in the given QBF clause
    //    - 'V', literals that appear in the QBF formula, but not the given clause
    // WARNING: These are only allocated once, so this all runs much faster... but we
    //          still need to clear them here.
    (*U)->size = 0;
    (*V)->size = 0;
    int64_t idx;
    uint32_t last_quant_idx = 0, num_universal_vars_so_far = 0, last_V_size,
             curr_quant_idx;
    size_t i, j, k, l;
    Variable quant_qbf_var;
    Literal exp_lit, lit;
    Result result;
    ExpVarMapping *exp_var_mapping;
    Quantifier *quant;
    for (i = 0; i < exp_clause->num_literals; ++i) {
        last_V_size = (*V)->size;
        // First, get the variable mapping
        exp_lit = exp_clause->lits[i];
        result = ht_get(expansion->exp_var_mappings, hash_fnv1a(LIT2VAR(exp_lit)));
        // Mapping has to exist
        assert(result.ok);
        exp_var_mapping = result.value.ptr;
        // Now, get index of the QBF quantifier
        result = ht_get(qbf->prefix_mapping, hash_fnv1a(exp_var_mapping->qbf_var));
        if (!result.ok) {
            // When a variable is free, we assume it is existentially-quantified at the
            // very beginning, and can just check if the annotation is empty. That is the
            // only condition, given we know it appears in the QBF clause.
            qbf_warn_free(qbf, exp_var_mapping->qbf_var);
            if (exp_var_mapping->num_annotation_literals != 0) return false;
            continue;
        }
        // QBF variable must be quantified
        quant = result.value.ptr;
        curr_quant_idx = quant->ordering;
        // Now, handle each quantifier between the indices, so we can iteratively build up
        // our test sets
        for (j = last_quant_idx; j < curr_quant_idx; ++j) {
            quant = alptr_get(qbf->prefix, j);
            // Make sure the type is a universal quantifier
            if (quant->type != QUANT_TYPE_UNIVERSAL) continue;
            // Go over each variable in the quantifier, and add them to the test sets
            // conditionally:
            //   - If the variable appears in the QBF clause, add its negation
            //   - If the variable does not appear in the QBF clause, add both polarities
            for (k = 0; k < quant->num_vars; ++k) {
                quant_qbf_var = quant->variables[k];
                num_universal_vars_so_far++;
                for (l = 0; l < qbf_clause->num_literals; ++l) {
                    if (quant_qbf_var == LIT2VAR(qbf_clause->lits[l]))
                        goto quant_var_in_qbf;
                }
                // Quantifier variable is not in the QBF, add both polarities.
                *V = allit_insert_sorted(*V, VAR2LIT(quant_qbf_var, false));
                *V = allit_insert_sorted(*V, VAR2LIT(quant_qbf_var, true));
                goto next_quant_var;
            quant_var_in_qbf:;
                // Quantified variable is in QBF, add negated polarity. We can use the
                // value of 'l', which now points to the literal
                *U = allit_insert_sorted(*U, LIT_NEG(qbf_clause->lits[l]));
            next_quant_var:;
            }
        }
        // we need to make sure there are exactly as many annotation universals as there
        // are universals to the left of this var(l) in the prefix.
        if (exp_var_mapping->num_annotation_literals != num_universal_vars_so_far)
            return false;
        // Now, make sure that the annotation is a subset of the union of U and V.
        for (j = 0; j < exp_var_mapping->num_annotation_literals; ++j) {
            lit = exp_var_mapping->annotation[j];
            // See if we can find the annotation literal in V...
            if (allit_binary_search_contains(*V, lit)) goto check_next_annotation_literal;
            // ... or in U.
            if (allit_binary_search_contains(*U, lit)) goto check_next_annotation_literal;
            // If all checks fail, we know to stop right away!
            return false;
        check_next_annotation_literal:;
        }
        // Since we now know, that we bound the free variables in V to the ones found in
        // the annotation, we can remove all negations of annotation literals from V.
        // The same would go for U, but U only contains the correct polarity anyway.
        for (j = 0; j < exp_var_mapping->num_annotation_literals; ++j) {
            lit = LIT_NEG(exp_var_mapping->annotation[j]);
            // TODO(Marcel): It should be possible to use `last_V_size` to search here,
            //               if we implement an array list view on the data. Then, nothing
            //               is copied. It should be fairly easy, too.
            while ((idx = allit_binary_search_index(*V, lit)) >= 0) allit_remove(*V, idx);
        }
        last_quant_idx = curr_quant_idx;
    }
    return true;
}

/* ~~~~~~~~~~~~~~~~~~~~ Main Checking Functions ~~~~~~~~~~~~~~~~~~~~ */

/** @brief Validates, that the given ExpClause found in Expansion can be obtained from a
 * QBFClause in QBF, and that the annotations of each ::Literal in ExpClause are correct.
 *
 * We do the following:
 *
 *   1. Validate, that the given Expansion literals can be obtained from the
 *      existentially-quantified variables in any clause from the QBF, and then
 *   2. validate, that the QBF clause we found has universally-quantified variables
 *      corresponding to the annotations of the Expansion literals with correct phase.
 *
 * @param exp_clause the ExpClause to check
 * @param exp_clause_index the position of ExpClause in the Expansion formula's clauses
 * @param expansion the Expansion formula
 * @param qbf the QBF formula
 * @param[out] result the results of this check
 */
void
ferat_check_expansion_clause(ExpClause const *const exp_clause, uint32_t exp_clause_index,
                             ArrayList_Literal_t **U, ArrayList_Literal_t **V,
                             Expansion *const expansion, QBF *const qbf,
                             FERATCheckResult *const result) {
    assert(exp_clause != NULL);
    assert(U != NULL);
    assert(V != NULL);
    assert(*U != NULL);
    assert(*V != NULL);
    assert(expansion != NULL);
    assert(qbf != NULL);
    assert(result != NULL);
    bool found_matching_clause = false;
    bool has_origins = (expansion->clause_origins != NULL);
    // This looks a bit confusing, but in essence, we just want to iterate over all QBF
    // clauses in case we DON'T have an origin mapping, but if we do have one, we only run
    // this loop once and ignore 'i' completely
    uint32_t matrix_idx;
    QBFClause *qbf_clause;
    for (size_t i = 0; i < qbf->matrix->size; ++i) {
    loop_over_matrix_start:;
        if (has_origins) {
            if (i >= expansion->clause_origins->size) {
                parse_warning(
                    &expansion->parser,
                    "Expected %d clauses in clause origin mapping comment ('c o "
                    "1 4 2 2 ... 0'), but yielded %d clauses so far. Falling "
                    "back to iterative search mode, this might be quite slow.\n",
                    expansion->clause_origins->size, exp_clause_index);
                has_origins = false;
                // Just free the list here, we don't need it anymore
                al32_free(expansion->clause_origins);
                expansion->clause_origins = NULL;
                goto loop_over_matrix_start;
            }
            matrix_idx = al32_get(expansion->clause_origins, exp_clause_index);
            if (matrix_idx >= qbf->matrix->size)
                fatal_parse_error(&expansion->parser, EXIT_PARSING_FAILURE,
                                  "Given origin index %u is invalid, as there are only "
                                  "%u clauses in the QBF matrix.\n",
                                  matrix_idx + 1, qbf->matrix->size);
            qbf_clause = alptr_get(qbf->matrix, matrix_idx);
        } else {
            qbf_clause = alptr_get(qbf->matrix, i);
        }
        // If we find a matching QBF clause, that has correct annotations, we can stop the
        // check for this expansion clause immediately
        if (ferat_test_expansion_origin_in_QBF(qbf_clause, exp_clause, qbf, expansion)) {
            found_matching_clause = true;
            if (ferat_check_annotations_against_expansion(qbf_clause, exp_clause, U, V,
                                                          qbf, expansion))
                return;
        }
        // See comment just before the loop
        if (has_origins) break;
    }
    // If we found a clause that could work, we show incorrect mappings, but if we
    // never even found a matching clause, we say there is no possible expansion
    if (found_matching_clause)

        ferat_insert_check_result(result, FERAT_CHECK_RESULT_INCORRECT_ANNOTATION,
                                  exp_clause_index);
    else
        ferat_insert_check_result(result, FERAT_CHECK_RESULT_INCORRECT_LITERALS,
                                  exp_clause_index);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ FERAT Check ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** @brief Creates a new, empty FERATCheckResult struct.
 */
FERATCheckResult *
ferat_check_result_new(void) {
    FERATCheckResult *result = malloc(sizeof(FERATCheckResult));
    assert(result != NULL);
    result->num_results = 0;
    result->types = al8_new(ARRAYLIST_CHECK_RESULT_DEFAULT_CAP);
    result->clause_indices = al32_new(ARRAYLIST_CHECK_RESULT_DEFAULT_CAP);
    return result;
}

/** @brief Prints the contents of the FERATCheckResult struct to @c stdout.
 */
void
ferat_check_result_print(FERATCheckResult const *const result) {
    char const *const inconsistencies
        = (result->num_results == 1) ? "inconsistency" : "inconsistencies";
    COMMENT("Found %u %s:\n", result->num_results, inconsistencies);
    FERATCheckResultType type;
    for (size_t i = 0; i < result->num_results; ++i) {
        type = al8_get(result->types, i);
        COMMENT("  %4lu. %s", i + 1, ferat_check_result_type_description(type));
        printf(" in expansion clause %u", 1 + al32_get(result->clause_indices, i));
        printf("\n");
    }
}

/** @brief Frees the FERATCheckResult struct and its underlying data structures.
 */
void
ferat_check_result_free(FERATCheckResult *const result) {
    assert(result != NULL);
    if (result->types != NULL) al8_free(result->types);
    if (result->clause_indices != NULL) al32_free(result->clause_indices);
    free(result);
}

/** @brief Checks the validity of some expansion step in a solver, given the Expansion
 * formula, and the QBF formula.
 *
 * @param[out] result the results of this check
 * @param qbf the QBF formula
 * @param expansion the Expansion formula
 * @returns @c true, if all clauses were found to be valid, otherwise, see result
 */
bool
ferat_check(FERATCheckResult *const result, QBF *const qbf, Expansion *const expansion) {
    assert(expansion != NULL);
    assert(qbf != NULL);
    assert(result != NULL);
    // Go over each expansion clause, and test it. This fills up the 'result' struct
    ExpClause *exp_clause;
    uint32_t i;
    ArrayList_Literal_t *U = allit_new(ARRAYLIST_U_DEFAULT_CAP),
                        *V = allit_new(ARRAYLIST_V_DEFAULT_CAP);
    ArrayList_uint32_t *stack = al32_new(ARRAYLIST_DEFAULT_CAP);
    // Free ExpClause directly after using it, since we do this check online
    for (i = 0; (exp_clause = expansion_yield_clause(expansion)) != NULL; ++i) {
        iterative_inplace_quickort(&stack, &sort_identity_partial, exp_clause->lits,
                                   exp_clause->num_literals);
        ferat_check_expansion_clause(exp_clause, i, &U, &V, expansion, qbf, result);
        free(exp_clause);
    }
    if (i != expansion->p_num_clauses)
        parse_warning(&expansion->parser, "Expected %u clause[s], but received %u\n",
                      expansion->p_num_clauses, i);
    al32_free(stack);
    allit_free(U);
    allit_free(V);
    return (result->num_results == 0);
}
