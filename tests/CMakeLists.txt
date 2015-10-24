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

add_executable(basic_test
    basic_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(basic_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(basic_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)

target_link_libraries(basic_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (
    NAME basic_test
    COMMAND basic_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(assump_test
    assump_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(assump_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(assump_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)
target_link_libraries(assump_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (
    NAME assump_test
    COMMAND assump_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(heap_test
    heap_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(heap_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(heap_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)
target_link_libraries(heap_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (
    NAME heap_test
    COMMAND heap_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(clause_test
    clause_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(clause_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(clause_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)
target_link_libraries(clause_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (
    NAME clause_test
    COMMAND clause_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(stp_test
    stp_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(stp_tests PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(stp_tests PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)
target_link_libraries(stp_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (NAME stp_test
    COMMAND stp_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(scc_test
    scc_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(scc_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(scc_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)
target_link_libraries(scc_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (NAME scc_test
    COMMAND scc_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(vrepl_test
    vrepl_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(vrepl_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(vrepl_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)
target_link_libraries(vrepl_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (
    NAME vrepl_test
    COMMAND vrepl_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(clause_cleaner_test
    clause_cleaner_test.cpp
)
if (STATICCOMPILE)
    set_target_properties(clause_cleaner_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(clause_cleaner_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)
target_link_libraries(clause_cleaner_test
    ${GTEST_BOTH_LIBRARIES}
    libcryptominisat4
)
add_test (
    NAME clause_cleaner_test
    COMMAND clause_cleaner_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


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
