include_directories(
    ${PROJECT_SOURCE_DIR}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/solvertypesmini.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat_c.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat_c.h" @ONLY)

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
    features_calc.cpp
    features_to_reconf.cpp
    solvefeatures.cpp
    searchstats.cpp
    xorfinder.cpp
    cryptominisat_c.cpp
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
    SET(cryptoms_lib_files ${cryptoms_lib_files}
        sqlitestats.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
    )
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${SQLITE3_LIBRARIES})
endif ()

if (TBB_FOUND)
    set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${TBB_MALLOC_LIBRARY_NAMES})
endif()

if (NOT STATICCOMPILE)
    add_library(libcryptominisat5 SHARED
        ${cryptoms_lib_files}
        cryptominisat.cpp
    )
else()
    add_library(libcryptominisat5 STATIC
        ${cryptoms_lib_files}
        cryptominisat.cpp
    )
endif()
GENERATE_EXPORT_HEADER(libcryptominisat5
         BASE_NAME libcryptominisat5
         #EXPORT_MACRO_NAME libcryptominisat5_EXPORT
         #EXPORT_FILE_NAME MyLibrary_Export.h
         #STATIC_DEFINE MyLibrary_BUILT_AS_STATIC
)

add_dependencies(libcryptominisat5
    tablestruct
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat_c.h
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h
    ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

target_link_libraries(libcryptominisat5
    LINK_PUBLIC ${cryptoms_lib_link_libs}
)
set_target_properties(libcryptominisat5 PROPERTIES
    OUTPUT_NAME cryptominisat5
    PUBLIC_HEADER "${cryptominisat5_public_headers}"
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if (IPASIR)
    if (NOT STATICCOMPILE)
        add_library(ipasircryptominisat5 SHARED
            ${cryptoms_lib_files}
            ipasir.cpp
        )
    else()
        add_library(ipasircryptominisat5 STATIC
            ${cryptoms_lib_files}
            ipasir.cpp
        )
    endif()
    target_link_libraries(ipasircryptominisat5
        LINK_PUBLIC ${cryptoms_lib_link_libs}
    )
    set_target_properties(ipasircryptominisat5 PROPERTIES
        OUTPUT_NAME ipasircryptominisat5
        PUBLIC_HEADER "${cryptominisat5_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

cmsat_add_public_header(libcryptominisat5 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat_c.h )
cmsat_add_public_header(libcryptominisat5 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h )
cmsat_add_public_header(libcryptominisat5 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h )

# -----------------------------------------------------------------------------
# Copy public headers into build directory include directory.
# The cryptominisat5Config.cmake we generate in the build directory depends on
# this.
# -----------------------------------------------------------------------------
set(HEADER_DEST "${PROJECT_BINARY_DIR}/include/cryptominisat5")
add_custom_target(CopyPublicHeaders ALL)
get_target_property(cryptominisat5_public_headers libcryptominisat5 PUBLIC_HEADER)
foreach(public_header ${cryptominisat5_public_headers})
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

install(TARGETS libcryptominisat5
    EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/${LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/${LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/${INCLUDEDIR}/cryptominisat5"
)

if (NOT ONLY_SIMPLE)
    add_executable(cryptominisat5
        main.cpp
        main_exe.cpp
        signalcode.cpp
    )

    if (USE_GAUSS)
        add_executable(scalmc
            main.cpp
            signalcode.cpp
            scalmc.cpp
        )
    endif()
endif()
add_executable(cryptominisat5_simple
    main_simple.cpp
)

set(cryptoms_exec_link_libs
    libcryptominisat5
    ${Boost_LIBRARIES}
)

IF (ZLIB_FOUND)
    SET(cryptoms_exec_link_libs ${cryptoms_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF()

if (NOT ONLY_SIMPLE)
    set_target_properties(cryptominisat5 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE)
    if (STATICCOMPILE)
      SET_TARGET_PROPERTIES(cryptominisat5 PROPERTIES LINK_SEARCH_END_STATIC 1)
    endif()

    if (USE_GAUSS)
        set_target_properties(scalmc PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
            INSTALL_RPATH_USE_LINK_PATH TRUE)
    endif()

endif()
set_target_properties(cryptominisat5_simple PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
    INSTALL_RPATH_USE_LINK_PATH TRUE)
if (STATICCOMPILE)
    SET_TARGET_PROPERTIES(cryptominisat5_simple PROPERTIES LINK_SEARCH_END_STATIC 1)
 endif()

if (NOT ONLY_SIMPLE)
    target_link_libraries(cryptominisat5
        ${cryptoms_exec_link_libs}
    )
    install(TARGETS cryptominisat5
        EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
        RUNTIME DESTINATION ${BINDIR}
    )
    SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat5")

    if (USE_GAUSS)
        target_link_libraries(scalmc
            ${cryptoms_exec_link_libs}
        )
        install(TARGETS scalmc
            EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
            RUNTIME DESTINATION ${BINDIR}
        )
        SET(CPACK_PACKAGE_EXECUTABLES "scalmc")
    endif()
endif()

target_link_libraries(cryptominisat5_simple
    ${cryptoms_exec_link_libs}
)
install(TARGETS cryptominisat5_simple
    EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    RUNTIME DESTINATION ${BINDIR}
)
SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat5_simple")


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
