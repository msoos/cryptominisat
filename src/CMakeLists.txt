include_directories(
    ${PROJECT_SOURCE_DIR}
)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat4/cryptominisat.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/solvertypesmini.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat4/solvertypesmini.h" @ONLY)


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
    occsimplifier.cpp
    subsumestrengthen.cpp
    clauseallocator.cpp
    sccfinder.cpp
    solverconf.cpp
    distillerallwithall.cpp
    distillerlongwithimpl.cpp
    str_impl_w_impl_stamp.cpp
    solutionextender.cpp
    completedetachreattacher.cpp
    searcher.cpp
    solver.cpp
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
    features_calc.cpp
    features_to_reconf.cpp
    solvefeatures.cpp
    searchstats.cpp
    xorfinder.cpp
#    watcharray.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

set(cryptoms_lib_link_libs "")

if (USE_GAUSS)
    SET(cryptoms_lib_files ${cryptoms_lib_files}
        gaussian.cpp
        matrixfinder.cpp
    )
endif()

if (M4RI_FOUND)
    include_directories(${M4RI_INCLUDE_DIRS})

    SET(cryptoms_lib_files ${cryptoms_lib_files} toplevelgauss.cpp)
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

add_library(temp_lib_norm OBJECT
    ${cryptoms_lib_files}
    cryptominisat.cpp
)

add_library(temp_lib_ipasir OBJECT
    ipasir.cpp
)

add_dependencies(temp_lib_norm
    tablestruct
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat4/cryptominisat.h
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat4/solvertypesmini.h
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)
add_dependencies(temp_lib_ipasir
    tablestruct
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat.h
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat4/solvertypesmini.h
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

list(APPEND cms_lib_objects_norm $<TARGET_OBJECTS:temp_lib_norm>)
list(APPEND cms_lib_objects_ipasir $<TARGET_OBJECTS:temp_lib_ipasir>)

if (ALSO_BUILD_STATIC_LIB)
    add_library(libcryptominisat4_static STATIC ${cms_lib_objects_norm})
    target_link_libraries(libcryptominisat4_static LINK_INTERFACE_LIBRARIES
        ${cryptoms_lib_link_libs}
    )
    set_target_properties(libcryptominisat4_static PROPERTIES
        OUTPUT_NAME cryptominisat4
        PUBLIC_HEADER "${cryptominisat4_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )

    add_library(libcryptominisat4_ipasir_static STATIC ${cms_lib_objects_ipasir})
    target_link_libraries(libcryptominisat4_ipasir_static LINK_INTERFACE_LIBRARIES
        ${cryptoms_lib_link_libs}
        libcryptominisat4_static
    )
    set_target_properties(libcryptominisat4_ipasir_static PROPERTIES
        OUTPUT_NAME cryptominisat4ipasir
        PUBLIC_HEADER "${cryptominisat4_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

add_library(libcryptominisat4 SHARED ${cms_lib_objects_norm})
target_link_libraries(libcryptominisat4
    LINK_PUBLIC ${cryptoms_lib_link_libs}
)
set_target_properties(libcryptominisat4 PROPERTIES
    OUTPUT_NAME cryptominisat4
    PUBLIC_HEADER "${cryptominisat4_public_headers}"
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

add_library(libcryptominisat4ipasir SHARED ${cms_lib_objects_ipasir})
target_link_libraries(libcryptominisat4ipasir
    LINK_PUBLIC ${cryptoms_lib_link_libs}
    libcryptominisat4
)
set_target_properties(libcryptominisat4ipasir PROPERTIES
    OUTPUT_NAME cryptominisat4ipasir
    PUBLIC_HEADER "${cryptominisat4_public_headers}"
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

cmsat_add_public_header(libcryptominisat4 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat4/cryptominisat.h )
cmsat_add_public_header(libcryptominisat4 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat4/solvertypesmini.h )

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
if (ALSO_BUILD_STATIC_LIB)
    install(TARGETS libcryptominisat4_static
        EXPORT ${CRYPTOMINISAT4_EXPORT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cryptominisat4"
    )
endif()


if (NOT ONLY_SIMPLE)
    add_executable(cryptominisat4
        main.cpp
        main_exe.cpp
        signalcode.cpp
    )
    add_executable(cusp4
        main.cpp
        signalcode.cpp
        cusp.cpp
    )
endif()
add_executable(cryptominisat4_simple
    main_simple.cpp
)

if (STATICCOMPILE)
    if (NOT ONLY_SIMPLE)
        set_target_properties(cryptominisat4 PROPERTIES LINK_SEARCH_END_STATIC 1)
        set_target_properties(cryptominisat4 PROPERTIES LINK_SEARCH_START_STATIC 0)

        set_target_properties(cusp4 PROPERTIES LINK_SEARCH_END_STATIC 1)
        set_target_properties(cusp4 PROPERTIES LINK_SEARCH_START_STATIC 0)
    endif()
    set_target_properties(cryptominisat4_simple PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(cryptominisat4_simple PROPERTIES LINK_SEARCH_START_STATIC 0)
endif (STATICCOMPILE)

set(cryptoms_exec_link_libs
    ${Boost_LIBRARIES}
    libcryptominisat4
)

IF (ZLIB_FOUND)
    SET(cryptoms_exec_link_libs ${cryptoms_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF()

if (NOT ONLY_SIMPLE)
    set_target_properties(cryptominisat4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_target_properties(cryptominisat4 PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

    set_target_properties(cusp4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_target_properties(cusp4 PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
set_target_properties(cryptominisat4_simple PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(cryptominisat4_simple PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

if (NOT ONLY_SIMPLE)
    target_link_libraries(cryptominisat4
        ${cryptoms_exec_link_libs}
    )
    install(TARGETS cryptominisat4
        EXPORT ${CRYPTOMINISAT4_EXPORT_NAME}
        RUNTIME DESTINATION bin
    )
    SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat4")

    target_link_libraries(cusp4
        ${cryptoms_exec_link_libs}
        rt
    )
    install(TARGETS cusp4
        EXPORT ${CRYPTOMINISAT4_EXPORT_NAME}
        RUNTIME DESTINATION bin
    )
    SET(CPACK_PACKAGE_EXECUTABLES "cusp4")
endif()

target_link_libraries(cryptominisat4_simple
    ${cryptoms_exec_link_libs}
)
install(TARGETS cryptominisat4_simple
    EXPORT ${CRYPTOMINISAT4_EXPORT_NAME}
    RUNTIME DESTINATION bin
)
SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat4_simple")


if (FEEDBACKFUZZ)
    add_executable(cms_feedback_fuzz
        fuzz.cpp
        libfuzz/FuzzerCrossOver.cpp
        libfuzz/FuzzerDriver.cpp
        libfuzz/FuzzerInterface.cpp
        libfuzz/FuzzerIO.cpp
        libfuzz/FuzzerLoop.cpp
        libfuzz/FuzzerMain.cpp
        libfuzz/FuzzerMutate.cpp
        libfuzz/FuzzerSanitizerOptions.cpp
        libfuzz/FuzzerSHA1.cpp
        libfuzz/FuzzerTraceState.cpp
        libfuzz/FuzzerUtil.cpp
    )
    target_link_libraries(cms_feedback_fuzz
        ${cryptoms_exec_link_libs}
    )

    set_target_properties(cms_feedback_fuzz PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
