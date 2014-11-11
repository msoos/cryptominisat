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
    message(FATAL_ERROR "Could not find lit tool. Try setting LIT_TOOL to the full path for lit")
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
set(GTEST_PREFIX ${CMAKE_PROJECT_DIR}/utils/gtest)
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

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

add_executable(basic_test
    basic_test.cpp
)

if (STATICCOMPILE)
    set_target_properties(basic_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(basic_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)

target_link_libraries(basic_test
    ${Boost_LIBRARIES}
    libcryptominisat4
)

add_executable(assump_test
    assump_test.cpp
)

if (STATICCOMPILE)
    set_target_properties(assump_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(assump_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)

target_link_libraries(assump_test
    ${Boost_LIBRARIES}
    libcryptominisat4
)

add_executable(readme_test
    readme_test.cpp
)

target_link_libraries(readme_test
    libcryptominisat4
)

add_executable(heap_test
    heap_test.cpp
)

if (STATICCOMPILE)
    set_target_properties(heap_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(heap_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)

target_link_libraries(heap_test
    ${Boost_LIBRARIES}
    libcryptominisat4
)

add_executable(clause_test
    clause_test.cpp
)

if (STATICCOMPILE)
    set_target_properties(clause_test PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(clause_test PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)

target_link_libraries(clause_test
    ${Boost_LIBRARIES}
    libcryptominisat4
)
