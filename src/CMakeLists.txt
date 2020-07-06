# Copyright (c) 2017, Mate Soos
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${BOSPHORUS_INCLUDE_DIRS} )

if (NOT WIN32)
    add_cxx_flag_if_supported("-Wno-bitfield-constant-conversion")
    #add_cxx_flag_if_supported("-Wduplicated-cond")
    #add_cxx_flag_if_supported("-Wduplicated-branches")
    add_cxx_flag_if_supported("-Wlogical-op")
    add_cxx_flag_if_supported("-Wrestrict")
    add_cxx_flag_if_supported("-Wnull-dereference")
    add_cxx_flag_if_supported("-Wdouble-promotion")
    add_cxx_flag_if_supported("-Wshadow")
    add_cxx_flag_if_supported("-Wformat=2")
    add_cxx_flag_if_supported("-Wextra-semi")
    add_cxx_flag_if_supported("-pedantic")
    #add_cxx_flag_if_supported("-Wdeprecated")

    if (FINAL_PREDICTOR)
    add_cxx_flag_if_supported("-Wno-unused-parameter")
    add_cxx_flag_if_supported("-Wno-unused-but-set-variable")
    endif()
endif()
add_sanitize_flags()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if (ENABLE_TESTING)
    add_definitions( -DCMS_TESTING_ENABLED )
    set(GTEST_PREFIX ${PROJECT_SOURCE_DIR}/utils/gtest)
    include_directories(${GTEST_PREFIX}/include)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/solvertypesmini.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat_c.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat_c.h" @ONLY)

if (SQLITE3_FOUND)
    if (NOT PYTHON_EXECUTABLE)
        MESSAGE(FATAL_ERROR "Unfortunately, the python interpreter is needed for statistics because of SQL text generation")
    endif()

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${PYTHON_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py cmsat_tablestructure.sql ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/cmsat_tablestructure.sql
    )
    add_custom_target(tablestruct ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp)
endif()

set(cryptoms_lib_files
    cnf.cpp
    drat.cpp
    propengine.cpp
    varreplacer.cpp
    clausecleaner.cpp
    occsimplifier.cpp
    subsumestrengthen.cpp
    clauseallocator.cpp
    sccfinder.cpp
    solverconf.cpp
    distillerlong.cpp
    distillerlongwithimpl.cpp
    str_impl_w_impl.cpp
    solutionextender.cpp
    completedetachreattacher.cpp
    searcher.cpp
    solver.cpp
    compfinder.cpp
    comphandler.cpp
    hyperengine.cpp
    subsumeimplicit.cpp
    datasync.cpp
    reducedb.cpp
    clausedumper.cpp
    bva.cpp
    intree.cpp
    satzilla_features_calc.cpp
    satzilla_features_to_reconf.cpp
    satzilla_features.cpp
    searchstats.cpp
    xorfinder.cpp
    cardfinder.cpp
    cryptominisat_c.cpp
    yalsat.cpp
    yals.c
    walksat.cpp
    sls.cpp
    vardistgen.cpp
    ccnr.cpp
    ccnr_cms.cpp
    lucky.cpp
#    watcharray.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

set(cryptoms_lib_link_libs "")

if (FINAL_PREDICTOR)

    set(cryptoms_lib_files
        ${cryptoms_lib_files}
#         predict/clustering_imp.cpp
        cl_predictors.cpp
    )
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} xgboost dmlc rabit rt)
endif()

if (USE_GAUSS)
    SET(cryptoms_lib_files ${cryptoms_lib_files}
        gaussian.cpp
        packedrow.cpp
        matrixfinder.cpp
    )
endif()

if (MPI_FOUND)
    SET(cryptoms_lib_files ${cryptoms_lib_files}
        datasyncserver.cpp
    )
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MPI_CXX_LIBRARIES})
endif()

if (M4RI_FOUND)
    include_directories(${M4RI_INCLUDE_DIRS})

    SET(cryptoms_lib_files ${cryptoms_lib_files} toplevelgauss.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${M4RI_LIBRARIES})
endif (M4RI_FOUND)

if (BOSPHORUS_LIBRARIES)
    include_directories( ${BOSPHORUS_INCLUDE_DIRS} )

    SET(cryptoms_lib_files ${cryptoms_lib_files} cms_bosphorus.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${BOSPHORUS_LIBRARIES})
endif()

if (BREAKID_LIBRARIES)
    include_directories( ${BREAKID_INCLUDE_DIRS} )

    SET(cryptoms_lib_files ${cryptoms_lib_files} cms_breakid.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${BREAKID_LIBRARIES})
endif()

if (SQLITE3_FOUND)
    SET(cryptoms_lib_files ${cryptoms_lib_files}
        sqlitestats.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
    )
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${SQLITE3_LIBRARIES})
endif ()

add_library(cryptominisat5
    ${cryptoms_lib_files}
    cryptominisat.cpp
)

GENERATE_EXPORT_HEADER(cryptominisat5
         BASE_NAME cryptominisat5
         #EXPORT_MACRO_NAME cryptominisat5_EXPORT
         #EXPORT_FILE_NAME MyLibrary_Export.h
         #STATIC_DEFINE MyLibrary_BUILT_AS_STATIC
)

