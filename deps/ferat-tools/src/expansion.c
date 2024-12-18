/* Author: Marcel Simader (marcel.simader@jku.at) */
/*t Date: 19.12.2023 */
/* (c) Marcel Simader 2023, Johannes Kepler Universität Linz */

#include "expansion.h"

#include "parsing.h"
#include "qbf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ARRAYLIST_CLAUSE_ORIGINS_DEFAULT_CAP       (1 << 15)
#define ARRAYLIST_EXP_VAR_MAPPING_KEYS_DEFAULT_CAP (1 << 10)
#define HASHTABLE_EXP_VAR_MAPPING_KEYS_DEFAULT_CAP (1 << 8)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Expansion Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** @brief Creates a new, empty Expansion.
 */
Expansion *
expansion_new(void) {
    Expansion *expansion = malloc(sizeof(Expansion));
    assert(expansion != NULL);
    // Parser left un-initialized
    expansion->num_clauses_yielded = 0;
    expansion->clause_origins = al32_new(ARRAYLIST_CLAUSE_ORIGINS_DEFAULT_CAP);
    expansion->exp_var_mapping_keys
        = alvar_new(ARRAYLIST_EXP_VAR_MAPPING_KEYS_DEFAULT_CAP);
    expansion->exp_var_mappings = ht_new(HASHTABLE_EXP_VAR_MAPPING_KEYS_DEFAULT_CAP);
    return expansion;
}

/** @brief Frees an Expansion, and its contained data structures.
 */
void
expansion_free(Expansion *const expansion) {
    assert(expansion != NULL);
    if (expansion->clause_origins != NULL) al32_free(expansion->clause_origins);
    if (expansion->exp_var_mapping_keys != NULL) {
        Result result;
        for (size_t i = 0; i < expansion->exp_var_mapping_keys->size; ++i) {
            result = ht_get(expansion->exp_var_mappings,
                            hash_fnv1a(alvar_get(expansion->exp_var_mapping_keys, i)));
            if (result.ok && (result.value.ptr != NULL)) free(result.value.ptr);
        }
        alvar_free(expansion->exp_var_mapping_keys);
    }
    if (expansion->exp_var_mappings != NULL) ht_free(expansion->exp_var_mappings);
    free(expansion);
}

/** @brief Prints the contents of an Expansion struct.
 */
void
expansion_print(Expansion const *const expansion) {
    assert(expansion != NULL);
    assert(expansion->clause_origins != NULL);
    assert(expansion->exp_var_mappings != NULL);
    COMMENT("Expansion {\n");
    COMMENT("  max_var=%u\n", expansion->exp_var_mapping_keys->size);
    COMMENT("  Clause Origings:\n");
    COMMENT("    ");
    for (size_t i = 0; i < expansion->clause_origins->size; ++i) {
        if (i != 0) printf(" ");
        printf("%u", al32_get(expansion->clause_origins, i));
    }
    printf("\n");
    COMMENT("  Variable Mappings:\n");
    ExpVarMapping *mapping;
    Result r;
    for (size_t i = 0; i < expansion->exp_var_mapping_keys->size; ++i) {
        r = ht_get(expansion->exp_var_mappings,
                   hash_fnv1a(alvar_get(expansion->exp_var_mapping_keys, i)));
        assert(r.ok);
        mapping = r.value.ptr;
        COMMENT("    Mapping:\n");
        COMMENT("      (CNF var) %u <-> (QBF var) %u\n", mapping->exp_var,
                mapping->qbf_var);
        COMMENT("      num_annotation_literals=%u\n", mapping->num_annotation_literals);
        COMMENT("      Annotations:\n");
        COMMENT("        ");
        for (size_t j = 0; j < mapping->num_annotation_literals; ++j) {
            if (j != 0) printf(" ");
            printf(LIT_FMT, LIT_FMT_ARGS(mapping->annotation[j]));
        }
        printf("\n");
    }
    COMMENT("  Clauses yielded=%u\n", expansion->num_clauses_yielded);
    COMMENT("}\n");
}

/* ~~~~~~~~~~~~~~~~~~~~ Parsing ~~~~~~~~~~~~~~~~~~~~ */

