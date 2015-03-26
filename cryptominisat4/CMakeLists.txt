include_directories(
    ${PROJECT_SOURCE_DIR}
)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)

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
    cryptominisat.cpp
    cleaningstats.cpp
    datasync.cpp
    reducedb.cpp
    clausedumper.cpp
    bva.cpp
    intree.cpp
    sql_tablestructure.cpp
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

if (MYSQL_FOUND AND STATS)
    SET(cryptoms_lib_files ${cryptoms_lib_files} mysqlstats.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MYSQL_LIB})
endif ()

if (SQLITE3_FOUND AND STATS)
    SET(cryptoms_lib_files ${cryptoms_lib_files} sqlitestats.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${SQLITE3_LIBRARIES})
endif ()

if (Perftools_FOUND)
    set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${Perftools_TCMALLOC_LIBRARY})
endif (Perftools_FOUND)

if (TBB_FOUND)
    #set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${TBB_MALLOC_LIBRARY_NAMES} ${TBB_MALLOC_PROXY_LIBRARY_NAMES})
    set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${TBB_MALLOC_LIBRARY_NAMES})
endif()

if (STATICCOMPILE)
    add_library(libcryptominisat4 STATIC ${cryptoms_lib_files})
else (STATICCOMPILE)
    add_library(libcryptominisat4 SHARED ${cryptoms_lib_files})
endif (STATICCOMPILE)

target_link_libraries(libcryptominisat4
    ${cryptoms_lib_link_libs}
)
set_target_properties(libcryptominisat4 PROPERTIES
    OUTPUT_NAME cryptominisat4
    PUBLIC_HEADER "${cryptominisat4_public_headers}"
)

cmsat_add_public_header(libcryptominisat4 cryptominisat.h )
cmsat_add_public_header(libcryptominisat4 solvertypesmini.h )

# -----------------------------------------------------------------------------
# Copy public headers into build directory include directory.
# The cryptominisat4Config.cmake we generate in the build directory depends on
# this.
# -----------------------------------------------------------------------------
set(HEADER_DEST "${PROJECT_BINARY_DIR}/include/cryptominisat4")
add_custom_target(CopyPublicHeaders ALL)
get_target_property(cryptominisat4_public_headers libcryptominisat4 PUBLIC_HEADER)
foreach(public_header ${cryptominisat4_public_headers})
    get_filename_component(HEADER_NAME ${public_header} NAME)
    add_custom_command(TARGET CopyPublicHeaders PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory
                               "${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E echo
                       "Copying ${HEADER_NAME} to ${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E
                           copy_if_different
                           ${public_header}
                           "${HEADER_DEST}"
                      )
endforeach()

install(TARGETS libcryptominisat4
    EXPORT ${CRYPTOMINISAT4_EXPORT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cryptominisat4"
)


if (NOT ONLY_SIMPLE)
    add_executable(cryptominisat
        dimacsparser.cpp
        main.cpp
    )
endif()
add_executable(cryptominisat_simple
    dimacsparser.cpp
    main_simple.cpp
)

if (STATICCOMPILE)
    if (NOT ONLY_SIMPLE)
        set_target_properties(cryptominisat PROPERTIES LINK_SEARCH_END_STATIC 1)
        set_target_properties(cryptominisat PROPERTIES LINK_SEARCH_START_STATIC 0)
    endif()
    set_target_properties(cryptominisat_simple PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(cryptominisat_simple PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)

set(cryptoms_exec_link_libs
    ${Boost_LIBRARIES}
    libcryptominisat4
)

IF (ZLIB_FOUND)
    SET(cryptoms_exec_link_libs ${cryptoms_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF()

if (NOT ONLY_SIMPLE)
    set_target_properties(cryptominisat PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_target_properties(cryptominisat PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
set_target_properties(cryptominisat_simple PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(cryptominisat_simple PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

if (NOT ONLY_SIMPLE)
    target_link_libraries(cryptominisat
        ${cryptoms_exec_link_libs}
    )
    install(TARGETS cryptominisat
        EXPORT ${CRYPTOMINISAT4_EXPORT_NAME}
        RUNTIME DESTINATION bin
    )
    SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat")
endif()

target_link_libraries(cryptominisat_simple
    ${cryptoms_exec_link_libs}
)
install(TARGETS cryptominisat_simple
    EXPORT ${CRYPTOMINISAT4_EXPORT_NAME}
    RUNTIME DESTINATION bin
)
SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat_simple")