if (SQLITE3_FOUND AND STATS)
    add_dependencies(cryptominisat5
        tablestruct
    )
endif()

# indicate that we depend on pthread, and compile in the actual library
target_link_libraries(cryptominisat5
    LINK_PUBLIC ${cryptoms_lib_link_libs}
    LINK_PUBLIC ${CMAKE_THREAD_LIBS_INIT}
)

if (NOT WIN32)
    set_target_properties(cryptominisat5 PROPERTIES
        PUBLIC_HEADER "${cryptominisat5_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
else()
    set_target_properties(cryptominisat5 PROPERTIES
        OUTPUT_NAME cryptominisat5win
        PUBLIC_HEADER "${cryptominisat5_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

if (IPASIR)
#     if ((NOT WIN32) AND (STATICCOMPILE))
#         add_library(ipasircryptominisat5_tmp
#             ipasir.cpp
#             ${cryptoms_lib_files}
#             cryptominisat.cpp
#         )
#         add_custom_target(ipasircryptominisat5_target
#               COMMAND ${CMAKE_AR} rc libipasircryptominisat5.a
#               $<TARGET_FILE:ipasircryptominisat5_tmp>
#               ${cryptoms_lib_link_libs}
#         )
#         add_dependencies(ipasircryptominisat5_target cryptominisat5 ipasircryptominisat5_tmp)
#         add_custom_target(ipasir_final ALL)
#         add_custom_command(TARGET ipasir_final
#             COMMAND ${CMAKE_COMMAND} -E copy
#                 ${CMAKE_CURRENT_BINARY_DIR}/libipasircryptominisat5.a
#                 ${PROJECT_BINARY_DIR}/lib/
#         )
#         add_dependencies(ipasir_final ipasircryptominisat5_target)
#     else ()
    add_library(ipasircryptominisat5
        ipasir.cpp
        ${cryptoms_lib_files}
        cryptominisat.cpp
    )
#         set_target_properties(ipasircryptominisat5 PROPERTIES LINK_SEARCH_START_STATIC 1)
    target_link_libraries(ipasircryptominisat5
        LINK_PUBLIC ${cryptoms_lib_link_libs}
        LINK_PUBLIC ${CMAKE_THREAD_LIBS_INIT}
    )
#         set_target_properties(ipasircryptominisat5 PROPERTIES LINK_SEARCH_END_STATIC 1)
    set_target_properties(ipasircryptominisat5 PROPERTIES
        OUTPUT_NAME ipasircryptominisat5
        PUBLIC_HEADER "${cryptominisat5_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
    install(TARGETS ipasircryptominisat5
        EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cryptominisat5"
    )
#     endif()

endif()

cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat_c.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/cryptominisat.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5/solvertypesmini.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/dimacsparser.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/streambuffer.h )

# -----------------------------------------------------------------------------
# Copy public headers into build directory include directory.
# The cryptominisat5Config.cmake we generate in the build directory depends on
# this.
# -----------------------------------------------------------------------------
set(HEADER_DEST "${PROJECT_BINARY_DIR}/include/cryptominisat5")
add_custom_target(CopyPublicHeaders ALL)
get_target_property(cryptominisat5_public_headers cryptominisat5 PUBLIC_HEADER)
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

install(TARGETS cryptominisat5
    EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cryptominisat5"
)

if (NOT ONLY_SIMPLE)
    add_executable(cryptominisat5-bin
        main.cpp
        main_common.cpp
        main_exe.cpp
        signalcode.cpp
    )
endif()

if (EMSCRIPTEN)
    add_executable(cryptominisat5_simple-bin
        main_emscripten.cpp
        main_common.cpp
    )

    set_target_properties(cryptominisat5_simple-bin
    PROPERTIES
    OUTPUT_NAME cryptominisat5_simple
    LINK_FLAGS "-s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'  -s LINKABLE=1 -s EXPORT_ALL=1"
    )

    SET_SOURCE_FILES_PROPERTIES(main_emscripten.cpp
        PROPERTIES COMPILE_FLAGS "-s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'  -s LINKABLE=1 -s EXPORT_ALL=1"
    )
else()
    add_executable(cryptominisat5_simple-bin
        main_simple.cpp
        main_common.cpp
    )
endif()

set(cryptoms_exec_link_libs
    cryptominisat5
)

IF (ZLIB_FOUND)
    SET(cryptoms_exec_link_libs ${cryptoms_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF()

set_target_properties(cryptominisat5_simple-bin PROPERTIES
    OUTPUT_NAME cryptominisat5_simple
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
    INSTALL_RPATH_USE_LINK_PATH TRUE)
target_link_libraries(cryptominisat5_simple-bin
    ${cryptoms_exec_link_libs}
)
install(TARGETS cryptominisat5_simple-bin
    # EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat5_simple")

if (NOT ONLY_SIMPLE)
    set_target_properties(cryptominisat5-bin PROPERTIES
        OUTPUT_NAME cryptominisat5
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE)
    target_link_libraries(cryptominisat5-bin
        ${Boost_LIBRARIES}
        ${cryptoms_exec_link_libs}
    )
    install(TARGETS cryptominisat5-bin
        # EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat5")
endif()

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
