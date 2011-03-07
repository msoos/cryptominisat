include_directories(${cryptoms_SOURCE_DIR}/mtl)
include_directories(${cryptoms_SOURCE_DIR}/Solver)
include_directories(${cryptoms_SOURCE_DIR}/MTRand)

# cmake_minimum_required( VERSION 2.6 FATAL_ERROR )
# find_package( Boost 1.34 COMPONENTS REQUIRED thread )
# link_directories ( ${Boost_LIBRARY_DIRS} )
# include_directories ( ${Boost_INCLUDE_DIRS} )


add_executable(cryptominisat
    Main.cpp
    Logger.cpp
    Solver.cpp
    SolverDebug.cpp
    SolverMisc.cpp
    Gaussian.cpp
    PackedRow.cpp
    XorFinder.cpp
    MatrixFinder.cpp
    VarReplacer.cpp
    VarRenamer.cpp
#   FindUndef.cpp
    ClauseCleaner.cpp
    RestartTypeChooser.cpp
    FailedLitSearcher.cpp
    PartFinder.cpp
    Subsumer.cpp
    PartHandler.cpp
    XorSubsumer.cpp
    StateSaver.cpp
    ClauseAllocator.cpp
    UselessBinRemover.cpp
    CompleteDetachReattacher.cpp
    DimacsParser.cpp
    SCCFinder.cpp
    SolverConf.cpp
    ClauseVivifier.cpp
    DataSync.cpp
    MTSolver.cpp
    BothCache.cpp
    CalcDefPolars.cpp
    SolutionExtender.cpp
)

target_link_libraries(cryptominisat
    ${ZLIB_LIBRARY}
    ${Perftools_LIBRARIES}
#    ${Boost_LIBRARIES}
#    ${CMAKE_THREAD_LIBS_INIT}
)
