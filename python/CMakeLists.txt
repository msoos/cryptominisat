set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build")

# Copy environment for distutils
file(COPY "${PROJECT_SOURCE_DIR}/src" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/README.rst" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/tests" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/bindings" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in" ${SETUP_PY})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.in" "${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in")
configure_file("${PROJECT_SOURCE_DIR}/src/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/GitSHA1.cpp" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/src/GitSHA1.h" "${CMAKE_CURRENT_BINARY_DIR}/src/GitSHA1.h" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/src/cryptominisat.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/src/cryptominisat_c.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat_c.h" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/src/solvertypesmini.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h" @ONLY)


add_custom_command(OUTPUT ${OUTPUT}/timestamp
                   COMMAND ${PYTHON_EXECUTABLE}
                   ARGS ${SETUP_PY} build
)

add_custom_target(python_interface ALL DEPENDS ${OUTPUT}/timestamp)

# Install with Setuptools
add_custom_target(python_interface_install
                  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install --record files.txt)
# Make a distribution source package
add_custom_target(python_interface_dist_package
                  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} sdist)
# Run unit tests
add_custom_target(python_interface_test
                  DEPENDS ${OUTPUT}/timestamp
                  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} test)

# Install with cmake
install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install --record files.txt)")

if (ENABLE_TESTING)
    add_test (NAME pytest
              WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
              COMMAND ${PYTHON_EXECUTABLE} "${SETUP_PY} test"
    )

    message(STATUS "Will run python test from ${CMAKE_CURRENT_BINARY_DIR} directory")
endif()