/** @brief Sets up an ExpansionParser, and parses a CNF Expansion preamble in DIMACS
 * format.
 *
 * @note The resulting Expansion struct will contain ExpClause structs which all have
 * sorted arrays. This is important for performance in check.c.
 *
 * @param stream the (gZip) stream to read from
 * @param expansion the Expansion struct to fill
 * @param silent if @c true, do not emit any output, fatal errors exit directly
 * @param[out] expansion a pointer to an existing Expansion struct to write into
 */
void
expansion_parse_preamble(gzFile stream, Expansion *const expansion, bool silent) {
    assert(stream != NULL);
    assert(expansion != NULL);
    assert(expansion->clause_origins != NULL);
    assert(expansion->exp_var_mapping_keys != NULL);
    assert(expansion->exp_var_mappings != NULL);
    // Parsing state-machine
    expansion->parser = (Parser){ .state = PARSE_STATE_NONE,
                                  .col = 1,
                                  .line = 1,
                                  .eof = false,
                                  .silent = silent,
                                  .la = '\0',
                                  .prev = '\0',
                                  .stream = stream };
    Parser *const parser = &expansion->parser;

    bool parsed_problem = false, parsed_origin_mapping = false;
    Variable max_var = VARIABLE_MIN;
    char *word;
    read_one_char(parser);
    while (!parser->eof && (parser->state != PARSE_STATE_CLAUSE)) {
        // This is identical for all cases, since this is a line-based
        // format
        if (handle_newline(parser)) continue;

        switch (parser->state) {
        case PARSE_STATE_NONE:
            switch (parser->la) {
            case 'c':;
                parser->state = PARSE_STATE_COMMENT;
                skip_white(parser);
                read_one_char(parser);
                break;
            case 'p':;
                parser->state = PARSE_STATE_PROBLEM;
                skip_white(parser);
                read_one_char(parser);
                break;
            default:; parser->state = PARSE_STATE_CLAUSE;
            }
            break;

        case PARSE_STATE_PROBLEM:;
            if (parsed_problem)
                fatal_parse_error(parser, EXIT_PARSING_FAILURE,
                                  "Found second, or duplicate 'p ...' header\n");
            word = expect_word(parser);
            if (strcmp(word, "cnf") != 0)
                fatal_parse_error(parser, EXIT_PARSING_FAILURE,
                                  "Only 'cnf' option is supported, not '%s'\n", word);
            expansion->p_max_var = expect_number(parser, true);
            expansion->p_num_clauses = expect_number(parser, true);

            free(word);
            parsed_problem = true;
            parser->state = PARSE_STATE_NONE;
            break;

        case PARSE_STATE_COMMENT:
            word = expect_word(parser);
            if (strcmp(word, "x") == 0)
                parser->state = PARSE_STATE_MAPPING_COMMENT;
            else if (strcmp(word, "o") == 0)
                parser->state = PARSE_STATE_ORIGIN_COMMENT;
            else
                parser->state = PARSE_STATE_PLAIN_COMMENT;
            free(word);
            break;

        case PARSE_STATE_PLAIN_COMMENT:
            // Parse until the end of the line
            read_one_char(parser);
            break;

        case PARSE_STATE_MAPPING_COMMENT:;
            ArrayList_Variable_t *const expansion_variables
                = expect_variable_list(parser);
            ArrayList_Variable_t *const qbf_variables = expect_variable_list(parser);

            if (qbf_variables->size != expansion_variables->size)
                fatal_parse_error(parser, EXIT_PARSING_FAILURE,
                                  "QBF variable (%u) and expansion variable lists (%u)"
                                  " must be of the same size\n",
                                  qbf_variables->size, expansion_variables->size);
            ArrayList_Literal_t *const annotation_literals = expect_literal_list(parser);

            Variable exp_var;
            for (size_t i = 0; i < qbf_variables->size; ++i) {
                ExpVarMapping *mapping = malloc(
                    sizeof(ExpVarMapping) + sizeof(Literal) * annotation_literals->size);
                assert(mapping != NULL);
                exp_var = alvar_get(expansion_variables, i);
                if (exp_var > max_var) max_var = exp_var;
                mapping->exp_var = exp_var;
                mapping->qbf_var = alvar_get(qbf_variables, i);
                mapping->num_annotation_literals = annotation_literals->size;
                for (size_t j = 0; j < annotation_literals->size; ++j)
                    mapping->annotation[j] = allit_get(annotation_literals, j);

                expansion->exp_var_mapping_keys
                    = alvar_append(expansion->exp_var_mapping_keys, exp_var);
                ht_insert(expansion->exp_var_mappings, hash_fnv1a(exp_var),
                          (uint64_t)mapping);
            }

            alvar_free(qbf_variables);
            alvar_free(expansion_variables);
            allit_free(annotation_literals);
            parser->state = PARSE_STATE_NONE;
            break;

        case PARSE_STATE_ORIGIN_COMMENT:;
            bool got_zero = false;
            uint32_t origin;
            while (!parser->eof && parser->la != '\n') {
                int64_t num = expect_number(parser, true);
                assert(num <= UINT32_MAX);
                origin = (uint32_t)num;
                if (origin == 0) {
                    // Terminate list, do not make a node for 0
                    got_zero = true;
                    break;
                }
                // NOTE: This is offset by -1, because 0 is reserved for the list sentinel
                //       (i.e. this is ONE-INDEXED)
                expansion->clause_origins
                    = al32_append(expansion->clause_origins, origin - 1);
            }
            if (!got_zero)
                parse_warning(parser, "Expected '0' delimiter, not %u\n", origin);
            parsed_origin_mapping = true;
            parser->state = PARSE_STATE_NONE;
            break;

        default:
            fatal_parse_error(parser, EXIT_PARSING_FAILURE,
                              "Reached illegal state: %s (%u)\n",
                              parse_state_name(parser->state), parser->state);
        }
    }

    if (!parsed_origin_mapping) {
        parse_warning(
            parser, "No clause origin mapping comment ('c o 1 4 2 2 ... 0') found. "
                    "Falling back to iterative search mode, this might be quite slow.\n");
        al32_free(expansion->clause_origins);
        expansion->clause_origins = NULL;
    }
    if (!parsed_problem)
        fatal_parse_error(parser, EXIT_PARSING_FAILURE,
                          "Expected a 'p ...' header but reached EOF\n");
    if (max_var != expansion->p_max_var) {
        parse_warning(parser,
                      "Expected maximum variable to be %u, but maximum variable is "
                      "actually %u in the expansion mapping comments\n",
                      expansion->p_max_var, max_var);
        // Always just pick bigger one, I guess
        if (max_var > expansion->p_max_var) expansion->p_max_var = max_var;
    }

    // TODO(Marcel): Double-check that the annotations are correct based on the quantifier
    //               prefix in the QBF. If the mapping is wrong, the formula has to be as
    //               well.
}

/** @brief Yields an ExpClause struct every time the function is called, or @c NULL, if
 * the stream is used up.
 *
 * @param eparser ExpParser struct from the expansion_parse_preamble() function
 * @returns a single new ExpClause pointer, which must be freed manually, or @c NULL, if
 *     the stream contains no more clauses
 */
ExpClause *
expansion_yield_clause(Expansion *const expansion) {
    assert(expansion != NULL);
    Parser *const parser = &expansion->parser;
    assert(parser->state == PARSE_STATE_CLAUSE);
    do
        if (parser->eof) return NULL;
    while (handle_newline(parser));
    parser->state = PARSE_STATE_CLAUSE;
    // All we really do here, is read in a literal list, and convert it into a clause.
    ArrayList_Literal_t *const clause_literals = expect_literal_list(parser);
    ExpClause *exp_clause
        = malloc(sizeof(ExpClause) + sizeof(Literal) * clause_literals->size);
    assert(exp_clause != NULL);
    exp_clause->num_literals = clause_literals->size;
    for (size_t i = 0; i < clause_literals->size; ++i)
        exp_clause->lits[i] = allit_get(clause_literals, i);
    allit_free(clause_literals);
    expansion->num_clauses_yielded += 1;
    return exp_clause;
}
