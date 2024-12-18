// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 19.12.2023
// (c) Marcel Simader 2023, Johannes Kepler Universität Linz

#include "arraylist.h"
#include "hashtable.h"
#include "parsing.h"
#include "qbf.h"

#include <zlib.h>

#ifndef FORALL_EXP_RAT_EXPANSION_INCLUDED
#define FORALL_EXP_RAT_EXPANSION_INCLUDED

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** @brief A variable mapping struct describes a single CNF expansion variable, and its
 * relation to the original QBF. It stores the corresponding QBF variable, and its
 * annotations.
 */
typedef struct ExpVarMapping {
    Variable qbf_var;
    Variable exp_var;
    uint32_t num_annotation_literals;
    // NOTE: We define this array with size 8 to allow instantiation with a C literal for
    //       testing. In the original definition, the annotation array was set to size 0,
    //       which would make it impossible to create a C literal with annotation
    //       literals.
    Literal annotation[8];
} ExpVarMapping;

/** @brief An expansion clause is the same as a QBFClause, but clearly denoted as distinct
 * type. It is essentially a shallow array.
 */
typedef struct ExpClause {
    uint32_t num_literals;
    // Variable-sized struct.
    Literal lits[0];
} ExpClause;

/** @brief A CNF expansion formula.
 *
 * An expansion struct describes the CNF expansion of some original QBF formula. It
 * contians an array list of the expansion variables the formula contains, which also act
 * as keys for the HashTable holding a pointer to each ExpVarMapping, and a BitArray for
 * keeping track of checked clauses.
 *
 * To get more clauses, pass this struct to the expansion_yield_clause() function
 */
typedef struct Expansion {
    Parser parser;                      ///< @brief The Parser state
    uint32_t p_max_var;                 ///< @brief The DIMACS preamble maximum variable
    uint32_t p_num_clauses;             ///< @brief The DIMACS preamble count of clauses
    uint32_t num_clauses_yielded;       ///< @brief The number of clauses yielded so far
    ArrayList_uint32_t *clause_origins; ///< @brief ArrayList of ExpClause origings,
                                        /// where each index in the arraylist is the
                                        /// corresponding expansion clause's index in the
                                        /// QBF
    ArrayList_Variable_t *exp_var_mapping_keys; ///< @brief ArrayList of (Exp) ::Variable
    HashTable *exp_var_mappings; ///< @brief HashTable of (Exp) ::Variable (key),
                                 ///< and ExpVarMapping * (value)
} Expansion;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Inline Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline uint64_t __attribute__((const, always_inline, unused))
hash_clause(uint64_t clause_ptr) {
    QBFClause const *const clause = (QBFClause *)clause_ptr;
    return hash_symmetric_adrian(clause->num_literals, clause->lits);
}

static inline bool __attribute__((const, always_inline, unused))
cmp_clause(uint64_t clause_ptr_0, uint64_t clause_ptr_1) {
    QBFClause const *const clause_0 = (QBFClause *)clause_ptr_0;
    QBFClause const *const clause_1 = (QBFClause *)clause_ptr_1;
    return qbf_clauses_match(clause_0, clause_1);
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Expansion *
expansion_new(void);

void
expansion_free(Expansion *const expansion);

void
expansion_print(Expansion const *const expansion);

void
expansion_parse_preamble(gzFile stream, Expansion *const expansion, bool silent);

ExpClause *
expansion_yield_clause(Expansion *const expansion) __attribute__((warn_unused_result));

#endif
