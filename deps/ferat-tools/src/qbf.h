// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 19.12.2023
// (c) Marcel Simader 2023, Johannes Kepler Universität Linz

#include "common.h"

#include "arraylist.h"
#include "hashtable.h"

#include <stdint.h>
#include <zlib.h>

#ifndef FORALL_EXP_RAT_QBF_INCLUDED
#define FORALL_EXP_RAT_QBF_INCLUDED

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** @brief A basic quantifier type.
 */
typedef enum QuantType {
    QUANT_TYPE_NONE = 0,
    QUANT_TYPE_EXISTENTIAL = 1,
    QUANT_TYPE_UNIVERSAL = 2,
} QuantType;

/** @brief A Quantifier holds its type, order in the prefix, and one or more variables
 * names bound to it.
 */
typedef struct Quantifier {
    QuantType type;
    uint32_t num_vars;
    uint32_t ordering;
    // Variable-sized struct.
    Variable variables[0];
} Quantifier;

/** @brief A QBF clause is the same as an ExpClause, but clearly denoted as distinct type.
 * It is essentially a shallow array wrapper for some number of literals.
 */
typedef struct QBFClause {
    uint32_t num_literals;
    // Variable-sized struct.
    Literal lits[0];
} QBFClause;

/** @brief A QBF formula.
 *
 * A QBF struct holds an array list of prefix Quantifier pointers, an array list of
 * Clause pointers, and a HashTable, which can be indexed using the prefix ::Variable%s
 * to get pointers to the Quantifier struct they are bound in.
 */
typedef struct QBF {
    uint32_t max_var;
    uint32_t num_alternations;
    HashTable *prefix_mapping; ///< @brief HashTable of (QBF) ::Variable (key), and
                               ///< Quantifier * (value)
    HashTable *warned_free;    ///< @brief HashTable of (QBF) ::Variable (key), and
                               ///< bool (value)
    ArrayList_ptr_t *prefix;   ///< @brief ArrayList of Quantifier *
    ArrayList_ptr_t *matrix;   ///< @brief ArrayList of QBFClause *
    // HashTable *num_existential_cache; ///< @brief Cache for the number of existential
    //                                   ///< literals in a given clause
} QBF;

// This type is a wrapper, so we can store values with clauses as keys in a hash table.
struct Wrapped_QBFClause {
    QBFClause const *clause;
    uint32_t value;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QBF *
qbf_new(void);

void
qbf_free(QBF *const qbf);

void
qbf_print(QBF const *const qbf);

void
qbf_warn_free(QBF *const qbf, Variable var);

void
qbf_sort_clauses_in_matrix(QBF *const qbf);

bool
qbf_clauses_match(QBFClause const *const qbf_clause_0,
                  QBFClause const *const qbf_clause_1) __attribute__((pure));

void
qbf_parse(gzFile stream, QBF *const qbf, bool silent);

void
qbf_mark_checked(QBF *const qbf, uint32_t clause_index);

#endif
