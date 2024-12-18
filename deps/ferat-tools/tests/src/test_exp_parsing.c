/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 13.03.2024 */
/* (c) Marcel Simader 2024, Johannes Kepler Universität Linz */

#include "common.h"

#include "../../src/expansion.h"
#include "test_runner.h"

#define MAX_NUM_TEST_EXP_CLAUSES (32)
static ExpClause const *clauses[MAX_NUM_TEST_EXP_CLAUSES];
DECLARE_GZ(exp_);
static Expansion *expansion;

#define EXPANSION_PARSE(formula)                                                     \
    do {                                                                             \
        TMP_WRITE(exp_, formula);                                                    \
        expansion = expansion_new();                                                 \
        expansion_parse_preamble(GZ(exp_), expansion, true);                         \
        for (size_t i = 0; (clauses[i] = expansion_yield_clause(expansion)) != NULL; \
             ++i)                                                                    \
            assert(i < MAX_NUM_TEST_EXP_CLAUSES);                                    \
    } while (0)

void
after_test(void) {
    GZCLOSE(exp_);
    expansion_free(expansion);
}

int
compare_exp_var_mapping(ExpVarMapping expected, union Value actual) {
    ExpVarMapping const *const actual_exp = actual.ptr;
    asserteq(expected.qbf_var, actual_exp->qbf_var);
    asserteq(expected.exp_var, actual_exp->exp_var);
    assertmemeq(expected.num_annotation_literals * sizeof(Literal), &expected.annotation,
                &actual_exp->annotation);
    pass();
}

int
test_simple(void) {
    EXPANSION_PARSE("c Nothing\nc o 1 0\np cnf 1 1\n1 0\n");
    asserteq(1, expansion->p_max_var);
    asserteq(1, expansion->p_num_clauses);
    asserteq(1, expansion->num_clauses_yielded);
    ARRAYLIST_EQUAL(al32, expansion->clause_origins, 1, (uint32_t[]){ 0 });
    HASHTABLE_SUBSET(expansion->exp_var_mappings, 0, (uint64_t[]){}, (ExpVarMapping[]){},
                     compare_exp_var_mapping);
    //
    asserteq(1, clauses[0]->num_literals);
    asserteq(VAR2LIT(1, false), clauses[0]->lits[0]);
    //
    asserteq(NULL, clauses[1]);

    pass();
}

int
test_mapping(void) {
    EXPANSION_PARSE("c x 1 0 1 0 0\nc o 1 0\np cnf 1 1\n1 0\n");
    asserteq(1, expansion->p_max_var);
    asserteq(1, expansion->p_num_clauses);
    asserteq(1, expansion->num_clauses_yielded);
    ARRAYLIST_EQUAL(al32, expansion->clause_origins, 1, (uint32_t[]){ 0 });
    ExpVarMapping evs0[1] = {
        { .exp_var = 1, .qbf_var = 1, .num_annotation_literals = 0, .annotation = {} }
    };
    uint64_t keys0[1] = { hash_fnv1a(1) };
    HASHTABLE_SUBSET(expansion->exp_var_mappings, 1, keys0, evs0,
                     compare_exp_var_mapping);
    //
    asserteq(1, clauses[0]->num_literals);
    asserteq(VAR2LIT(1, false), clauses[0]->lits[0]);
    //
    asserteq(NULL, clauses[1]);

    EXPANSION_PARSE("c x 1 2 0 1 2 0 0\nc x 3 0 5 0 -1 -2 3 0\nc o 1 3 0\np cnf 3 2\n1 "
                    "-2 0\n 2 -3\n");
    asserteq(3, expansion->p_max_var);
    asserteq(2, expansion->p_num_clauses);
    asserteq(2, expansion->num_clauses_yielded);
    uint32_t origins0[2] = { 0, 2 };
    ARRAYLIST_EQUAL(al32, expansion->clause_origins, 2, origins0);
    ExpVarMapping evs1[3] = {
        { .exp_var = 1, .qbf_var = 1, .num_annotation_literals = 0, .annotation = {} },
        { .exp_var = 2, .qbf_var = 2, .num_annotation_literals = 0, .annotation = {} },
        { .exp_var = 3,
          .qbf_var = 5,
          .num_annotation_literals = 3,
          .annotation = { VAR2LIT(1, true), VAR2LIT(2, true), VAR2LIT(3, false) } }
    };
    uint64_t keys1[3] = { hash_fnv1a(1), hash_fnv1a(2), hash_fnv1a(3) };
    HASHTABLE_SUBSET(expansion->exp_var_mappings, 3, keys1, evs1,
                     compare_exp_var_mapping);
    //
    asserteq(2, clauses[0]->num_literals);
    asserteq(VAR2LIT(1, false), clauses[0]->lits[0]);
    asserteq(VAR2LIT(2, true), clauses[0]->lits[1]);
    //
    asserteq(2, clauses[1]->num_literals);
    asserteq(VAR2LIT(2, false), clauses[1]->lits[0]);
    asserteq(VAR2LIT(3, true), clauses[1]->lits[1]);
    //
    asserteq(NULL, clauses[2]);

    pass();
}

int
main(void) {
    addtest(test_simple, "Simple");
    addtest(test_mapping, "Mapping");
    addafter(after_test);
    runtests("Expansion Parsing");
}
