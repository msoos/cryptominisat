include_directories(${cryptoms_SOURCE_DIR}/MTRand)

add_executable(vegardFuzz
    cnf-fuzz-gen2.cc
    )
