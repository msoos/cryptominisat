execute_process(COMMAND python-config --cflags OUTPUT_VARIABLE PY_C_CONFIG)
execute_process(COMMAND python-config --ldflags OUTPUT_VARIABLE PY_LD_CONFIG)
set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build")

configure_file(${SETUP_PY_IN} ${SETUP_PY})

add_custom_command(OUTPUT ${OUTPUT}/timestamp
                   COMMAND ${PYTHON_EXECUTABLE}
                   ARGS setup.py build_ext --inplace --rpath ../lib
                   DEPENDS libcryptominisat4
)

add_custom_target(pytarget ALL DEPENDS ${OUTPUT}/timestamp)

install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install --record files.txt)")

if (ENABLE_TESTING)
    add_test (NAME pytest
              WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
              COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test_pycryptosat.py"
    )

    message(STATUS "Will run python test from ${CMAKE_CURRENT_BINARY_DIR} directory")
endif()
