#  Author: Marcel Simader (marcel.simader@jku.at)
#  Date: 14.11.2023
#  (c) Marcel Simader 2023, Johannes Kepler Universität Linz

from typing import Final, final

#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Exit Code ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class ExitCode:
    # pipeline
    DEPS_NOT_FOUND: Final[int] = 71
    QBF_SAT: Final[int] = 72
    EXPANSION_SAT: Final[int] = 73
    INVALID_RAT_PROOF: Final[int] = 74
    INVALID_EXPANSION_MAPPING: Final[int] = 75
    INVALID_FERAT_PROOF: Final[int] = 76
    # process runner
    PROCESS_FAILED: Final[int] = 90
    PROCESS_TIMED_OUT: Final[int] = 91
    PROCESS_OS_ERROR: Final[int] = 92
    # other
    FAIL: Final[int] = 1
    CLI_ERR: Final[int] = 2
