include_directories(
    ${ZLIB_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
#list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" GitSHA1.h)


set(cryptoms_lib_files
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
    gatefinder.cpp
    sqlstats.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

set(cryptoms_lib_link_libs "")

if (M4RI_FOUND)
    include_directories(${M4RI_INCLUDE_DIRS})

    #add_library(m4ri STATIC IMPORTED)
    add_library(m4ri SHARED IMPORTED)
    set_property(TARGET m4ri PROPERTY IMPORTED_LOCATION ${M4RI_LIBRARIES})

    SET(cryptoms_lib_files ${cryptoms_lib_files} xorfinder.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} m4ri)
endif (M4RI_FOUND)

if (MYSQL_FOUND)
    SET(cryptoms_lib_files ${cryptoms_lib_files} mysqlstats.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MYSQL_LIB})
endif (MYSQL_FOUND)

#add_library(cryptoms STATIC
add_library(cryptoms SHARED
    ${cryptoms_lib_files}
)

target_link_libraries(cryptoms
    ${cryptoms_lib_link_libs}
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
