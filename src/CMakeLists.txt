# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
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

if(NOT WIN32)
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

    if(FINAL_PREDICTOR)
        add_cxx_flag_if_supported("-Wno-unused-parameter")
        add_cxx_flag_if_supported("-Wno-unused-but-set-variable")
    endif()
endif()
add_sanitize_flags()

if(ENABLE_TESTING)
    add_compile_definitions(CMS_TESTING_ENABLED)
    message(STATUS "Including gtest dir ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)

if(STATS OR FINAL_PREDICTOR)
    if(NOT Python3_EXECUTABLE)
        message(FATAL_ERROR "Unfortunately, the python interpreter is needed for statistics because of SQL text generation")
    endif()

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py cmsat_tablestructure.sql ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/cmsat_tablestructure.sql ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(tablestruct ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp)
endif()

if(FINAL_PREDICTOR)
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py src/predict/predictor_short.json ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/src/predict/predictor_short.json ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(pred_short ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp)

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py src/predict/predictor_long.json ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/src/predict/predictor_long.json ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(pred_long ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp)

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py src/predict/predictor_forever.json ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/src/predict/predictor_forever.json ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(pred_forever ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp)
endif()

# Needed for PicoSAT trace generation
add_compile_definitions(TRACE)

set(cryptoms_lib_files
    cnf.cpp
    probe.cpp
    oracle_use.cpp
    backbone.cpp
    propengine.cpp
    varreplacer.cpp
    clausecleaner.cpp
    occsimplifier.cpp
    gatefinder.cpp
    subsumestrengthen.cpp
    clauseallocator.cpp
    sccfinder.cpp
    solverconf.cpp
    distillerlong.cpp
    distillerlitrem.cpp
    distillerbin.cpp
    distillerlongwithimpl.cpp
    str_impl_w_impl.cpp
    solutionextender.cpp
    completedetachreattacher.cpp
    searcher.cpp
    solver.cpp
    hyperengine.cpp
    subsumeimplicit.cpp
    datasync.cpp
    reducedb.cpp
    intree.cpp
    searchstats.cpp
    xorfinder.cpp
    cardfinder.cpp
    cryptominisat_c.cpp
    sls.cpp
    sqlstats.cpp
    vardistgen.cpp
    ccnr.cpp
    ccnr_cms.cpp
    ccnr_oracle.cpp
    ccnr_oracle_pre.cpp
    lucky.cpp
    get_clause_query.cpp
    gaussian.cpp
    packedrow.cpp
    matrixfinder.cpp
    mpicosat/mpicosat.c
    mpicosat/version.c
    oracle/oracle.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

if(GPU)
    set(cryptoms_lib_files
        ${cryptoms_lib_files}
        gpuShareLib/Utils.cc
        gpuShareLib/Assigs.cu
        gpuShareLib/BaseTypes.cu
        gpuShareLib/Clauses.cu
        gpuShareLib/ClauseUpdates.cu
        gpuShareLib/CorrespArr.cu
        gpuShareLib/GpuClauseSharerImpl.cu
        gpuShareLib/GpuRunner.cu
        gpuShareLib/GpuUtils.cu
        gpuShareLib/Helper.cu
        gpuShareLib/Reported.cu
        gpuShareLib/Reporter.cu
    )
endif()

set(cryptoms_lib_link_libs "")

if(FINAL_PREDICTOR)
    set(cryptoms_lib_files
        ${cryptoms_lib_files}
#         predict/clustering_imp.cpp
        cl_predictors_xgb.cpp
        cl_predictors_py.cpp
        cl_predictors_lgbm.cpp
        cl_predictors_abs.cpp
    )
    set(cryptoms_lib_link_libs ${cryptoms_lib_link_libs}
        _lightgbm xgboost dmlc rabit rt ${Python3_LIBRARIES})
endif()

if(STATS_NEEDED)
    set(cryptoms_lib_files
        ${cryptoms_lib_files}
        community_finder.cpp
        satzilla_features_calc.cpp
        satzilla_features.cpp
    )
endif()

if(MPI_FOUND)
    set(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MPI_CXX_LIBRARIES})
endif()

if(BREAKID_LIBRARIES)
    set(cryptoms_lib_files ${cryptoms_lib_files} cms_breakid.cpp)
    set(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${BREAKID_LIBRARIES})
endif()

if(STATS)
    set(cryptoms_lib_files ${cryptoms_lib_files}
        sqlitestats.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
    )
    set(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${SQLITE3_LIBRARIES})
endif()

if(FINAL_PREDICTOR)
    set(cryptoms_lib_files ${cryptoms_lib_files}
        ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp
    )
endif()

add_library(cryptominisat5
    ${cryptoms_lib_files}
    cryptominisat.cpp
)

set(CRYPTOMINISAT_COMMON_PRIVATE_INCLUDES
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LOUVAIN_COMMUNITIES_INCLUDE_DIRS}
    ${MPI_INCLUDE_PATH}
    "${cadical_DIR}/include/"
    "${cadiback_DIR}/include/"
    $<INSTALL_INTERFACE:include>
    ${BREAKID_INCLUDE_DIRS}
)
set(CRYPTOMINISAT_COMMON_PUBLIC_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_include_directories(cryptominisat5
    PRIVATE ${CRYPTOMINISAT_COMMON_PRIVATE_INCLUDES}
    PUBLIC ${CRYPTOMINISAT_COMMON_PUBLIC_INCLUDES}
)

if(ENABLE_TESTING)
    target_include_directories(cryptominisat5 PRIVATE
        ${gtest_SOURCE_DIR}/include
        ${gtest_SOURCE_DIR}
    )
endif()

if(FINAL_PREDICTOR)
    target_include_directories(cryptominisat5 PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
    )
endif()

if(STATS)
    target_include_directories(cryptominisat5 PRIVATE ${SQLITE3_INCLUDE_DIR})
endif()

set_target_properties(cryptominisat5
    PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_property(TARGET cryptominisat5
    PROPERTY CUDA_ARCHITECTURES 35 50 72
)

if(STATS)
    add_dependencies(cryptominisat5
        tablestruct
    )
endif()

if(FINAL_PREDICTOR)
    add_dependencies(cryptominisat5
        pred_short
        pred_long
        pred_forever
    )
endif()

# indicate that we depend on pthread, and compile in the actual library
target_link_libraries(cryptominisat5
    PRIVATE
        ${cryptoms_lib_link_libs}
        ${LOUVAIN_COMMUNITIES_LIBRARIES}
        ${cadiback}
        ${cadical}
    PUBLIC
        Threads::Threads
        PkgConfig::GMP
)

if(NOT WIN32)
    set_target_properties(cryptominisat5 PROPERTIES
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
else()
    set_target_properties(cryptominisat5 PROPERTIES
        OUTPUT_NAME cryptominisat5win
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

if(IPASIR)
    add_library(ipasircryptominisat5
        ipasir.cpp
        ${cryptoms_lib_files}
        cryptominisat.cpp
    )
    target_include_directories(ipasircryptominisat5
        PRIVATE ${CRYPTOMINISAT_COMMON_PRIVATE_INCLUDES}
        PUBLIC ${CRYPTOMINISAT_COMMON_PUBLIC_INCLUDES}
    )
    target_link_libraries(ipasircryptominisat5
        PRIVATE
            Threads::Threads
            ${cryptoms_lib_link_libs}
            ${LOUVAIN_COMMUNITIES_LIBRARIES}
            ${cadiback}
            ${cadical}
        PUBLIC
            PkgConfig::GMP
    )
    set_target_properties(ipasircryptominisat5 PROPERTIES
        OUTPUT_NAME ipasircryptominisat5
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
    generate_export_header(ipasircryptominisat5)
    install(TARGETS ipasircryptominisat5
        EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cryptominisat5"
    )
endif()

cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat_c.h)
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/mpicosat/mpicosat.h)
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat.h)
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/solvertypesmini.h)
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/dimacsparser.h)
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/streambuffer.h)

