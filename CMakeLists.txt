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


# -----------------------------------------------------------------------------
# Set build flags if compiler if gcc
# -----------------------------------------------------------------------------
if (CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "  -Wall -Wextra -Wunused -Wsign-compare -Wtype-limits -Wuninitialized -Wno-deprecated -g -fno-omit-frame-pointer -O2 -mtune=native")
    SET(CMAKE_CXX_FLAGS_DEBUG "-Wunused -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Wno-deprecated -O0 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wno-deprecated -O2 -g -DNDEBUG -mtune=native") #-fprofile-generate / -fprofile-use
endif(CMAKE_COMPILER_IS_GNUCC)

# -----------------------------------------------------------------------------
# Uncomment these for static compilation under Linux (messes up Valgrind)
# -----------------------------------------------------------------------------
#IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    SET(CMAKE_EXE_LINKER_FLAGS "-static")
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# -----------------------------------------------------------------------------
# Look for ZLIP (For reading zipped CNFs)
# -----------------------------------------------------------------------------
find_package(ZLIB  REQUIRED)
link_directories( ${ZLIB_LIBRARY} )
include_directories( ${ZLIB_INCLUDE_DIR} )

# -----------------------------------------------------------------------------
# Look for OpenMP (Multi-threading extension to C++)
# -----------------------------------------------------------------------------
FIND_PACKAGE(OpenMP REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# -----------------------------------------------------------------------------
# Look for Google Perftools
# -----------------------------------------------------------------------------
#find_package(Perftools)

# -----------------------------------------------------------------------------
# Some awful definitiions
# -----------------------------------------------------------------------------
add_definitions(-DVERSION="3.0.0")
#add_definitions(-DUSE_GAUSS)

# -----------------------------------------------------------------------------
# Look for Boost program options
# -----------------------------------------------------------------------------
find_package( Boost 1.34 REQUIRED COMPONENTS program_options)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

# -----------------------------------------------------------------------------
# Look for GMP (The GNU Multiple Precision Arithmetic Library)
# -----------------------------------------------------------------------------
FIND_PACKAGE(GMP REQUIRED)
INCLUDE_DIRECTORIES(${GMP_INCLUDE_DIR})
#SET(DGtalLibDependencies ${DGtalLibDependencies} ${GMPXX_LIBRARIES} ${GMP_LIBRARIES})
SET(DGtalLibDependencies ${DGtalLibDependencies} ${GMP_LIBRARIES})


# ---------------
# Look for VTK
#---------------
#IF(NOT VTK_BINARY_DIR)
#  FIND_PACKAGE(VTK REQUIRED)
#  IF(NOT VTK_USE_RENDERING)
#    MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
#  ENDIF(NOT VTK_USE_RENDERING)
#  IF(NOT VTK_USE_INFOVIS)
#    MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_INFOVIS.")
#  ENDIF(NOT VTK_USE_INFOVIS)
#  INCLUDE(${VTK_USE_FILE})
#ENDIF(NOT VTK_BINARY_DIR)


# -----------------------------------------------------------------------------
# Compile all subdirs: solver, utilities, fuzzers, etc.
# -----------------------------------------------------------------------------
add_subdirectory(m4ri)
add_subdirectory(Solver)
add_subdirectory(dsharp)

# -----------------------------------------------------------------------------
# Copy built binaries to root (build) dir
# -----------------------------------------------------------------------------
add_custom_target(copy ALL COMMENT "Copying binaries from subdirs to build directory")
add_custom_command(
    TARGET copy
    COMMAND ${CMAKE_COMMAND} -E copy Solver/cryptominisat .
    #COMMAND ${CMAKE_COMMAND} -E copy scripts/fuzzers/fuzzSat/fuzzSat ../scripts/
    COMMAND ${CMAKE_COMMAND} -E copy dsharp/dsharp_exec .
    COMMAND ${CMAKE_COMMAND} -E copy dsharp/dnnf2cnf/dnnf2cnf .
)
add_dependencies(copy
    cryptominisat
    cnfFuzz
    vegardFuzz
    precosat
    dsharp_exec
    dnnf2cnf
)


set(CPACK_PACKAGE_NAME "cryptominisat")
set(CPACK_PACKAGE_VENDOR "www.msoos.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A general-purpose award-winning SAT solver")
set(CPACK_PACKAGE_VERSION "3.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "3")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_GENERATOR TGZ)
include(CPack)


