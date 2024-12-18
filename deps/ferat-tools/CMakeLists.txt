# Author: Marcel Simader (marcel.simader@jku.at)
# Date: 18.12.2023
# (c) Marcel Simader 2023, Johannes Kepler Universität Linz

cmake_minimum_required(VERSION 3.16.3)
project(ferat-tools VERSION 0.5.2)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~ Source Compilation ~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(CMAKE_C_STANDARD 11)

# TODO: Change verbosity level back to 1, maybe? (-DVERBOSE=1)
if("${CMAKE_BUILD_TYPE}" STREQUAL "San")
    add_compile_options(-g -Wall -Wextra -O1 -fsanitize=address)
    add_link_options(-fsanitize=address)
    add_definitions(-DVERBOSE=0 -D_FORTIFY_SOURCE=2)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -g)
    add_definitions(-DVERBOSE=0 -D_FORTIFY_SOURCE=2)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Profile")
    add_compile_options(-Wall -Wextra -Og -pg -fprofile-arcs -fbranch-probabilities)
    add_link_options(-pg -fprofile-arcs -fbranch-probabilities)
    add_definitions(-DNDEBUG=1 -U_FORTIFY_SOURCE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    add_compile_options(-O3 -g -Wall -Wextra -flto)
    add_link_options(-flto)
    add_definitions(-DNDEBUG=1 -D_FORTIFY_SOURCE=1)
else()
    if("${CMAKE_BUILD_TYPE}" STREQUAL "")
        message(WARNING "No build type set, using 'Release'")
    endif()
    add_compile_options(-O3 -flto)
    add_link_options(-flto)
    add_definitions(-DNDEBUG=1 -U_FORTIFY_SOURCE)
endif()

set(SRCS
    src/sorting.c
    src/arraylist.c
    src/check.c
    src/expansion.c
    src/ferat-tools.c
    src/hashtable.c
    src/parsing.c
    src/qbf.c
)
set(HDRS
    src/sorting.h
    src/arraylist.h
    src/check.h
    src/expansion.h
    src/ferat-tools.h
    src/hashtable.h
    src/parsing.h
    src/qbf.h
    src/varstruct.h
)
add_executable(ferat-tools ${SRCS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
add_library(ferat STATIC ${SRCS})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~ External Libraries ~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(ferat-tools ${ZLIB_LIBRARIES})
else()
    # TODO: We can set a definition here, and use standard file IO in the source code, in
    #       case we ever want to support no-zlib builds.
    message(FATAL_ERROR "critical external library zlib not found")
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~ Documentation Generation ~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

option(CDOC_OUTPUT_DIR "Generated documentation output dir path" "_docs")
set(DOXYGEN_CONFIG "${PROJECT_SOURCE_DIR}/.doxygen")

message(CHECK_START "Finding Doxygen for documentation")
find_program(DOXYGEN doxygen)
if(${DOXYGEN} STREQUAL "DOXYGEN-NOTFOUND")
    message(CHECK_FAIL "not found")
    message(WARNING "Documentation will not be generated: Doxygen not found")
else()
    message(CHECK_PASS "found under ${DOXYGEN}")
    add_custom_target(docs
        COMMAND ${DOXYGEN} ${DOXYGEN_CONFIG}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    set_target_properties(docs PROPERTIES EXCLUDE_FROM_ALL EXCLUDE_FROM_DEFAULT_BUILD)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~ Subprojects ~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_subdirectory(tests)
    target_link_libraries(test_exp_parsing PRIVATE ferat ${ZLIB_LIBRARIES})
    target_link_libraries(test_qbf_parsing PRIVATE ferat ${ZLIB_LIBRARIES})
    target_link_libraries(test_check PRIVATE ferat ${ZLIB_LIBRARIES})
endif()
