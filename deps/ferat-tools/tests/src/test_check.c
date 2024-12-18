/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 21.08.2024 */
/* (c) Marcel Simader 2024, Johannes Kepler Universität Linz */

#include "common.h"

#include "../../src/check.h"
#include "test_runner.h"

DECLARE_GZ(exp_);
DECLARE_GZ(qbf_);
static Expansion *expansion;
static QBF *qbf;

#define CHECK_PARSE(exp_formula, qbf_formula)                \
    do {                                                     \
        TMP_WRITE(qbf_, qbf_formula);                        \
        qbf = qbf_new();                                     \
        qbf_parse(GZ(qbf_), qbf, true);                      \
        TMP_WRITE(exp_, exp_formula);                        \
        expansion = expansion_new();                         \
        expansion_parse_preamble(GZ(exp_), expansion, true); \
    } while (0)

void
after_test(void) {
    GZCLOSE(exp_);
    GZCLOSE(qbf_);
}

int
test_simple() {
    FERATCheckResult *result;

    CHECK_PARSE("p cnf 1 0\n0", "p cnf 1 0\n0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    // ∀1 ∃2,3. (1 v 2 v 3)
    // -
    // {1 2} <- {2 3}^[-1]
    CHECK_PARSE("c x 1 2 0 2 3 0 -1 0\nc o 1 0\np cnf 2 1\n1 2 0",
                "p cnf 3 1\na 1 0\ne 2 3 0\n1 2 3 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    // ∀1 ∃3,2. (2 v 1 v 3)
    // -
    // {2 1} <- {3 2}^[-1]
    CHECK_PARSE("c x 2 1 0 3 2 0 -1 0\nc o 1 0\np cnf 2 1\n2 1 0",
                "p cnf 3 1\na 1 0\ne 3 2 0\n2 1 3 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    // ∀1 ∃3,2 ∃4. (2 v 1 v 3) ∧ (4)
    // -
    // {2 1} <- {3 2}^[-1]
    // 3 <- 4^[1]
    CHECK_PARSE("c x 2 1 0 3 2 0 -1 0\nc x 3 0 4 0 1 0\nc o 1 1 2 0\np cnf 3 1\n2 1 0\n1 "
                "2 0\n 3 0",
                "p cnf 4 1\na 1 0\ne 3 2 0\ne 4 0\n2 1 3 0\n4 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    // ∀1 ∃4,5 ∀2 ∃6 ∀3. (-1 v 4 v 5) ∧ (1 v 2 v 3 v -4 v -5 v 6) ∧ (1 v -2 v -3)
    //                   ∧ (-4 v -5 v -6)
    // -
    // {1 2} <- {4 5}^[1]
    // {3 4} <- {4 5}^[-1]
    // 5 <- 6^[-1 -2]
    // 6 <- 6^[1 -2]
    // 7 <- 6^[1 2]
    CHECK_PARSE("c x 1 2 0 4 5 0 1 0\nc x 3 4 0 4 5 0 -1 0\nc x 5 0 6 0 -1 -2 0\nc x 6 0 "
                "6 0 1 -2 0\nc x 7 0 6 0 1 2 0\nc o 1 2 3 4 4 "
                "0\np cnf 7 5\n1 2 0\n-3 -4 5 0\n0\n-1 -2 -7 0\n-1 -2 -6 0\n",
                "p cnf 6 4\na 1 0\ne 4 5 0\na 2 0\ne 6 0\na 3 0\n-1 4 5 0\n1 2 3 -4 -5 "
                "6 0\n1 -2 -3 0\n-4 -5 -6 0\n");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    pass();
}

int
test_wo_clause_origin(void) {
    FERATCheckResult *result;

    // NOTE: Identical to 'test_simple', but with the clause origin mapping removed.

    // ∀1 ∃2,3. (1 v 2 v 3)
    // -
    // {1 2} <- {2 3}^[-1]
    CHECK_PARSE("c x 1 2 0 2 3 0 -1 0\np cnf 2 1\n1 2 0",
                "p cnf 3 1\na 1 0\ne 2 3 0\n1 2 3 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    // ∀1 ∃3,2. (2 v 1 v 3)
    // -
    // {2 1} <- {3 2}^[-1]
    CHECK_PARSE("c x 2 1 0 3 2 0 -1 0\np cnf 2 1\n2 1 0",
                "p cnf 3 1\na 1 0\ne 3 2 0\n2 1 3 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    // ∀1 ∃3,2 ∃4. (2 v 1 v 3) ∧ (4)
    // -
    // {2 1} <- {3 2}^[-1]
    // 3 <- 4^[1]
    CHECK_PARSE("c x 2 1 0 3 2 0 -1 0\nc x 3 0 4 0 1 0\np cnf 3 1\n2 1 0\n1 "
                "2 0\n 3 0",
                "p cnf 4 1\na 1 0\ne 3 2 0\ne 4 0\n2 1 3 0\n4 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    // ∀1 ∃4,5 ∀2 ∃6 ∀3. (-1 v 4 v 5) ∧ (1 v 2 v 3 v -4 v -5 v 6) ∧ (1 v -2 v -3)
    //                   ∧ (-4 v -5 v -6)
    // -
    // {1 2} <- {4 5}^[1]
    // {3 4} <- {4 5}^[-1]
    // 5 <- 6^[-1 -2]
    // 6 <- 6^[1 -2]
    // 7 <- 6^[1 2]
    CHECK_PARSE("c x 1 2 0 4 5 0 1 0\nc x 3 4 0 4 5 0 -1 0\nc x 5 0 6 0 -1 -2 0\nc x 6 0 "
                "6 0 1 -2 0\nc x 7 0 6 0 1 2 0\np cnf 7 5\n1 2 0\n-3 -4 5 0\n0\n-1 -2 -7 "
                "0\n-1 -2 -6 0\n",
                "p cnf 6 4\na 1 0\ne 4 5 0\na 2 0\ne 6 0\na 3 0\n-1 4 5 0\n1 2 3 -4 -5 "
                "6 0\n1 -2 -3 0\n-4 -5 -6 0\n");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(0, result->num_results);

    pass();
}

int
test_wrong_num_existentials(void) {
    FERATCheckResult *result;

    // ∀1 ∃2,3 ∃4,5. (-5 2 1 3) ∧ (4 5)
    // -
    // {1 2} <- {2 3}^[-1]
    // 3 <- 4
    CHECK_PARSE(
        "c x 1 2 0 2 3 0 -1 0\nc x 3 0 4 0 0\nc o 1 1 2 0\np cnf 3 3\n2 1 0\n1 2 0\n 3 0",
        "p cnf 5 2\na 1 0\ne 2 3 0\ne 4 5 0\n-5 2 1 3 0\n4 5 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(3, result->num_results);
    FERATCheckResultType type_array_0[3]
        = { FERAT_CHECK_RESULT_INCORRECT_LITERALS, FERAT_CHECK_RESULT_INCORRECT_LITERALS,
            FERAT_CHECK_RESULT_INCORRECT_LITERALS };
    ARRAYLIST_EQUAL(al8, result->types, 3, type_array_0);

    // ∀1 ∃2,3 ∃4,5. (-5 2 1 3) ∧ (4 5)
    // -
    // {1 2} <- {2 3}^[-1]
    // {3 4} <- {4 5}^[1]
    CHECK_PARSE(
        "c x 1 2 0 2 3 0 -1 0\nc x 3 4 0 4 5 0 1 0\nc o 1 1 2 0\np cnf 4 3\n1 0\n2 "
        "0\n3 4 0",
        "p cnf 5 2\na 1 0\ne 2 3 0\ne 4 5 0\n-5 2 1 3 0\n4 5 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(2, result->num_results);
    FERATCheckResultType type_array_1[2] = { FERAT_CHECK_RESULT_INCORRECT_LITERALS,
                                             FERAT_CHECK_RESULT_INCORRECT_LITERALS };
    ARRAYLIST_EQUAL(al8, result->types, 2, type_array_1);

    pass();
}

int
test_wrong_existentials(void) {
    FERATCheckResult *result;

    // ∀1 ∃2,3 ∃4,5. (-5 2 1 3) ∧ (4 5)
    // -
    // {1 2} <- {2 3}^[-1]
    // {3 4} <- {3 4}^[1]
    // 5 <- 4^[-1]
    // 6 <- 5^[1]
    // 7 <- 5^[-1]
    CHECK_PARSE("c x 1 2 0 2 3 0 -1 0\nc x 3 4 0 3 4 0 1 0\nc x 5 0 4 0 -1 0\nc x 6 0 5 "
                "0 1 0\nc x 7 0 5 0 "
                "-1 0\nc o 1 1 2 2 0\np cnf 7 4\n1 2 -5 0\n5 2 -7\n1 2 0\n3 6 0",
                "p cnf 5 2\na 1 0\ne 2 3 0\ne 4 5 0\n-5 2 1 3 0\n4 5 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(4, result->num_results);
    FERATCheckResultType type_array_0[4]
        = { FERAT_CHECK_RESULT_INCORRECT_LITERALS, FERAT_CHECK_RESULT_INCORRECT_LITERALS,
            FERAT_CHECK_RESULT_INCORRECT_LITERALS,
            FERAT_CHECK_RESULT_INCORRECT_LITERALS };
    ARRAYLIST_EQUAL(al8, result->types, 4, type_array_0);

    pass();
}

int
test_wrong_annotation_size(void) {
    FERATCheckResult *result;

    // ∀1 ∃2,3 ∀4 ∃5. (-5 2 1 3) ∧ (4 5)
    // -
    // {1 2} <- {2 3}^[-1]
    // 3 <- 5^[-1 4]
    // 4 <- 5^[-1 -4]
    // 5 <- 2^[-1 -4]
    CHECK_PARSE("c x 1 2 0 2 3 0 -1 0\nc x 3 0 5 0 -1 4 0\nc x 4 0 5 0 -1 -4 0\nc x 5 0 "
                "2 0 -1 -4 0\nc o 1 2 "
                "0\np cnf 4 2\n5 2 -3 0\n4 0",
                "p cnf 5 2\na 1 0\ne 2 3 0\na 4 0\ne 5 0\n-5 2 1 3 0\n4 5 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(1, result->num_results);
    FERATCheckResultType type_array_0[1] = { FERAT_CHECK_RESULT_INCORRECT_ANNOTATION };
    uint32_t idx_array_0[1] = { 0 };
    ARRAYLIST_EQUAL(al8, result->types, 1, type_array_0);
    ARRAYLIST_EQUAL(al32, result->clause_indices, 1, idx_array_0);

    pass();
}

int
test_conflicting_annotation(void) {
    FERATCheckResult *result;

    // ∀1 ∃4,5 ∀2 ∃6 ∀3. (-1 v 4 v 5) ∧ (1 v 2 v 3 v -4 v -5 v 6) ∧ (1 v -2 v -3)
    //                   ∧ (-4 v -5 v -6)
    // -
    // {1 2} <- {4 5}^[1]
    // {3 4} <- {4 5}^[-1]
    // 5 <- 6^[-1 -2]
    // 6 <- 6^[1 -2]
    // 7 <- 6^[-1 2]
    CHECK_PARSE("c x 1 2 0 4 5 0 1 0\nc x 3 4 0 4 5 0 -1 0\nc x 5 0 6 0 -1 -2 0\nc x 6 0 "
                "6 0 1 -2 0\nc x 7 0 6 0 -1 2 0\nc o 1 2 3 4 4 "
                "0\np cnf 7 5\n1 2 0\n-3 -4 5 0\n0\n-1 -2 -7 0\n-1 -2 -6 0\n",
                "p cnf 6 4\na 1 0\ne 4 5 0\na 2 0\ne 6 0\na 3 0\n-1 4 5 0\n1 2 3 -4 -5 "
                "6 0\n1 -2 -3 0\n-4 -5 -6 0\n");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    asserteq(1, result->num_results);
    FERATCheckResultType type_array_0[1] = { FERAT_CHECK_RESULT_INCORRECT_ANNOTATION };
    uint32_t idx_array_0[1] = { 3 };
    ARRAYLIST_EQUAL(al8, result->types, 1, type_array_0);
    ARRAYLIST_EQUAL(al32, result->clause_indices, 1, idx_array_0);

    pass();
}

int
test_wrong_annotation(void) {
    FERATCheckResult *result;

    // ∀1 ∃3,2 ∃4. (2 v 1 v 3) ∧ (4)
    // -
    // {2 1} <- {3 2}^[-1]
    // 3 <- 4
    CHECK_PARSE("c x 2 1 0 3 2 0 -1 0\nc x 3 0 4 0 0\nc o 1 1 2 0\np cnf 3 1\n2 1 0\n1 "
                "2 0\n 3 0",
                "p cnf 4 1\na 1 0\ne 3 2 0\ne 4 0\n2 1 3 0\n4 0");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    FERATCheckResultType type_array_0[1] = { FERAT_CHECK_RESULT_INCORRECT_ANNOTATION };
    uint32_t idx_array_0[1] = { 2 };
    ARRAYLIST_EQUAL(al8, result->types, 1, type_array_0);
    ARRAYLIST_EQUAL(al32, result->clause_indices, 1, idx_array_0);

    // ∀1 ∃4,5 ∀2 ∃6 ∀3. (-1 v 4 v 5) ∧ (1 v 2 v 3 v -4 v -5 v 6) ∧ (1 v -2 v -3)
    //                   ∧ (-4 v -5 v -6)
    // -
    // {1 2} <- {4 5}^[1]
    // {3 4} <- {4 5}^[-1]
    // 5 <- 6^[-1 -2]
    // 6 <- 6^[1]
    // 7 <- 6^[2]
    CHECK_PARSE("c x 1 2 0 4 5 0 1 0\nc x 3 4 0 4 5 0 -1 0\nc x 5 0 6 0 -1 -2 0\nc x 6 0 "
                "6 0 1 0\nc x 7 0 6 0 2 0\nc o 1 2 3 4 4 "
                "0\np cnf 7 5\n1 2 0\n-3 -4 5 0\n0\n-1 -2 -7 0\n-1 -2 -6 0\n",
                "p cnf 6 4\na 1 0\ne 4 5 0\na 2 0\ne 6 0\na 3 0\n-1 4 5 0\n1 2 3 -4 -5 "
                "6 0\n1 -2 -3 0\n-4 -5 -6 0\n");
    result = ferat_check_result_new();
    ferat_check(result, qbf, expansion);
    FERATCheckResultType type_array_1[2] = { FERAT_CHECK_RESULT_INCORRECT_ANNOTATION,
                                             FERAT_CHECK_RESULT_INCORRECT_ANNOTATION };
    uint32_t idx_array_1[2] = { 3, 4 };
    ARRAYLIST_EQUAL(al8, result->types, 2, type_array_1);
    ARRAYLIST_EQUAL(al32, result->clause_indices, 2, idx_array_1);

    pass();
}

int
main(void) {
    addtest(test_simple, "Simple");
    addtest(test_wo_clause_origin, "Without Clause Origin");
    addtest(test_wrong_num_existentials, "Wrong No. of Existentials");
    addtest(test_wrong_existentials, "Wrong Existentials");
    addtest(test_wrong_annotation_size, "Wrong Annotation Size");
    addtest(test_conflicting_annotation, "Conflicting Annotation Literals");
    addtest(test_wrong_annotation, "Wrong Annotation Literals");
    addafter(after_test);
    runtests("\\forall-Exp+RAT Expansion Check");
}
