# -----------------------------------------------------------------------------
# Check we can find Python
# -----------------------------------------------------------------------------

if (ENABLE_TESTING AND NOT MSVC)
    add_test (
        NAME verifier_test
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/verifier_test.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
