cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

IF(DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

#set(CMAKE_C_COMPILER "gcc-4.4")
#set(CMAKE_CXX_COMPILER "/usr/bin/g++-4.4")

PROJECT(cryptoms)
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -Wall -Werror -DUSE_GAUSS -g -mtune=native")
SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -Werror -O0 -DUSE_GAUSS -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -fprofile-use -Werror -O3 -g0 -fno-exceptions -DNDEBUG -mtune=native -fomit-frame-pointer")
SET(CMAKE_EXE_LINKER_FLAGS "-static")

find_package( ZLIB )
link_directories( ${ZLIB_LIBRARY} )
include_directories( ${ZLIB_INCLUDE_DIR} )

find_package(Boost)
if(Boost_FOUND)
    message("Boost found --- clauses will be packed (speedup)")
    add_definitions(-DUSE_POOLS)
else(Boost_FOUND)
    message("Boost NOT found, so clauses will NOT be packed (slowdown)")
endif(Boost_FOUND)

add_definitions(-DVERSION="2.6.1")

#FIND_PACKAGE(Threads REQUIRED)
#set(thread_library "${CMAKE_THREAD_LIBS_INIT}")

add_subdirectory(Solver)
#add_subdirectory(SatELite)

add_custom_target(copy ALL
    COMMENT "Copying binaries from subdirs to build directory")

add_custom_command(
    TARGET copy
    COMMAND ${CMAKE_COMMAND} -E copy Solver/cryptominisat .
#   COMMAND ${CMAKE_COMMAND} -E copy SatELite/satelite .
)

#add_dependencies(copy cryptominisat satelite)
add_dependencies(copy cryptominisat)
