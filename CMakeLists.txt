cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# -----------------------------------------------------------------------------
# Make RelWithDebInfo the default build type if otherwise not set
# -----------------------------------------------------------------------------
IF(DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
  CACHE STRING "Choose the type of build: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
  "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

PROJECT(cryptoms)
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/m4-extra) #m4-extra contains some library search cmake stuff

macro(add_cxx_flag flagname)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}")
endmacro()

option(SANITIZE "Use Clang sanitizers. This will force using clang++ as the compiler" OFF)
if (SANITIZE)
    SET (CMAKE_CXX_COMPILER "clang++")
    add_cxx_flag("-fsanitize=return")
    add_cxx_flag("-fsanitize=bounds")
    add_cxx_flag("-fsanitize=integer")
    add_cxx_flag("-fsanitize=undefined")
    add_cxx_flag("-fsanitize=float-divide-by-zero")
    add_cxx_flag("-fsanitize=integer-divide-by-zero")
    add_cxx_flag("-fsanitize=null")
    add_cxx_flag("-fsanitize=unsigned-integer-overflow")
    add_cxx_flag("-fsanitize=signed-integer-overflow")
    #add_cxx_flag("-fsanitize=address")
    add_cxx_flag("-fsanitize=alignment")
    add_cxx_flag("-fsanitize=bool")
    add_cxx_flag("-fsanitize=bounds")
    add_cxx_flag("-fsanitize=enum")
    add_cxx_flag("-fsanitize=float-cast-overflow")
    #add_cxx_flag("-fno-sanitize-recover")
    add_cxx_flag("-Wno-bitfield-constant-conversion")
endif()

SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++0x -g -O3 -mtune=native")
SET(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wno-deprecated -O0 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++0x -O2 -g -DNDEBUG -mtune=native")


include(CheckCXXCompilerFlag)
macro(add_cxx_flag_if_supported flagname)
  check_cxx_compiler_flag("${flagname}" HAVE_FLAG_${flagname})

  if(HAVE_FLAG_${flagname})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}")
    message(STATUS "flag ${flagname} supported")
  else()
    message(STATUS "flag ${flagname} NOT supported")
  endif()
endmacro()

add_cxx_flag_if_supported("-Wall")
add_cxx_flag_if_supported("-Wextra")
add_cxx_flag_if_supported("-Wunused")
add_cxx_flag_if_supported("-pedantic")
add_cxx_flag_if_supported("-Wsign-compare")
add_cxx_flag_if_supported("-fno-omit-frame-pointer")
add_cxx_flag_if_supported("-Wtype-limits")
add_cxx_flag_if_supported("-Wuninitialized")
add_cxx_flag_if_supported("-Wno-deprecated")
add_cxx_flag_if_supported("-Wstrict-aliasing")
add_cxx_flag_if_supported("-Wpointer-arith")
add_cxx_flag_if_supported("-Wheader-guard")

# -----------------------------------------------------------------------------
# Uncomment these for static compilation under Linux (messes up Valgrind)
# -----------------------------------------------------------------------------
option(STATICCOMPILE "Compile to static executable (only works on linux)" OFF)
IF(STATICCOMPILE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(Boost_USE_STATIC_LIBS ON)
ENDIF(STATICCOMPILE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))

# -----------------------------------------------------------------------------
# Look for ZLIP (For reading zipped CNFs)
# -----------------------------------------------------------------------------
find_package(ZLIB)
IF (ZLIB_FOUND)
    MESSAGE(STATUS "OK, Found ZLIB!")
    include_directories(${ZLIB_INCLUDE_DIR})
    link_directories(${ZLIB_LIB_DIR})
    add_definitions( -DUSE_ZLIB )
ELSE (ZLIB_FOUND)
    MESSAGE(STATUS "WARNING: Did not find ZLIB, gzipped file support will be disabled")
ENDIF (ZLIB_FOUND)

# -----------------------------------------------------------------------------
# Add GIT version
# -----------------------------------------------------------------------------
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

option(NOSTATS "Don't use statistics at all")
if (NOT NOSTATS)
    add_definitions( -DSTATS_NEEDED )
    find_package(MySQL)
    IF (MYSQL_FOUND)
        MESSAGE(STATUS "OK, Found MySQL!")
        include_directories(${MySQL_INCLUDE_DIR})
        link_directories(${MySQL_LIB_DIR})
        add_definitions( -DUSE_MYSQL )
    else (MYSQL_FOUND)
        MESSAGE(STATUS "WARNING: Did not find MySQL, MySQL support will be disabled")
    endif (MYSQL_FOUND)
ELSE ()
    MESSAGE(STATUS "Not compiling detailed statistics. Leads to faster system")
ENDIF ()

find_package( Boost 1.46 REQUIRED COMPONENTS program_options unit_test_framework)

#M4RI
find_package(M4RI)
IF (M4RI_FOUND)
    MESSAGE(STATUS "OK, Found M4RI!")
    add_definitions( -DUSE_M4RI )
ELSE (M4RI_FOUND)
    MESSAGE(WARNING "Did not find M4RI, XOR detection&manipulation disabled")
ENDIF (M4RI_FOUND)

# find_package(Perftools)
# IF (Perftools_FOUND)
#     MESSAGE(STATUS "OK, Found Google Perftools")
# else (Perftools_FOUND)
#     MESSAGE(WARNING "Did NOT find Perftools, memory allocation will be terrible")
# endif (Perftools_FOUND)

# -----------------------------------------------------------------------------
# Compile all subdirs: solver, utilities, fuzzers, etc.
# -----------------------------------------------------------------------------
add_subdirectory(cmsat)

# -----------------------------------------------------------------------------
# Copy built binaries to root (build) dir
# -----------------------------------------------------------------------------
add_custom_target(copy ALL COMMENT "Copying binaries from subdirs to build directory")
add_custom_command(
    TARGET copy
    COMMAND ${CMAKE_COMMAND} -E copy cmsat/cryptominisat .
)
add_dependencies(copy
    cryptominisat
)

add_subdirectory(tests)

enable_testing()
ADD_TEST (basic_test tests/basic_test)
ADD_TEST (assump_test tests/assump_test)

set(CPACK_PACKAGE_NAME "cryptominisat")
set(CPACK_PACKAGE_VENDOR "www.msoos.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A general-purpose award-winning SAT solver")
set(CPACK_PACKAGE_VERSION "3.2.0")
set(CPACK_PACKAGE_VERSION_MAJOR "3")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/.git/"
  "/scripts.*/"
  "~$"
  "${CPACK_SOURCE_IGNORE_FILES}"
  "/tests/"
  "/m4/"
  "/cactusplots/"
  "/debugbuild/"
  "/gitstats/"
  "/matrixes/"
  "/man/"
  "/.kdev4/"
  "/web/jquery/docs/"
  "/web/jquery/examples/"
  "/web/scriptaculous-js-1.9.0/test/"
  ".gitignore"
  ".git$"
  "tar.gz$"
  "tar.bz2$"
  ".kdev4$"
  ".directory$"
)
include(CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
