include_directories(${crypto_SOURCE_DIR}/mtl)
include_directories(${crypto_SOURCE_DIR}/Solver)
include_directories(${crypto_SOURCE_DIR}/MTRand)

add_executable(cryptominisat Main.C)

target_link_libraries(cryptominisat
    cryptominisatlib
    ${ZLIB_LIBRARY}
)

add_library(cryptominisatlib
    Logger.C
    Solver.C
    Gaussian.cpp
    PackedRow.cpp
    XorFinder.cpp
    Conglomerate.cpp
    MatrixFinder.cpp
    VarReplacer.cpp
    FindUndef.cpp
    ClauseCleaner.cpp
    RestartTypeChooser.cpp
)

