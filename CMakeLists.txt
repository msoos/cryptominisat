cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)

# -----------------------------------------------------------------------------
# Make RelWithDebInfo the default build type if otherwise not set
# -----------------------------------------------------------------------------
IF(DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
  CACHE STRING "Choose the type of build: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
  "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

PROJECT(cryptominisat4)
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

SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++11 -pthread -g -O3 -mtune=native")
SET(CMAKE_CXX_FLAGS_DEBUG "-std=c++11 -pthread -O0 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -pthread -O3 -g0 -DNDEBUG -mtune=native")


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

option(ENABLE_TESTING "Enable testing" OFF)
if(ENABLE_TESTING)
    message(STATUS "Testing is enabled")
else()
    message(WARNING "Testing is disabled")
endif()

# -----------------------------------------------------------------------------
# Uncomment these for static compilation under Linux (messes up Valgrind)
# -----------------------------------------------------------------------------
option(STATICCOMPILE "Compile to static executable (only works on linux)" OFF)
if(STATICCOMPILE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(Boost_USE_STATIC_LIBS ON)
    #set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static-libgcc -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    set(NOMYSQL ON)
    set(ENABLE_TESTING OFF)
endif()

option(MORE_DEBUG "Use more debug flags" OFF)
IF(MORE_DEBUG)
    add_definitions(-DMORE_DEBUG)
endif()

# -----------------------------------------------------------------------------
# Add GIT version
# -----------------------------------------------------------------------------
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

option(SIMPLE "Very simplistic executable -- no Boost needed")
#set(SIMPLE "ON")
if (NOT SIMPLE)
    if (ENABLE_TESTING)
        find_package( Boost 1.46 REQUIRED COMPONENTS program_options unit_test_framework)
    else()
        find_package( Boost 1.46 REQUIRED COMPONENTS program_options)
    endif()
else()
    set(NOTESTS "ON")
    set(NOZLIB "ON")
endif()

find_package (Threads REQUIRED)
find_package (TBB)
if (TBB_FOUND)
    message(STATUS "OK, found Intel TBB")
    add_definitions(-DUSE_TBB)
else()
    message(STATUS "Did not find Intel Threading Buliding Blocks. Threads will be slower to allocate memory")
endif()

option(NOSTATS "Don't use statistics at all")
if (NOT NOSTATS)
    add_definitions( -DSTATS_NEEDED )
    if (NOT NOMYSQL)
        find_package(MySQL)
        IF (MYSQL_FOUND)
            MESSAGE(STATUS "OK, Found MySQL!")
            include_directories(${MySQL_INCLUDE_DIR})
            link_directories(${MySQL_LIB_DIR})
            add_definitions( -DUSE_MYSQL )
        else (MYSQL_FOUND)
            MESSAGE(STATUS "WARNING: Did not find MySQL, MySQL support will be disabled")
        endif (MYSQL_FOUND)
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

# -----------------------------------------------------------------------------
# Provide an export name to be used by targets that wish to export themselves.
# -----------------------------------------------------------------------------
set(CRYPTOMINISAT4_EXPORT_NAME "cryptominisat4Targets")

# -----------------------------------------------------------------------------
# Compile all subdirs: solver, utilities, fuzzers, etc.
# -----------------------------------------------------------------------------
add_subdirectory(cryptominisat4 cmsat4-src)

# -----------------------------------------------------------------------------
# Add uninstall target for makefiles
# -----------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

if (ENABLE_TESTING)
    enable_testing()
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

set(CPACK_PACKAGE_NAME "cryptominisat")
set(CPACK_PACKAGE_VENDOR "www.msoos.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A general-purpose award-winning SAT solver")
set(CPACK_PACKAGE_VERSION "4.1.0")
set(CPACK_PACKAGE_VERSION_MAJOR "4")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/.git/"
  "/scripts.*/"
  "~$"
  "${CPACK_SOURCE_IGNORE_FILES}"
  "/m4/"
  "/cactusplots/"
  "/debugbuild/"
  "/gitstats/"
  "/matrixes/"
  "/man/"
  "/desc/"
  "/.kdev4/"
  "/web/file_creator.py"
  "/web/dat*"
  "/web/dygraphs-.*"
  "/web/dygraphs_*"
  "/web/jquery/docs/"
  "/web/jquery/examples/"
  "/web/scriptaculous-js-1.9.0/test/"
  ".gitignore"
  ".git$"
  "tar.gz$"
  "tar.bz2$"
  ".kdev4$"
  ".directory$"
  "HOWTO-ANALYSIS"
)
include(CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
