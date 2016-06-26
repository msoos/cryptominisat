# -----------------------------------------------------------------------------
# Check we can find Python
# -----------------------------------------------------------------------------

if (ENABLE_TESTING)
    find_package(PythonInterp REQUIRED)
    mark_as_advanced(CLEAR PYTHON_EXECUTABLE) # Make visible in cmake-gui/ccmake

    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "Python cannot be found. Please install it or disable testing.")
    endif()

    add_test (
        NAME verifier_test
        COMMAND verifier_test.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
