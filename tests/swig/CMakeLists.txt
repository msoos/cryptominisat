execute_process(COMMAND python-config --cflags OUTPUT_VARIABLE PY_C_CONFIG)
execute_process(COMMAND python-config --ldflags OUTPUT_VARIABLE PY_LD_CONFIG)
set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build")

configure_file(${SETUP_PY_IN} ${SETUP_PY})

# swig3.0 -I../../src -python -c++ -o solver_module.cc solver.i
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/solver.so

                   COMMAND swig3.0
                   ARGS -I${PROJECT_SOURCE_DIR}/src -python -c++ -o ${CMAKE_CURRENT_BINARY_DIR}/solver_module.cpp ${CMAKE_CURRENT_SOURCE_DIR}/solver.i

                   COMMAND ${PYTHON_EXECUTABLE}
                   ARGS ${SETUP_PY} build_ext --inplace --rpath ${PROJECT_BINARY_DIR}/lib

                   DEPENDS libcryptominisat4
)

add_custom_target(solver_swig_test ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/solver.so)


# add_test (NAME pytest
#           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#           COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test_pycryptosat.py"
# )

message(STATUS "Will run solver test from ${CMAKE_CURRENT_BINARY_DIR} directory")
