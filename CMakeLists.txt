cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

IF(DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

PROJECT(cryptominisat)
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -march=native")
SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -O0 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -g0 -DNDEBUG -march=native")

find_package( ZLIB )
link_directories( ${ZLIB_LIBRARY} )
include_directories( ${ZLIB_INCLUDE_DIR} )

include_directories(${cryptominisat_SOURCE_DIR}/mtl)
include_directories(${cryptominisat_SOURCE_DIR}/Solver)
include_directories(${cryptominisat_SOURCE_DIR}/MTRand)

link_directories(${grain-of-salt_BINARY_DIR}/Solver)

add_executable(cryptominisat Solver/Main.C)

target_link_libraries(cryptominisat
	cryptominisatlib
	${ZLIB_LIBRARY}
)

add_library(cryptominisatlib
	Solver/fcopy.cpp
	Solver/Logger.C
	Solver/Solver.C
	Solver/clause.cpp
        Solver/gaussian.cpp
        Solver/packedRow.cpp
        Solver/xorFinder.cpp
        Solver/conglomerate.cpp
        Solver/MatrixFinder.cpp
)

