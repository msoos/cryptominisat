# -----------------------------------------------------------------------------
# Check we can find Python and the lit tool
# -----------------------------------------------------------------------------
find_package(PythonInterp REQUIRED)
mark_as_advanced(CLEAR PYTHON_EXECUTABLE) # Make visible in cmake-gui/ccmake

if(NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Python cannot be found. Please install it or disable testing.")
endif()

find_program(LIT_TOOL
    lit
    DOC "Path to lit tool"
)

if(NOT LIT_TOOL)
    message(FATAL_ERROR "Could not find 'lit' tool. You need to install it by e.g. 'pip install lit' If it's already installed, set LIT_TOOL to the full path for lit")
endif()

# Checking absolute path because if(EXISTS ...) behaviour only well
# defined if path is absolute
IF(NOT IS_ABSOLUTE "${LIT_TOOL}")
    message(FATAL_ERROR "LIT_TOOL must be set to an absolute PATH")
endif()

if(NOT EXISTS "${LIT_TOOL}")
    # Can happen if users environment changes after initial configure
    message(FATAL_ERROR "LIT_TOOL is set but the path does not seem to exist. Try deleting the LIT_TOOL cache variable and reconfiguring")
endif()

set(LIT_ARGS -v CACHE STRING "Arguments to pass to lit")

# -----------------------------------------------------------------------------
# Find GTest library which will be used to drive tests
# -----------------------------------------------------------------------------
# GoogleTest devs don't recommend using a pre-built GTest library
# ( https://code.google.com/p/googletest/wiki/FAQ#Why_is_it_not_recommended_to_install_a_pre-compiled_copy_of_Goog ).
# Because of this, distros like Ubuntu don't provide a pre-built GTest library
# so ``find_package(GTest REQUIRED)`` fails.
#
# Instead it is recommended that projects build their own copy of GTest. Detecting
# the location of GTest source code is probably error prone so using a copy in the
# repository seems like the easiest thing to do. This also has the added benefit that
# everyone uses the same version of GTest.
set(GTEST_PREFIX ${PROJECT_SOURCE_DIR}/utils/gtest)
message(STATUS "NOTE: if adding the 'gtest' subdirectory fails, you need to issue 'git submodule init' and 'git submodule update'")
add_subdirectory(${GTEST_PREFIX} gtest)
set(GTEST_BOTH_LIBRARIES gtest gtest_main)

include_directories(${GTEST_PREFIX}/include)

# Add handy macros/functions
include(AddSTPGTest)
include(AddGTestSuite)

# -----------------------------------------------------------------------------
# Tests that drive cryptominisat by using cnf files (e.g. smt2, smt and cvc files)
# -----------------------------------------------------------------------------

option(TEST_CNF_FILES
       "Enable tests that use cnf files to drive cryptominisat"
       ON
      )

if(TEST_CNF_FILES)
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests/cnf-files)
endif()


# -----------------------------------------------------------------------------

add_subdirectory(${PROJECT_SOURCE_DIR}/utils/cnf-utils cnf-utils)
add_subdirectory(${PROJECT_SOURCE_DIR}/utils/sha1-sat sha1-sat)


if (PYTHON_OK AND NOT STATS)
    # add_subdirectory(swig)
endif()


include_directories(
    ${PROJECT_SOURCE_DIR}
)
include_directories(
    ${PROJECT_BINARY_DIR}/cmsat4-src
)

add_executable(readme_test
    readme_test.cpp
)
target_link_libraries(readme_test
    libcryptominisat4
)
add_test (
    NAME readme_test
    COMMAND readme_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set (MY_TESTS
    basic_test
    assump_test
    heap_test
    clause_test
    stp_test
    scc_test
    vrepl_test
    clause_cleaner_test
    probe_test
    distiller_all_with_all_test
    str_impl_w_impl_stamp
    distill_long_with_implicit_test
    subsume_impl_test
    comp_find_test
    intree_test
    xorfinder_test
    comphandler_test
)

if (USE_GAUSS)
    set (MY_TESTS ${MY_TESTS}
        gauss_test
        matrixfinder_test
    )
endif()

foreach(F ${MY_TESTS})
    add_executable(${F}
        ${F}.cpp
    )
    if (STATICCOMPILE)
        set_target_properties(${F} PROPERTIES LINK_SEARCH_END_STATIC 1)
        set_target_properties(${F} PROPERTIES LINK_SEARCH_START_STATIC 0)
    endif (STATICCOMPILE)
    target_link_libraries(${F}
        ${GTEST_BOTH_LIBRARIES}
        libcryptominisat4
    )
    add_test (
        NAME ${F}
        COMMAND ${F}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()


# This test is not stable enough, depends on CPU speed
# if (NOT SLOW_DEBUG)
#     add_executable(library_speed_test
#         library_speed_test.cpp
#     )
#
#     target_link_libraries(library_speed_test
#         libcryptominisat4
#     )
# endif()
