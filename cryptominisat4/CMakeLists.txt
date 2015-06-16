include_directories(
    ${PROJECT_SOURCE_DIR}
)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)


add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND xxd -i cmsat_tablestructure.sql ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
    DEPENDS ${CMAKE_SOURCE_DIR}/cmsat_tablestructure.sql
)

add_custom_target(tablestruct ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp)

set(cryptoms_lib_files
    cnf.cpp
    propengine.cpp
    varreplacer.cpp
    clausecleaner.cpp
    clauseusagestats.cpp
    prober.cpp
    simplifier.cpp
    subsumestrengthen.cpp
    clauseallocator.cpp
    sccfinder.cpp
    solverconf.cpp
    distiller.cpp
    strengthener.cpp
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
    subsumeimplicit.cpp
    cleaningstats.cpp
    datasync.cpp
    reducedb.cpp
    clausedumper.cpp
    bva.cpp
    intree.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
    features_fast.cpp
#    watcharray.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

set(cryptoms_lib_link_libs "")

if (M4RI_FOUND)
    include_directories(${M4RI_INCLUDE_DIRS})

    SET(cryptoms_lib_files ${cryptoms_lib_files} xorfinder.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${M4RI_LIBRARIES})
endif (M4RI_FOUND)

if (MYSQL_FOUND AND STATS)
    SET(cryptoms_lib_files ${cryptoms_lib_files} mysqlstats.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MYSQL_LIB})
endif ()

if (SQLITE3_FOUND AND STATS)
    SET(cryptoms_lib_files ${cryptoms_lib_files} sqlitestats.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${SQLITE3_LIBRARIES})
endif ()

if (TBB_FOUND)
    #set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${TBB_MALLOC_LIBRARY_NAMES} ${TBB_MALLOC_PROXY_LIBRARY_NAMES})
    set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${TBB_MALLOC_LIBRARY_NAMES})
endif()


add_library(libcryptominisat4_ipasir_static STATIC
    ${cryptoms_lib_files}
    cryptominisat.cpp
    ipasir.cpp
)
add_dependencies(libcryptominisat4_ipasir_static tablestruct)
target_link_libraries(libcryptominisat4_ipasir_static LINK_INTERFACE_LIBRARIES
    ${cryptoms_lib_link_libs}
)
set_target_properties(libcryptominisat4_ipasir_static PROPERTIES
    OUTPUT_NAME cryptominisat4ipasir
    PUBLIC_HEADER "${cryptominisat4_public_headers}"
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

