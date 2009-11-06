cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

PROJECT(cryptominisat)

SET(MY_BUILD_TYPE RelWithDebInfo CACHE STRING "Used instead of CMAKE_BUILD_TYPE")
SET(CMAKE_BUILD_TYPE ${MY_BUILD_TYPE} CACHE STRING "..." FORCE)
MARK_AS_ADVANCED(CMAKE_BUILD_TYPE )
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -march=native")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")

find_package( Boost 1.34 COMPONENTS system thread filesystem)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

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
        ${Boost_LIBRARIES}
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

