cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

IF(DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()


PROJECT(cryptoms)
if (CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -fno-omit-frame-pointer -pedantic -Wall -Wextra -Wno-deprecated -g -mtune=native")
    SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -Werror -Wno-deprecated -O0 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -Werror -Wno-deprecated -O3 -g -DNDEBUG -mtune=native -fomit-frame-pointer") #-fprofile-generate / -fprofile-use
endif(CMAKE_COMPILER_IS_GNUCC)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/m4-extra)

find_package(ZLIB  REQUIRED)
link_directories( ${ZLIB_LIBRARY} )
include_directories( ${ZLIB_INCLUDE_DIR} )

FIND_PACKAGE(OpenMP REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

find_package(MySQL REQUIRED)
include_directories(${MySQL_INCLUDE_DIR})
link_directories(${MySQL_LIB_DIR})

#find_package(Perftools)

add_definitions(-DVERSION="2.9.11")
add_definitions(-DUSE_GAUSS)

add_subdirectory(cmsat)

add_custom_target(copy ALL
    COMMENT "Copying binaries from subdirs to build directory")

add_custom_command(
    TARGET copy
    COMMAND ${CMAKE_COMMAND} -E copy cmsat/cryptominisat .
)

add_dependencies(copy cryptominisat)
