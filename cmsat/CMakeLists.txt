include_directories(
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
#list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" GitSHA1.h)


set(forl_lib_files
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
    implcache.cpp
    stamp.cpp
    compfinder.cpp
    comphandler.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

set(forl_lib_link_libs "")

if (M4RI_FOUND)
    include_directories(${M4RI_INCLUDE_DIRS})

    add_library(m4ri STATIC IMPORTED)
    #add_library(m4ri SHARED IMPORTED)
    set_property(TARGET m4ri PROPERTY IMPORTED_LOCATION ${M4RI_LIBRARIES})

    SET(forl_lib_files ${forl_lib_files} xorfinder.cpp)
    SET(forl_lib_link_libs ${forl_lib_link_libs} m4ri)
endif (M4RI_FOUND)

if (MYSQL_FOUND AND STATSNEEDED)
    SET(forl_lib_files ${forl_lib_files} mysqlstats.cpp)
    SET(forl_lib_link_libs ${forl_lib_link_libs} ${MYSQL_LIB})
endif (MYSQL_FOUND AND STATSNEEDED)

#add_library(forl SHARED
add_library(forl_lib STATIC
    ${forl_lib_files}
)

target_link_libraries(forl_lib
    ${forl_lib_link_libs}
)

add_executable(forl
    dimacsparser.cpp
    main.cpp
)

set(forl_exec_link_libs
    ${Boost_LIBRARIES}
    forl_lib
#    vtkViews
#    vtkInfovis
#    vtkRendering
#    vtkImaging
#    vtkIO
#    ${CMAKE_THREAD_LIBS_INIT}
)

IF (ZLIB_FOUND)
SET(forl_exec_link_libs ${forl_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF(ZLIB_FOUND)


target_link_libraries(forl
    ${forl_exec_link_libs}
)


SET(CPACK_PACKAGE_EXECUTABLES "forl")
