# Author: Marcel Simader (marcel0simader@gmail.com)
# Date: 08.11.2023

cmake_minimum_required(VERSION 3.16.3)
project(ferat VERSION 0.9.0)

# NOTE: This only works, because --archive and --upate are options to
#       both 'rsync' and 'cp'!
find_program(COPY_PROG rsync cp REQUIRED)
message(STATUS "Copying files with: ${COPY_PROG}")
find_program(MAKE make REQUIRED)

set(CP ${COPY_PROG} --archive)
set(CP_DIR ${COPY_PROG} --archive --update)
set(MAKE ${MAKE} -j)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~ OPTIONS ~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(IJTIHAD_BACKEND lingeling)

set(DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)
set(FERAT_DIR ${CMAKE_SOURCE_DIR}/ferat)

set(BIN_DIR ${FERAT_DIR}/bin)
file(MAKE_DIRECTORY ${BIN_DIR})

set(WORK_DIR ${CMAKE_BINARY_DIR}/work)
file(MAKE_DIRECTORY ${WORK_DIR})

option(LRAT
    "Use this option to enable compiling in LRAT mode."
    FALSE
)

option(RECONFIGURE
    "Use this option to force all dependencies to be reconfigured. By default,
    configurations are reused to avoid constant recompilation."
    FALSE
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~ BUILD ~~~~~~~~~~~~~~~~~~~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(EXECUTABLE_OUTPUT_PATH ${BIN_DIR})

# ~~~~~~~~~~~~~~~~~~~~ ijtihad ~~~~~~~~~~~~~~~~~~~~

add_custom_command(OUTPUT ${WORK_DIR}/ijtihad
    COMMAND ${CP_DIR} ${DEPS_DIR}/ijtihad ${WORK_DIR}
)
add_custom_target(ijtihad ALL
    COMMAND BACKEND=${IJTIHAD_BACKEND} ${MAKE} && ${CP} ijtihad ${BIN_DIR}/ijtihad
    DEPENDS ${WORK_DIR}/ijtihad
    WORKING_DIRECTORY ${WORK_DIR}/ijtihad
)

# ~~~~~~~~~~~~~~~~~~~~ drat-trim ~~~~~~~~~~~~~~~~~~~~

add_custom_command(OUTPUT ${WORK_DIR}/drat-trim
    COMMAND ${CP_DIR} ${DEPS_DIR}/drat-trim ${WORK_DIR}
)
add_custom_target(drat-trim ALL
    COMMAND ${MAKE} && ${CP} drat-trim ${BIN_DIR}/drat-trim
    DEPENDS ${WORK_DIR}/drat-trim
    WORKING_DIRECTORY ${WORK_DIR}/drat-trim
)

# ~~~~~~~~~~~~~~~~~~~~ lrat-trim ~~~~~~~~~~~~~~~~~~~~

if(${LRAT})
    add_custom_command(OUTPUT ${WORK_DIR}/lrat-trim
        COMMAND ${CP_DIR} ${DEPS_DIR}/lrat-trim ${WORK_DIR}
    )
    add_custom_target(lrat-trim ALL
        COMMAND ./configure && ${MAKE} && ${CP} lrat-trim ${BIN_DIR}/lrat-trim
        DEPENDS ${WORK_DIR}/lrat-trim
        WORKING_DIRECTORY ${WORK_DIR}/lrat-trim
    )
endif()

# ~~~~~~~~~~~~~~~~~~~~ KISSAT ~~~~~~~~~~~~~~~~~~~~

add_custom_command(OUTPUT ${WORK_DIR}/kissat
    COMMAND ${CP_DIR} ${DEPS_DIR}/kissat ${WORK_DIR}
)
# This is to avoid KISSAT constantly re-compiling all of its sources, since one of the
# dependencies listed in its Makefile is the Makefile itself. Problem is, the `configure`
# command constantly changes the creation date of said file...
if(NOT ${RECONFIGURE}
        AND IS_DIRECTORY ${WORK_DIR}/kissat/build
        AND EXISTS ${WORK_DIR}/kissat/build/makefile)
    add_custom_target(kissat ALL
        COMMAND ${MAKE} && ${CP} build/kissat ${BIN_DIR}/kissat
        DEPENDS ${WORK_DIR}/kissat
        WORKING_DIRECTORY ${WORK_DIR}/kissat
    )
else()
    message(STATUS "Reconfiguring...")
    add_custom_target(kissat ALL
        COMMAND ./configure && ${MAKE} && ${CP} build/kissat ${BIN_DIR}/kissat
        DEPENDS ${WORK_DIR}/kissat
        WORKING_DIRECTORY ${WORK_DIR}/kissat
    )
endif()

# ~~~~~~~~~~~~~~~~~~~~ CaDiCaL ~~~~~~~~~~~~~~~~~~~~

if(${LRAT})
    add_custom_command(OUTPUT ${WORK_DIR}/cadical
        COMMAND ${CP_DIR} ${DEPS_DIR}/cadical ${WORK_DIR}
    )
    # This is to avoid CaDiCaL constantly re-compiling all of its sources, since one of
    # the dependencies listed in its Makefile is the Makefile itself. Problem is, the
    # `configure` command constantly changes the creation date of said file...
    if(NOT ${RECONFIGURE}
            AND IS_DIRECTORY ${WORK_DIR}/cadical/build
            AND EXISTS ${WORK_DIR}/cadical/build/makefile)
        add_custom_target(cadical ALL
            COMMAND ${MAKE} && ${CP} build/cadical ${BIN_DIR}/cadical
            DEPENDS ${WORK_DIR}/cadical
            WORKING_DIRECTORY ${WORK_DIR}/cadical
        )
    else()
        message(STATUS "Reconfiguring...")
        add_custom_target(cadical ALL
            COMMAND ./configure && ${MAKE} && ${CP} build/cadical ${BIN_DIR}/cadical
            DEPENDS ${WORK_DIR}/cadical
            WORKING_DIRECTORY ${WORK_DIR}/cadical
        )
    endif()
endif()

# ~~~~~~~~~~~~~~~~~~~~ FERAT-tools ~~~~~~~~~~~~~~~~~~~~

add_subdirectory(${DEPS_DIR}/ferat-tools)
