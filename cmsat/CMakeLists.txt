include_directories(
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
#list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" GitSHA1.h)


set(cryptoms_lib_files
    cnf.cpp
    propengine.cpp
    packedrow.cpp
    varreplacer.cpp
    clausecleaner.cpp
    prober.cpp
    simplifier.cpp
    subsumestrengthen.cpp
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
    implcache.cpp
    stamp.cpp
    compfinder.cpp
    comphandler.cpp
    hyperengine.cpp
#    watcharray.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

set(cryptoms_lib_link_libs "")

if (M4RI_FOUND)
    include_directories(${M4RI_INCLUDE_DIRS})

    if (STATICCOMPILE)
        add_library(m4ri STATIC IMPORTED)
    else (STATICCOMPILE)
        add_library(m4ri SHARED IMPORTED)
    endif (STATICCOMPILE)

    set_property(TARGET m4ri PROPERTY IMPORTED_LOCATION ${M4RI_LIBRARIES})

    SET(cryptoms_lib_files ${cryptoms_lib_files} xorfinder.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} m4ri)
endif (M4RI_FOUND)

if (MYSQL_FOUND AND STATSNEEDED)
    SET(cryptoms_lib_files ${cryptoms_lib_files} mysqlstats.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MYSQL_LIB})
endif (MYSQL_FOUND AND STATSNEEDED)

if (Perftools_FOUND)
    set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${Perftools_TCMALLOC_LIBRARY})
endif (Perftools_FOUND)


if (STATICCOMPILE)
    add_library(cryptoms STATIC
        ${cryptoms_lib_files}
    )
else (STATICCOMPILE)
    add_library(cryptoms SHARED
        ${cryptoms_lib_files}
    )
endif (STATICCOMPILE)

target_link_libraries(cryptoms
    ${cryptoms_lib_link_libs}
)

add_executable(cryptominisat
    dimacsparser.cpp
    main.cpp
)

set(cryptoms_exec_link_libs
    ${Boost_LIBRARIES}
    cryptoms
#    vtkViews
#    vtkInfovis
#    vtkRendering
#    vtkImaging
#    vtkIO
#    ${CMAKE_THREAD_LIBS_INIT}
)

IF (ZLIB_FOUND)
SET(cryptoms_exec_link_libs ${cryptoms_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF(ZLIB_FOUND)


target_link_libraries(cryptominisat
    ${cryptoms_exec_link_libs}
)

SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat")