# Public headers are fully known now; the CopyPublicHeaders target below reads
# this property, so it must be set before that target is created.
get_target_property(cryptominisat5_public_headers cryptominisat5 PUBLIC_HEADER)

# -----------------------------------------------------------------------------
# Symlink public headers into build directory include directory.
# Using symlinks (not copies) ensures #pragma once works correctly when the
# same header is reachable via both src/ and build/include/cryptominisat5/.
# The cryptominisat5Config.cmake we generate in the build directory depends on
# this.
# -----------------------------------------------------------------------------
set(HEADER_DEST "${PROJECT_BINARY_DIR}/include/cryptominisat5")
add_custom_target(CopyPublicHeaders ALL)
foreach(public_header ${cryptominisat5_public_headers})
    get_filename_component(HEADER_NAME ${public_header} NAME)
    add_custom_command(TARGET CopyPublicHeaders PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory
                               "${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E echo
                       "Symlinking ${HEADER_NAME} to ${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E remove
                               "${HEADER_DEST}/${HEADER_NAME}"
                       COMMAND ${CMAKE_COMMAND} -E create_symlink
                               "${public_header}"
                               "${HEADER_DEST}/${HEADER_NAME}"
                      )
endforeach()

generate_export_header(cryptominisat5)
install(TARGETS cryptominisat5
    EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cryptominisat5"
)

