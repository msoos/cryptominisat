cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

IF(DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()


PROJECT(cryptoms)
if (CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -Wall -Werror -g -mtune=native")
    SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -Werror -O0 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -fprofile-use -Werror -O3 -g0 -fno-exceptions -DNDEBUG -mtune=native -fomit-frame-pointer")
endif(CMAKE_COMPILER_IS_GNUCC)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

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

add_definitions(-DVERSION="2.7.0")
add_definitions(-DUSE_GAUSS)

#FIND_PACKAGE(Threads REQUIRED)
#set(thread_library "${CMAKE_THREAD_LIBS_INIT}")

add_subdirectory(Solver)

add_custom_target(copy ALL
    COMMENT "Copying binaries from subdirs to build directory")

add_custom_command(
    TARGET copy
    COMMAND ${CMAKE_COMMAND} -E copy Solver/cryptominisat .
)

add_dependencies(copy cryptominisat)
