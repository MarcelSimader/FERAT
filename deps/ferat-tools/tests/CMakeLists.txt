# Author: Marcel Simader (marcel.simader@jku.at)
# Date: 13.03.2024
# (c) Marcel Simader 2024, Johannes Kepler Universität Linz

set(CMAKE_C_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/tests)

add_compile_options(-g -Og)

# ~~~~~~~~~~~~~~~~~~~~ Internal Libraries ~~~~~~~~~~~~~~~~~~~~

set(LIBFERAT ${PROJECT_SOURCE_DIR}/libferat.a)
include_directories(${PROJECT_SOURCE_DIR}/src)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~ Test Executables ~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_executable(test_exp_parsing src/test_exp_parsing.c)
add_executable(test_qbf_parsing src/test_qbf_parsing.c)
add_executable(test_check src/test_check.c)
