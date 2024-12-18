cmake_minimum_required(VERSION 3.3)
project(ijtihad)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        auxiliary.hh
        main.cc
        parse_utils.hh
        qtypes.cc
        qtypes.hh
        Reader.cc
        Reader.hh
        ReadException.cc
        ReadException.hh
        ReadQ.cc
        ReadQ.hh
        MySolver.cc
        MySolver.hh
        debug.hh
        LitTuple.cc
        LitTuple.hh
        QuantifiedFormula.cc
        QuantifiedFormula.hh
        SolverOptions.cc
        SolverOptions.hh)

add_executable(ijtihad ${SOURCE_FILES})
