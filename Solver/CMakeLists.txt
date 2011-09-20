include_directories(${PROJECT_SOURCE_DIR}/mtl)
include_directories(${PROJECT_SOURCE_DIR}/Solver)
include_directories(${PROJECT_SOURCE_DIR}/MTRand)
include_directories ( ${Boost_INCLUDE_DIRS} )
include_directories (${PROJECT_SOURCE_DIR}/m4ri/src )

add_library(cryptoms
    Solver.cpp
    PackedRow.cpp
    VarReplacer.cpp
    ClauseCleaner.cpp
    FailedLitSearcher.cpp
    Subsumer.cpp
    ClauseAllocator.cpp
    CompleteDetachReattacher.cpp
    DimacsParser.cpp
    SCCFinder.cpp
    SolverConf.cpp
    ClauseVivifier.cpp
    CalcDefPolars.cpp
    SolutionExtender.cpp
    ImplCache.cpp
    CommandControl.cpp
    RestartPrinter.cpp
    ThreadControl.cpp
    XorFinder.cpp
    GateFinder.cpp
)

target_link_libraries(cryptoms
    m4ri
)

add_executable(cryptominisat
    Main.cpp
)

target_link_libraries(cryptominisat
    ${ZLIB_LIBRARY}
    ${Perftools_LIBRARIES}
    ${Boost_LIBRARIES}
    cryptoms
#    ${CMAKE_THREAD_LIBS_INIT}
)

SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat")
