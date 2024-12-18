/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 18.12.2023 */
/* (c) Marcel Simader 2023, Johannes Kepler Universität Linz */

#include "common.h"

#include "arraylist.h"
#include "check.h"
#include "expansion.h"
#include "ferat-tools.h"
#include "qbf.h"
#include "sorting.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <zlib.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Private Macros ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define FERAT_ZLIB_BUFFER_SIZE (1 << 16)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Main Entry Point ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void
cli_help(char const *const program_name, uint8_t exit_code) {
    printf(FERAT_USAGE_FMT "\n", program_name);
    exit(exit_code);
}

void
cli_version(void) {
    printf("FERAT by Simader, Seidl, and Rebola-Pardo\n");
    printf("Version %s\n", FERAT_VERSION);
    exit(EXIT_SUCCESS);
}

/** @mainpage
 * FERAT-tools is a utility developed by Martina Seidl and Marcel Simader at the Institute
 * for Symbolic Artificial Intelligence at Johannes Kepler University. It can check the
 * expansion of an expansion-based QBF solver. We use it to combine an expansion trace in
 * CNF with a RAT proof to create a FERAT proof.
 */
int
main(int argc, char const **argv) {
    assert(argc >= 1);
    char const *const program_name = argv[0];

    for (uint8_t i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
            cli_help(program_name, EXIT_SUCCESS);
        else if (!strcmp(argv[i], "-v") || !strcmp(argv[1], "--version"))
            cli_version();
    }

    // Neither -v nor -h, so we have to get exactly 2 arguments
    if (argc != 3) {
        printf("Expected 2 arguments, received %u\n", argc - 1);
        cli_help(program_name, EXIT_FAILURE);
    }

    char const *qbf_file_name = argv[1], *expansion_file_name = argv[2];

    gzFile qbf_fd = gzopen(qbf_file_name, "rb");
    if (gzbuffer(qbf_fd, FERAT_ZLIB_BUFFER_SIZE) == -1) {
        ERR_COMMENT("Unable to expand zlib buffer\n");
        return EXIT_FAILURE;
    }
    if (qbf_fd == Z_NULL) {
        ERR_COMMENT("Unable to open QBF input file: %s\n", qbf_file_name);
        return EXIT_FAILURE;
    }

    gzFile expansion_fd = gzopen(expansion_file_name, "rb");
    if (gzbuffer(expansion_fd, FERAT_ZLIB_BUFFER_SIZE) == -1) {
        ERR_COMMENT("Unable to expand zlib buffer\n");
        return EXIT_FAILURE;
    }
    if (expansion_fd == Z_NULL) {
        ERR_COMMENT("Unable to open CNF expansion file: %s\n", expansion_file_name);
        return EXIT_FAILURE;
    }

    /* TODO(Marcel): Make more CLI flags */
    bool const silent = false;

    INIT_TIME();

    // QBF parsing
    START_TIME(qbf_parsing_time);
    INFO("Start parsing QBF\n");
    QBF *const qbf = qbf_new();
    qbf_parse(qbf_fd, qbf, silent);
    COMMENT("Parsed QBF with max variable %u and %u clause[s]\n", qbf->max_var,
            qbf->matrix->size);
    END_TIME(qbf_parsing_time);
    FLUSH();

    // QBF clause sorting
    START_TIME(qbf_sorting_time);
    INFO("Start sorting QBF clauses by quantifier index\n");
    qbf_sort_clauses_in_matrix(qbf);
    COMMENT("Sorted QBF clauses by quantifier index\n");
    END_TIME(qbf_sorting_time);
#if VERBOSE
    qbf_print(qbf);
#endif
    FLUSH();

    // CNF expansion parsing
    START_TIME(expansion_parsing_time);
    INFO("Start parsing CNF expansion\n");
    Expansion *const expansion = expansion_new();
    expansion_parse_preamble(expansion_fd, expansion, silent);
    COMMENT("Parsed CNF expansion with max variable %u, reporting %u clause[s]\n",
            expansion->p_max_var, expansion->p_num_clauses);
    END_TIME(expansion_parsing_time);
#if VERBOSE
    expansion_print(expansion);
#endif
    FLUSH();

    // Checking
    START_TIME(checking_time);
    INFO("Start checking expansion step\n");
    FERATCheckResult *const result = ferat_check_result_new();
    bool valid = ferat_check(result, qbf, expansion);
    END_TIME(checking_time);
#if VERBOSE
    expansion_print(expansion);
#endif
    FLUSH();

    // Result output
    uint8_t exit_code;
    COMMENT("\n");
    if (valid) {
        RESULT("VERIFIED\n");
        exit_code = EXIT_VERIFIED;
    } else {
        RESULT("NOT VERIFIED\n");
        ferat_check_result_print(result);
        exit_code = EXIT_NOT_VERIFIED;
    }
    COMMENT("\n");
    COMMENT("QBF parsing took " USEC_TO_HUM_RDBL_FMT "\n",
            USEC_TO_HUM_RDBL_FMT_ARGS(qbf_parsing_time));
    COMMENT("QBF sorting took " USEC_TO_HUM_RDBL_FMT "\n",
            USEC_TO_HUM_RDBL_FMT_ARGS(qbf_sorting_time));
    COMMENT("CNF expansion parsing took " USEC_TO_HUM_RDBL_FMT "\n",
            USEC_TO_HUM_RDBL_FMT_ARGS(expansion_parsing_time));
    COMMENT("Expansion verification took " USEC_TO_HUM_RDBL_FMT "\n",
            USEC_TO_HUM_RDBL_FMT_ARGS(checking_time));
    COMMENT("Total time " USEC_TO_HUM_RDBL_FMT "\n",
            USEC_TO_HUM_RDBL_FMT_ARGS(qbf_parsing_time + qbf_sorting_time
                                      + expansion_parsing_time + checking_time));
    FLUSH();

    // Cleanup
    expansion_free(expansion);
    qbf_free(qbf);
    ferat_check_result_free(result);

    return exit_code;
}
