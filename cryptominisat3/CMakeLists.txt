include_directories(
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
#list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" GitSHA1.h)

#set(cryptominisat3_lib_objects "")
set(cryptominisat3_public_headers "")

set(cryptoms_lib_files
    cnf.cpp
    propengine.cpp
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
    subsumeimplicit.cpp
    cryptominisat.cpp
#    watcharray.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

# foreach(target ${stp_lib_targets})
#     list(APPEND stp_lib_objects $<TARGET_OBJECTS:${target}>)
#
#     get_target_property(TARGETS_PUBLIC_HEADERS ${target} PUBLIC_HEADER)
#     if (EXISTS "${TARGETS_PUBLIC_HEADERS}")
#         list(APPEND stp_public_headers "${TARGETS_PUBLIC_HEADERS}")
#         message("Adding public header(s) ${TARGETS_PUBLIC_HEADERS} to target libstp")
#     endif()
# endforeach()


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

if (MYSQL_FOUND AND NOT NOSTATS)
    SET(cryptoms_lib_files ${cryptoms_lib_files} mysqlstats.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MYSQL_LIB})
endif (MYSQL_FOUND AND NOT NOSTATS)

if (Perftools_FOUND)
    set(cryptoms_lib_link_libs  ${cryptoms_lib_link_libs} ${Perftools_TCMALLOC_LIBRARY})
endif (Perftools_FOUND)


if (STATICCOMPILE)
    add_library(libcryptominisat3 STATIC
        ${cryptoms_lib_files}
    )
else (STATICCOMPILE)
    add_library(libcryptominisat3 SHARED
        ${cryptoms_lib_files}
    )
endif (STATICCOMPILE)

target_link_libraries(libcryptominisat3
    ${cryptoms_lib_link_libs}
)
set_target_properties(libcryptominisat3 PROPERTIES
    OUTPUT_NAME cryptominisat3
    PUBLIC_HEADER "${cryptominisat3_public_headers}"
)

cmsat_add_public_header(libcryptominisat3 cryptominisat.h )
cmsat_add_public_header(libcryptominisat3 solverconf.h )
cmsat_add_public_header(libcryptominisat3 solvertypesmini.h )

# -----------------------------------------------------------------------------
# Copy public headers into build directory include directory.
# This is done so that projects that use cryptominisat3 as external project (not via CMake)
# can find the public header files.
# -----------------------------------------------------------------------------
set(HEADER_DEST "${CMAKE_BINARY_DIR}/include/cryptominisat3")
add_custom_target(CopyPublicHeaders ALL)
foreach(public_header ${cryptominisat3_public_headers})
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

install(TARGETS libcryptominisat3
    EXPORT ${CRYPTOMINISAT3_EXPORT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cryptominisat3"
)


add_executable(cryptominisat3
    dimacsparser.cpp
    main.cpp
)

set(cryptoms_exec_link_libs
    ${Boost_LIBRARIES}
    libcryptominisat3
)

IF (ZLIB_FOUND)
    SET(cryptoms_exec_link_libs ${cryptoms_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF()

set_target_properties(cryptominisat3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(cryptominisat3 PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
target_link_libraries(cryptominisat3
    ${cryptoms_exec_link_libs}
)
install(TARGETS cryptominisat3
    EXPORT ${CRYPTOMINISAT3_EXPORT_NAME}
    RUNTIME DESTINATION bin
)

SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat3")
