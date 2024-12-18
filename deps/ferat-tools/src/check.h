// Author: Marcel Simader (marcel0simader@gmail.com)
// Date: 03.01.2024
// (c) Marcel Simader 2024

#include "common.h"

#include "expansion.h"
#include "qbf.h"

#ifndef FORALL_EXP_RAT_CHECK_INCLUDED
#define FORALL_EXP_RAT_CHECK_INCLUDED

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** @brief Represents a type of error that was found while checking a FERAT proof.
 */
typedef enum FERATCheckResultType {
    FERAT_CHECK_RESULT_INCORRECT_LITERALS = 1,
    FERAT_CHECK_RESULT_INCORRECT_ANNOTATION = 2,
} FERATCheckResultType;

/** @brief Holds an array of #FERATCheckResultType error types, and an array of equal size
 * with all corresponding clause indices, at which the errors first appear.
 */
typedef struct FERATCheckResult {
    ArrayList_uint8_t *types;           ///< @brief ArrayList of #FERATCheckResultType
    ArrayList_uint32_t *clause_indices; ///< @brief ArrayList of @c uint32_t
    uint32_t num_results;
} FERATCheckResult;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FERATCheckResult *
ferat_check_result_new(void);

void
ferat_check_result_print(FERATCheckResult const *const result);

void
ferat_check_result_free(FERATCheckResult *const result);

bool
ferat_check(FERATCheckResult *const result, QBF *const qbf, Expansion *const expansion);

#endif
