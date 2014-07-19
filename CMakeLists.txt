cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)

set(CPACK_PACKAGE_NAME "cryptominisat")
set(CPACK_PACKAGE_VENDOR "www.msoos.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A general-purpose award-winning SAT solver")
set(CPACK_PACKAGE_VERSION "4.2.0")
set(CPACK_PACKAGE_VERSION_MAJOR "4")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")

# -----------------------------------------------------------------------------
# Make RelWithDebInfo the default build type if otherwise not set
# -----------------------------------------------------------------------------
set(build_types Debug Release RelWithDebInfo MinSizeRel)
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "You can choose the type of build, options are:${build_types}")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE String
        "Options are ${build_types}"
        FORCE
    )

    # Provide drop down menu options in cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${build_types})
endif()
message(STATUS "Doing a ${CMAKE_BUILD_TYPE} build")

# -----------------------------------------------------------------------------
# Option to enable/disable assertions
# -----------------------------------------------------------------------------

# Filter out definition of NDEBUG from the default build configuration flags.
# We will add this ourselves if we want to disable assertions
foreach (build_config ${build_types})
    string(TOUPPER ${build_config} upper_case_build_config)
    foreach (language CXX C)
        set(VAR_TO_MODIFY "CMAKE_${language}_FLAGS_${upper_case_build_config}")
        string(REGEX REPLACE "(^| )[/-]D *NDEBUG($| )"
                             " "
                             replacement
                             "${${VAR_TO_MODIFY}}"
              )
        #message("Original (${VAR_TO_MODIFY}) is ${${VAR_TO_MODIFY}} replacement is ${replacement}")
        set(${VAR_TO_MODIFY} "${replacement}" CACHE STRING "Default flags for ${build_config} configuration" FORCE)
    endforeach()
endforeach()

PROJECT(cryptominisat4)

option(ENABLE_ASSERTIONS "Build with assertions enabled" ON)
if (ENABLE_ASSERTIONS)
    # NDEBUG was already removed.
else()
    # Note this definition doesn't appear in the cache variables.
    add_definitions(-DNDEBUG)
endif()

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake) #m4-extra contains some library search cmake stuff

macro(add_cxx_flag flagname)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}")
endmacro()

option(SANITIZE "Use Clang sanitizers. This will force using clang++ as the compiler" OFF)
if (SANITIZE)
    SET (CMAKE_CXX_COMPILER "clang++")
    #add_cxx_flag("-fsanitize=address")
    add_cxx_flag("-fsanitize=integer")
    #add_cxx_flag("-fsanitize=undefined")

    #add_cxx_flag("-fsanitize=null")
    #add_cxx_flag("-fsanitize=alignment")
    #add_cxx_flag("-fsanitize=bounds")
    #add_cxx_flag("-fno-sanitize-recover")

    add_cxx_flag("-fsanitize=return")
    add_cxx_flag("-fsanitize=bounds")
    add_cxx_flag("-fsanitize=float-divide-by-zero")
    add_cxx_flag("-fsanitize=integer-divide-by-zero")
    add_cxx_flag("-fsanitize=unsigned-integer-overflow")
    add_cxx_flag("-fsanitize=signed-integer-overflow")
    add_cxx_flag("-fsanitize=bool")
    add_cxx_flag("-fsanitize=enum")
    add_cxx_flag("-fsanitize=float-cast-overflow")
    add_cxx_flag("-Wno-bitfield-constant-conversion")
    #add_cxx_flag("-Weverything")
    add_cxx_flag("-Wshadow")
    #add_cxx_flag("-Wshorten-64-to-32")
    #add_cxx_flag("-Wweak-vtables")
    add_cxx_flag("-Wextra-semi")
    #add_cxx_flag("-Wsign-conversion")
    add_cxx_flag("-Wdeprecated")
    #add_cxx_flag("-fPIC")
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -g")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -mtune=native")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -g0 -DNDEBUG -mtune=native")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")

include(CheckCXXCompilerFlag)
macro(add_cxx_flag_if_supported flagname)
  check_cxx_compiler_flag("${flagname}" HAVE_FLAG_${flagname})

  if(HAVE_FLAG_${flagname})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}")
    message(STATUS "flag ${flagname} supported")
  else()
    message(STATUS "flag ${flagname} NOT supported")
  endif()
endmacro()

