# Original work Copyright (c) 2017, Mate Soos
# Modified work Copyright (c) 2017, Pierre Vignet
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

execute_process(COMMAND python-config --cflags OUTPUT_VARIABLE PY_C_CONFIG)
execute_process(COMMAND python-config --ldflags OUTPUT_VARIABLE PY_LD_CONFIG)

set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build")

# Copy environment for distutils
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/README.rst" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/tests" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in" ${SETUP_PY})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.in" "${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in")
configure_file("${PROJECT_SOURCE_DIR}/src/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/GitSHA1.cpp" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/src/cryptominisat.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/src/solvertypesmini.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h" @ONLY)

add_custom_target(mypylib
    COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build
)

add_dependencies(mypylib
    tablestruct
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

add_custom_target(python_interface ALL DEPENDS mypylib)

# Install with Setuptools
add_custom_target(python_interface_install
                  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install --record files.txt)
# Make a distribution source package
add_custom_target(python_interface_dist_package
                  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} sdist)
# Run unit tests
add_custom_target(python_interface_test
                  DEPENDS mypylib
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
