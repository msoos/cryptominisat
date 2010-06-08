include_directories(${cryptoms_SOURCE_DIR}/mtl)
include_directories(${cryptoms_SOURCE_DIR}/Solver)
include_directories(${cryptoms_SOURCE_DIR}/MTRand)

# cmake_minimum_required( VERSION 2.6 FATAL_ERROR )
# find_package( Boost 1.34 COMPONENTS REQUIRED thread )
# link_directories ( ${Boost_LIBRARY_DIRS} )
# include_directories ( ${Boost_INCLUDE_DIRS} )


add_executable(cryptominisat Main.C)

target_link_libraries(cryptominisat
    cryptominisatlib
    ${ZLIB_LIBRARY}
#    ${Boost_LIBRARIES}
#    ${CMAKE_THREAD_LIBS_INIT}
)

add_library(cryptominisatlib
    Logger.cpp
    Solver.cpp
    Gaussian.cpp
    PackedRow.cpp
    XorFinder.cpp
    MatrixFinder.cpp
    VarReplacer.cpp
    FindUndef.cpp
    ClauseCleaner.cpp
    RestartTypeChooser.cpp
    SmallPtr.cpp
    Clause.cpp
    FailedVarSearcher.cpp
    PartFinder.cpp
    Subsumer.cpp
    PartHandler.cpp
    XorSubsumer.cpp
    StateSaver.cpp
    Clause.cpp
)