add_cxx_flag_if_supported("-Wall")
add_cxx_flag_if_supported("-Wextra")
add_cxx_flag_if_supported("-Wunused")
add_cxx_flag_if_supported("-pedantic")
add_cxx_flag_if_supported("-Wsign-compare")
add_cxx_flag_if_supported("-fno-omit-frame-pointer")
add_cxx_flag_if_supported("-Wtype-limits")
add_cxx_flag_if_supported("-Wuninitialized")
add_cxx_flag_if_supported("-Wno-deprecated")
add_cxx_flag_if_supported("-Wstrict-aliasing")
add_cxx_flag_if_supported("-Wpointer-arith")
add_cxx_flag_if_supported("-Wheader-guard")
#add_cxx_flag_if_supported("-fopt-info")

# -----------------------------------------------------------------------------
# Uncomment these for static compilation under Linux (messes up Valgrind)
# -----------------------------------------------------------------------------
option(STATICCOMPILE "Compile to static executable (only works on linux)" OFF)
if(STATICCOMPILE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(Boost_USE_STATIC_LIBS ON)
    set(CMAKE_EXE_LINKER_FLAGS "-static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")

    set(NOMYSQL ON)
else()
    add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

option(MORE_DEBUG "Use more debug flags" OFF)
IF(MORE_DEBUG)
    add_definitions(-DMORE_DEBUG)
endif()

option(ENABLE_TESTING "Enable testing" OFF)

# -----------------------------------------------------------------------------
# Add GIT version
# -----------------------------------------------------------------------------
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_get_exact_tag(GIT_TAG)

set (boost_components "")
if (ENABLE_TESTING)
    set(boost_components ${boost_components} unit_test_framework)
endif()
set(boost_components ${boost_components} program_options)
find_package( Boost 1.46 COMPONENTS ${boost_components})

option(SIMPLE "Very simplistic executable -- no Boost needed" OFF)
if(NOT Boost_FOUND)
    set(SIMPLE ON)
    set(ENABLE_TESTING OFF)
endif()

find_package (Threads REQUIRED)
find_package (TBB)
if (TBB_FOUND)
    message(STATUS "OK, found Intel TBB")
    add_definitions(-DUSE_TBB)
else()
    message(STATUS "Did not find Intel Threading Buliding Blocks. Threads will be slower to allocate memory")
endif()

option(NOSTATS "Don't use statistics at all" OFF)
if (NOT NOSTATS)
    add_definitions( -DSTATS_NEEDED )
    if (NOT NOMYSQL)
        find_package(MySQL)
        IF (MYSQL_FOUND)
            MESSAGE(STATUS "OK, Found MySQL!")
            include_directories(${MySQL_INCLUDE_DIR})
            link_directories(${MySQL_LIB_DIR})
            add_definitions( -DUSE_MYSQL )
        else ()
            MESSAGE(STATUS "WARNING: Did not find MySQL, MySQL support will be disabled")
        endif()
    endif()

    if (NOT NOSQLITE)
        find_package(Sqlite3)
        IF (SQLITE3_FOUND)
            MESSAGE(STATUS "OK, Found Sqlite3!")
            include_directories(${SQLITE3_INCLUDE_DIR})
            add_definitions( -DUSE_SQLITE3 )
        else ()
            MESSAGE(STATUS "WARNING: Did not find Sqlite3, Sqlite3 support will be disabled")
        endif ()
    endif()
ELSE ()
    MESSAGE(STATUS "Not compiling detailed statistics. Leads to faster system")
ENDIF ()

# -----------------------------------------------------------------------------
# Look for ZLIB (For reading zipped CNFs)
# -----------------------------------------------------------------------------
if (NOT NOZLIB)
    find_package(ZLIB)
    IF (ZLIB_FOUND)
        MESSAGE(STATUS "OK, Found ZLIB!")
        include_directories(${ZLIB_INCLUDE_DIR})
        link_directories(${ZLIB_LIB_DIR})
        add_definitions( -DUSE_ZLIB )
    ELSE (ZLIB_FOUND)
        MESSAGE(STATUS "WARNING: Did not find ZLIB, gzipped file support will be disabled")
    ENDIF (ZLIB_FOUND)

    find_package(Valgrind)
    if (VALGRIND_FOUND)
        message(STATUS "OK, Found Valgrind. Using valgrind client requests to mark freed clauses in pool as undefined")
        add_definitions(-DUSE_VALGRIND)
        include_directories(${VALGRIND_INCLUDE_DIR})
    else()
        message(STATUS "Cannot find valgrind, we will not be able to mark memory pool objects as undefined")
    endif()
endif()

#M4RI
if (NOT NOM4RI)
    find_package(M4RI)
    IF (M4RI_FOUND)
        MESSAGE(STATUS "OK, Found M4RI!")
        add_definitions( -DUSE_M4RI )
    ELSE (M4RI_FOUND)
        MESSAGE(WARNING "Did not find M4RI, XOR detection&manipulation disabled")
    ENDIF (M4RI_FOUND)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

macro(cmsat_add_public_header LIBTARGET HEADER)
    set(HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}")
    if(EXISTS "${HEADER_PATH}")
        # Get existing PUBLIC_HEADER
        get_target_property(EXISTING_PUBLIC_HEADERS ${LIBTARGET} PUBLIC_HEADER)
        if(EXISTING_PUBLIC_HEADERS)
            list(APPEND EXISTING_PUBLIC_HEADERS "${HEADER_PATH}")
        else()
            # Do not append to empty list
            set(EXISTING_PUBLIC_HEADERS "${HEADER_PATH}")
        endif()
        set_target_properties(${LIBTARGET}
                              PROPERTIES
                              PUBLIC_HEADER "${EXISTING_PUBLIC_HEADERS}"
                             )
    else()
        message(FATAL_ERROR "Cannot add public header, file ${HEADER_PATH} does not exist.")
    endif()
endmacro()

if (ENABLE_TESTING)
    enable_testing()
endif()

# -----------------------------------------------------------------------------
# Provide an export name to be used by targets that wish to export themselves.
# -----------------------------------------------------------------------------
set(CRYPTOMINISAT4_EXPORT_NAME "cryptominisat4Targets")

add_subdirectory(cryptominisat4 cmsat4-src)

# -----------------------------------------------------------------------------
# Look for python
# -----------------------------------------------------------------------------
find_package (PythonInterp 2.7)
find_package (PythonLibs 2.7)
if (PYTHON_EXECUTABLE AND PYTHON_LIBRARY)
#     message(STATUS "PYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}")
#     message(STATUS "PYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}")
#     message(STATUS "PYTHON_INCLUDE_DIR:FILEPATH=${PYTHON_INCLUDE_DIR}")
#     message(STATUS "PYTHONLIBS_VERSION_STRING=${PYTHONLIBS_VERSION_STRING}")
    message(STATUS "OK, found python interpreter and libs, building plugin")
    add_subdirectory(python py-lib)
else()
    message(WARNING "Cannot find python interpreter and libs, not building python plugin")
endif()

# -----------------------------------------------------------------------------
# Add uninstall target for makefiles
# -----------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

if (ENABLE_TESTING)
    add_test (basic_test tests/basic_test)
    add_test (assump_test tests/assump_test)
    add_test (readme_test tests/readme_test)
    add_test (heap_test tests/heap_test)
    add_test (clause_test tests/clause_test)

    message(STATUS "Testing is enabled")
    set(UNIT_TEST_EXE_SUFFIX "Tests" CACHE STRING "Suffix for Unit test executable")
    add_subdirectory(tests)

else()
    message(WARNING "Testing is disabled")
endif()

# -----------------------------------------------------------------------------
# Export our targets so that other CMake based projects can interface with
# the build of cryptominisat4 in the build-tree
# -----------------------------------------------------------------------------
set(CRYPTOMINISAT4_TARGETS_FILENAME "cryptominisat4Targets.cmake")
set(CRYPTOMINISAT4_CONFIG_FILENAME "cryptominisat4Config.cmake")

# Export targets
export(TARGETS libcryptominisat4 cryptominisat FILE "${PROJECT_BINARY_DIR}/${CRYPTOMINISAT4_TARGETS_FILENAME}")

# Create cryptominisat4Config file
set(EXPORT_TYPE "Build-tree")
set(CONF_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include")
configure_file(cryptominisat4Config.cmake.in
    "${CMAKE_BINARY_DIR}/${CRYPTOMINISAT4_CONFIG_FILENAME}" @ONLY
)

if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/cryptominisat4)
endif()
set(CRYPTOMINISAT4_INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
    "Installation directory for cryptominisat4 CMake files")

# Create cryptominisat4Config file
set(EXPORT_TYPE "installed")
set(CONF_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
configure_file(cryptominisat4Config.cmake.in
   "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT4_CONFIG_FILENAME}" @ONLY
)

install(FILES
    "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT4_CONFIG_FILENAME}"
    DESTINATION "${CRYPTOMINISAT4_INSTALL_CMAKE_DIR}"
)

# Install the export set for use with the install-tree
install(EXPORT ${CRYPTOMINISAT4_EXPORT_NAME} DESTINATION
    "${CRYPTOMINISAT4_INSTALL_CMAKE_DIR}"
)

set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/.git/"
  "/scripts.*/"
  "~$"
  "/m4/"
  "/cactusplots/"
  "/debugbuild/"
  "/gitstats/"
  "/matrixes/"
  "/man/"
  "/config/"
  "/utils/"
  "/tests/"
  "/desc/"
  "/.kdev4/"
  "/web/file_creator.py"
  "/web/dat*"
  "/web/dygraphs_backup/"
  "/web/dygraphs-old/"
  "/web/jquery/docs/"
  "/web/jquery/examples/"
  "/web/scriptaculous-js-1.9.0/test/"
  ".gitignore$"
  ".git$"
  "tar.gz$"
  "tar.bz2$"
  ".kdev4$"
  "copy.sh$"
  ".travis.yml"
  ".travis-cmake.sh"
  ".csv$"
  ".tmp$"
  ".directory$"
  "HOWTO-ANALYSIS"
  "/web/dygraphs/auto_tests"
    "/web/dygraphs/closure-todo.txt"
    "/web/dygraphs/common"
    "/web/dygraphs/compile-with-closure.sh"
    "/web/dygraphs/dashed-canvas.js"
    "/web/dygraphs/datahandler"
    "/web/dygraphs/data.js"
    "/web/dygraphs/docs"
    "/web/dygraphs/dygraph-canvas.js"
    "/web/dygraphs/dygraph-dev.js"
    "/web/dygraphs/dygraph-exports.js"
    "/web/dygraphs/dygraph-externs.js"
    "/web/dygraphs/dygraph-gviz.js"
    "/web/dygraphs/dygraph-interaction-model.js"
    "/web/dygraphs/dygraph-internal.externs.js"
    "/web/dygraphs/dygraph.js"
    "/web/dygraphs/dygraph-layout.js"
    "/web/dygraphs/dygraph-options.js"
    "/web/dygraphs/dygraph-options-reference.js"
    "/web/dygraphs/dygraph-plugin-base.js"
    "/web/dygraphs/dygraph-plugin-install.js"
    "/web/dygraphs/dygraph-tickers.js"
    "/web/dygraphs/dygraph-types.js"
    "/web/dygraphs/dygraph-utils.js"
    "/web/dygraphs/excanvas.js"
    "/web/dygraphs/experimental"
    "/web/dygraphs/extras"
    "/web/dygraphs/file-size-stats.sh"
    "/web/dygraphs/gadget.xml"
    "/web/dygraphs/gallery"
    "/web/dygraphs/generate-combined.sh"
    "/web/dygraphs/.dygraph-combined-clean.js"
    "/web/dygraphs/generate-documentation.py"
    "/web/dygraphs/generate-download.py"
    "/web/dygraphs/generate-jar.sh"
    "/web/dygraphs/generate-jsdoc.sh"
    "/web/dygraphs/gviz-api.js"
    "/web/dygraphs/gwt"
    "/web/dygraphs/jsdoc-toolkit"
    "/web/dygraphs/jshint"
    "/web/dygraphs/jsTestDriver.conf"
    "/web/dygraphs/LICENSE.txt"
    "/web/dygraphs/lint.sh"
    "/web/dygraphs/Makefile"
    "/web/dygraphs/NOTES"
    "/web/dygraphs/phantom-driver.js"
    "/web/dygraphs/phantom-perf.js"
    "/web/dygraphs/plugins"
    "/web/dygraphs/push-to-web.sh"
    "/web/dygraphs/README"
    "/web/dygraphs/release.sh"
    "/web/dygraphs/releases.json"
    "/web/dygraphs/screenshot.png"
    "/web/dygraphs/stacktrace.js"
    "/web/dygraphs/tests"
    "/web/dygraphs/test.sh"
    "/web/dygraphs/thumbnail.png"
    "/web/dygraphs/yuicompressor-2.4.2.jar"
    ".gitmodules$"
)
include(CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
