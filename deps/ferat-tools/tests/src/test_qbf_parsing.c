/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 13.03.2024 */
/* (c) Marcel Simader 2024, Johannes Kepler Universität Linz */

#include "common.h"

#include "../../src/qbf.h"
#include "test_runner.h"

DECLARE_GZ(qbf_);
static QBF *qbf;

#define QBF_PARSE(formula)              \
    do {                                \
        TMP_WRITE(qbf_, formula);       \
        qbf = qbf_new();                \
        qbf_parse(GZ(qbf_), qbf, true); \
    } while (0)

void
after_test(void) {
    GZCLOSE(qbf_);
    qbf_free(qbf);
}

int
test_simple(void) {
    QBF_PARSE("p cnf 0 0\n");
    asserteq(0, qbf->max_var);
    asserteq(0, qbf->matrix->size);
    asserteq(0, qbf->prefix->size);
    asserteq(0, qbf->matrix->size);

    QBF_PARSE("p cnf 1 1\ne 1\n1 0\n");
    asserteq(1, qbf->max_var);
    asserteq(1, qbf->matrix->size);
    asserteq(1, qbf->prefix->size);
    asserteq(1, qbf->matrix->size);

    pass();
}

int
test_quant(void) {
    Quantifier *quant;

    QBF_PARSE("p cnf 1 0\na 1\n");
    asserteq(1, qbf->max_var);
    asserteq(0, qbf->matrix->size);
    asserteq(1, qbf->prefix->size);
    asserteq(0, qbf->matrix->size);
    quant = alptr_get(qbf->prefix, 0);
    asserteq(1, quant->num_vars);
    asserteq(0, quant->ordering);
    asserteq(QUANT_TYPE_UNIVERSAL, quant->type);
    asserteq(1, quant->variables[0]);

    QBF_PARSE("p cnf 1 0\ne 1\n");
    quant = alptr_get(qbf->prefix, 0);
    asserteq(1, quant->num_vars);
    asserteq(0, quant->ordering);
    asserteq(QUANT_TYPE_EXISTENTIAL, quant->type);
    asserteq(1, quant->variables[0]);

    QBF_PARSE("p cnf 2 0\ne 1\na 2\n");
    quant = alptr_get(qbf->prefix, 0);
    asserteq(1, quant->num_vars);
    asserteq(0, quant->ordering);
    asserteq(QUANT_TYPE_EXISTENTIAL, quant->type);
    asserteq(1, quant->variables[0]);
    quant = alptr_get(qbf->prefix, 1);
    asserteq(1, quant->num_vars);
    asserteq(1, quant->ordering);
    asserteq(QUANT_TYPE_UNIVERSAL, quant->type);
    asserteq(2, quant->variables[0]);

    QBF_PARSE("p cnf 4 0\na 1 3\ne 2 4\n");
    quant = alptr_get(qbf->prefix, 0);
    asserteq(2, quant->num_vars);
    asserteq(0, quant->ordering);
    asserteq(QUANT_TYPE_UNIVERSAL, quant->type);
    asserteq(1, quant->variables[0]);
    asserteq(3, quant->variables[1]);
    quant = alptr_get(qbf->prefix, 1);
    asserteq(2, quant->num_vars);
    asserteq(1, quant->ordering);
    asserteq(QUANT_TYPE_EXISTENTIAL, quant->type);
    asserteq(2, quant->variables[0]);
    asserteq(4, quant->variables[1]);

    QBF_PARSE("p cnf 4 0\na 1\na 2\ne 3\na 4");
    quant = alptr_get(qbf->prefix, 0);
    asserteq(1, quant->num_vars);
    asserteq(0, quant->ordering);
    asserteq(QUANT_TYPE_UNIVERSAL, quant->type);
    asserteq(1, quant->variables[0]);
    quant = alptr_get(qbf->prefix, 1);
    asserteq(1, quant->num_vars);
    asserteq(1, quant->ordering);
    asserteq(QUANT_TYPE_UNIVERSAL, quant->type);
    asserteq(2, quant->variables[0]);
    quant = alptr_get(qbf->prefix, 2);
    asserteq(1, quant->num_vars);
    asserteq(2, quant->ordering);
    asserteq(QUANT_TYPE_EXISTENTIAL, quant->type);
    asserteq(3, quant->variables[0]);
    quant = alptr_get(qbf->prefix, 3);
    asserteq(1, quant->num_vars);
    asserteq(3, quant->ordering);
    asserteq(QUANT_TYPE_UNIVERSAL, quant->type);
    asserteq(4, quant->variables[0]);

    pass();
}

int
test_clauses(void) {
    QBFClause *clause;

    QBF_PARSE("p cnf 4 2\na 1\na 2\ne 3\na 4\n1 2 0\n3 4 -1 -2 0\n");
    clause = alptr_get(qbf->matrix, 0);
    asserteq(2, clause->num_literals);
    asserteq(VAR2LIT(1, 0), clause->lits[0]);
    asserteq(VAR2LIT(2, 0), clause->lits[1]);
    clause = alptr_get(qbf->matrix, 1);
    asserteq(4, clause->num_literals);
    asserteq(VAR2LIT(3, 0), clause->lits[0]);
    asserteq(VAR2LIT(4, 0), clause->lits[1]);
    asserteq(VAR2LIT(1, 1), clause->lits[2]);
    asserteq(VAR2LIT(2, 1), clause->lits[3]);

    pass();
}

int
main(void) {
    addtest(test_simple, "Simple");
    addtest(test_quant, "Quantifiers");
    addtest(test_clauses, "Clauses");
    addafter(after_test);
    runtests("QBF Parsing");
}
