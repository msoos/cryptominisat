include_directories(
    ${ZLIB_INCLUDE_DIR} 
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${M4RI_INCLUDE_DIRS}
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
#list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" GitSHA1.h)

#add_library(m4ri STATIC IMPORTED)
add_library(m4ri SHARED IMPORTED)

set_property(TARGET m4ri PROPERTY IMPORTED_LOCATION ${M4RI_LIBRARIES})

#add_library(cryptoms STATIC
add_library(cryptoms SHARED
    propengine.cpp
    packedrow.cpp
    varreplacer.cpp
    clausecleaner.cpp
    prober.cpp
    simplifier.cpp
    clauseallocator.cpp
    sccfinder.cpp
    solverconf.cpp
    clausevivifier.cpp
    calcdefpolars.cpp
    solutionextender.cpp
    completedetachreattacher.cpp
    searcher.cpp
    solver.cpp
    xorfinder.cpp
    gatefinder.cpp
    sqlstats.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

target_link_libraries(cryptoms
    m4ri
    ${MYSQL_LIB}
)

add_executable(cryptominisat
    dimacsparser.cpp
    main.cpp
)

target_link_libraries(cryptominisat
    ${ZLIB_LIBRARY}
    ${Boost_LIBRARIES}
    cryptoms
#    vtkViews
#    vtkInfovis
#    vtkRendering
#    vtkImaging
#    vtkIO
#    ${CMAKE_THREAD_LIBS_INIT}
)


SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat")
