include_directories(
    ${ZLIB_INCLUDE_DIR} 
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${M4RI_INCLUDE_DIRS}
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
#list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" GitSHA1.h)

add_library(cryptoms
    PropEngine.cpp
    PackedRow.cpp
    VarReplacer.cpp
    ClauseCleaner.cpp
    Prober.cpp
    Simplifier.cpp
    ClauseAllocator.cpp
    #CompleteDetachReattacher.cpp
    DimacsParser.cpp
    SCCFinder.cpp
    SolverConf.cpp
    ClauseVivifier.cpp
    CalcDefPolars.cpp
    SolutionExtender.cpp
    CompleteDetachReattacher.cpp
    ImplCache.cpp
    Searcher.cpp
    Solver.cpp
    #    XorFinder.cpp
    #    GateFinder.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

add_executable(cryptominisat
    Main.cpp
)

target_link_libraries(cryptominisat
    ${ZLIB_LIBRARY}
    ${Perftools_LIBRARIES}
    ${Boost_LIBRARIES}
    ${M4RI_LIBRARIES}
    cryptoms
#    vtkViews
#    vtkInfovis
#    vtkRendering
#    vtkImaging
#    vtkIO
#    ${CMAKE_THREAD_LIBS_INIT}
)


SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat")
