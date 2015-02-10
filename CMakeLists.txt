cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

#IF(DEFINED CMAKE_BUILD_TYPE)
#    SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
#ELSE(DEFINED CMAKE_BUILD_TYPE)
#  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
#ENDIF(DEFINED CMAKE_BUILD_TYPE)

PROJECT(cryptoms)
if (CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -fno-omit-frame-pointer -Wall -Werror -Wno-deprecated -g -mtune=native")
    SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -Werror -Wno-deprecated -O0 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -Werror -Wno-deprecated -O3 -g -DNDEBUG -mtune=native -fomit-frame-pointer") #-fprofile-generate / -fprofile-use
endif(CMAKE_COMPILER_IS_GNUCC)

#IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    SET(CMAKE_EXE_LINKER_FLAGS "-static")
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/m4-extra)
#find_package(Perftools)

find_package(MPI)
if (MPI_FOUND)
    add_definitions( -DUSE_MPI )
    set( CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER} )
    #enable_language( CXX )
else (MPI_FOUND)
     MESSAGE(STATUS "No suitable C++ MPI implementation found. CryptoMiniSat will not be distributed.")
endif(MPI_FOUND)

find_package(ZLIB  REQUIRED)
link_directories( ${ZLIB_LIBRARY} )
include_directories( ${ZLIB_INCLUDE_DIR} )

FIND_PACKAGE(OpenMP REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

add_definitions( -DVERSION="3.0.0" )
add_definitions( -DUSE_GAUSS )

add_subdirectory(Solver)

add_custom_target(copy ALL
    COMMENT "Copying binaries from subdirs to build directory")

add_custom_command(
    TARGET copy
    COMMAND ${CMAKE_COMMAND} -E copy Solver/cryptominisat .
)

add_dependencies(copy cryptominisat)
