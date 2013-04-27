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

PROJECT(forl)
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/m4-extra) #m4-extra contains some library search cmake stuff

# SET (CMAKE_CXX_COMPILER  "/usr/bin/clang")
# SET (CMAKE_AR      "/usr/bin/llvm-ar-3.1")
# SET (CMAKE_LINKER  "/usr/bin/llvm-ld-3.1" CACHE INTERNAL STRING)
# SET (CMAKE_NM      "/usr/bin/llvm-nm-3.1")
# SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump-3.1")
# SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib-3.1")

# -----------------------------------------------------------------------------
# Set build flags if compiler if gcc
# -----------------------------------------------------------------------------
if (CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "-Werror -std=c++0x  -pedantic -Wall -Wextra -Wunused -Wsign-compare -Wtype-limits -Wuninitialized -Wno-deprecated -g -fno-omit-frame-pointer -O4 -mtune=native")
    SET(CMAKE_CXX_FLAGS_DEBUG "-Werror -std=c++0x -Wunused -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Wno-deprecated -O0 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -g0 -DNDEBUG -mtune=native") #-fprofile-generate / -fprofile-use
endif(CMAKE_COMPILER_IS_GNUCC)

# -----------------------------------------------------------------------------
# Uncomment these for static compilation under Linux (messes up Valgrind)
# -----------------------------------------------------------------------------
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

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
# Look for OpenMP (Multi-threading extension to C++)
# Disabled --> not used
# -----------------------------------------------------------------------------
# FIND_PACKAGE(OpenMP)
# IF (OPENMP_FOUND)
#     MESSAGE(STATUS "OK, Found OpenMP!")
#     #include_directories(${OpenMP_INCLUDE_DIR})
#     #link_directories(${OpenMP_LIB_DIR})
#     add_definitions( -DUSE_OPENMP )
#     set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_C_FLAGS}")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
# ELSE (OPENMP_FOUND)
#     MESSAGE(STATUS "WARNING: Did not find OpenMP, multi-threading will be disabled")
# ENDIF (OPENMP_FOUND)


# -----------------------------------------------------------------------------
# Add GIT version
# -----------------------------------------------------------------------------
#include(GetGitRevisionDescription)
#get_git_head_revision(GIT_REFSPEC GIT_SHA1)



#for DRUP
set(DRUP 1)
if (DRUP)
    add_definitions( -DDRUP )
    #add_definitions( -DDRUP_DEBUG )
endif(DRUP)


#SET(STATSNEEDED 1)

#MySQL
if (STATSNEEDED)
    find_package(MySQL)
    IF (MYSQL_FOUND)
      MESSAGE(STATUS "OK, Found MySQL!")
      include_directories(${MySQL_INCLUDE_DIR})
      link_directories(${MySQL_LIB_DIR})
      add_definitions( -DUSE_MYSQL )
    else (MYSQL_FOUND)
           ERROR(STATUS "WARNING: Did not find MySQL, MySQL support will be disabled")
    endif (MYSQL_FOUND)
ELSE (STATSNEEDED)
    MESSAGE(STATUS "Not using MySQL because STATS_NEEDED is not set")
ENDIF (STATSNEEDED)

# Look for Boost program options
#SET(Boost_USE_STATIC_LIBS ON)
find_package( Boost 1.46 REQUIRED COMPONENTS program_options)

#M4RI
find_package(M4RI)
IF (M4RI_FOUND)
    MESSAGE(STATUS "OK, Found M4RI!")
    add_definitions( -DUSE_M4RI )
ELSE (M4RI_FOUND)
    MESSAGE(FATAL_ERROR "Did not find M4RI, XOR detection&manipulation would not work! Exiting!")
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

if (DRUP)
    add_custom_command(
        TARGET copy
        COMMAND ${CMAKE_COMMAND} -E copy cmsat/forl-drup .
    )
    add_dependencies(copy
        forl-drup
    )
else(DRUP)
    add_custom_command(
        TARGET copy
        COMMAND ${CMAKE_COMMAND} -E copy cmsat/forl-nodrup .
    )
    add_dependencies(copy
        forl-nodrup
    )
endif(DRUP)


set(CPACK_PACKAGE_NAME "forl")
set(CPACK_PACKAGE_VENDOR "www.msoos.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A general-purpose SAT solver")
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
  "/scripts*"
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
  #"/web/jquery/docs/"
  #"/web/jquery/examples/"
  #"/web/scriptaculous-js-1.9.0/test/"
  #"/web/"
  ".gitignore"
  ".git$"
  "tar.gz$"
  "tar.bz2$"
  ".kdev4$"
  "HOWTO*"
  "Doxyfile"
  "README"
  "NEWS"
  "INSTALL"
  #"cmsat_table*"
)
include(CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)