add_executable(cryptominisat5-bin
    main.cpp
    main_common.cpp
    main_exe.cpp
    signalcode.cpp
)

add_executable(oracle
    oracle/oracle_main.cpp
    oracle/oracle.cpp
)
target_include_directories(oracle PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/oracle
)
target_compile_features(oracle PRIVATE cxx_std_17)

set(cadical_SRC_DIR "${cadical_SRC_DIR}" CACHE PATH "CaDiCaL source dir (for cadical.hpp)")
add_executable(assump_fuzz_oracle
    oracle/assump_fuzz_oracle.cpp
    oracle/oracle.cpp
)
target_include_directories(assump_fuzz_oracle PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/oracle
    "${cadiback_DIR}/include/"
    "${cadical_SRC_DIR}"
)
target_link_libraries(assump_fuzz_oracle PRIVATE ${cadiback} ${cadical})
target_compile_features(assump_fuzz_oracle PRIVATE cxx_std_17)

if(MPI_FOUND)
    add_executable(cryptominisat5_mpi-bin
        main_mpi.cpp
        datasyncserver.cpp
    )
endif()

target_link_libraries(cryptominisat5-bin
    PRIVATE
        cryptominisat5
)

target_include_directories(cryptominisat5-bin PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(ZLIB_FOUND)
    target_include_directories(cryptominisat5-bin PRIVATE ${ZLIB_INCLUDE_DIR})
    target_link_libraries(cryptominisat5-bin PRIVATE ${ZLIB_LIBRARY})
endif()

##########################
### Deal with binaries
##########################
if(MPI_FOUND)
    set_target_properties(cryptominisat5_mpi-bin PROPERTIES
        OUTPUT_NAME cryptominisat5_mpi
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    target_link_libraries(cryptominisat5_mpi-bin
        cryptominisat5
        ${MPI_C_LIBRARIES}
    )
    install(TARGETS cryptominisat5_mpi-bin
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    set(CPACK_PACKAGE_EXECUTABLES "cryptominisat5_mpi")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set_target_properties(cryptominisat5-bin PROPERTIES
        OUTPUT_NAME cryptominisat5
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    target_link_options(cryptominisat5-bin PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s EXPORTED_RUNTIME_METHODS='[\"callMain\", \"ccall\", \"cwrap\", \"FS\", \"print\"]'"
        "SHELL:-s FORCE_FILESYSTEM=1"
    )
else()
    set_target_properties(cryptominisat5-bin PROPERTIES
        OUTPUT_NAME cryptominisat5
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

install(TARGETS cryptominisat5-bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
set(CPACK_PACKAGE_EXECUTABLES "cryptominisat5-bin" "cryptominisat5")
