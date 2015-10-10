cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)
if (CMAKE_MAJOR_VERSION GREATER 2)
    if(POLICY CMP0022)
        cmake_policy(SET CMP0022 OLD)
    endif()
    if(POLICY CMP0048)
        cmake_policy(SET CMP0048 OLD)
    endif()
    if(POLICY CMP0046)
        cmake_policy(SET CMP0046 OLD)
    endif()

    if(POLICY CMP0026)
        cmake_policy(SET CMP0026 OLD)
    endif()
endif()

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

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake) #m4-extra contains some library search cmake stuff

macro(add_cxx_flag flagname)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}")
endmacro()

option(FEEDBACKFUZZ "Use Clang coverage sanitizers" OFF)
if (FEEDBACKFUZZ)
    SET (CMAKE_CXX_COMPILER "clang++")
    add_cxx_flag("-fsanitize=address")
    add_cxx_flag("-fsanitize-coverage=edge,indirect-calls,8bit-counters")
endif()

option(SANITIZE "Use Clang sanitizers. This will force using clang++ as the compiler" OFF)
if (SANITIZE)
    SET (CMAKE_CXX_COMPILER "clang++")

    #add_cxx_flag("-fsanitize=address")
    add_cxx_flag("-fsanitize=integer")
    #add_cxx_flag("-fsanitize=undefined")

    #add_cxx_flag("-fsanitize=null")
    add_cxx_flag("-fsanitize=alignment")
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
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()

include(CheckCXXCompilerFlag)
macro(add_cxx_flag_if_supported flagname)
  check_cxx_compiler_flag("${flagname}" HAVE_FLAG_${flagname})

  if(HAVE_FLAG_${flagname})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}")
  endif()
endmacro()

option(ENABLE_ASSERTIONS "Build with assertions enabled" ON)
if (ENABLE_ASSERTIONS)
    # NDEBUG was already removed.
else()
    # Note this definition doesn't appear in the cache variables.
    add_definitions(-DNDEBUG)
    add_cxx_flag_if_supported("-fno-stack-protector")
    add_definitions(-D_FORTIFY_SOURCE=0)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -g")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -mtune=native")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g0 -DNDEBUG -mtune=native")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0")

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
#add_cxx_flag_if_supported("-fno-omit-frame-pointer")
add_cxx_flag_if_supported("-Wtype-limits")
add_cxx_flag_if_supported("-Wuninitialized")
add_cxx_flag_if_supported("-Wno-deprecated")
add_cxx_flag_if_supported("-Wstrict-aliasing")
add_cxx_flag_if_supported("-Wpointer-arith")
add_cxx_flag_if_supported("-Wheader-guard")
add_cxx_flag_if_supported("-fvisibility=hidden")
add_cxx_flag_if_supported("-Wpointer-arith")
add_cxx_flag_if_supported("-Wformat-nonliteral")
add_cxx_flag_if_supported("-Winit-self")
add_cxx_flag_if_supported("-Wparentheses")
add_cxx_flag_if_supported("-Wunreachable-code")
add_cxx_flag_if_supported("-ggdb3")
# add_cxx_flag_if_supported("-flto") # slow compile and not enough benefits
add_cxx_flag("-fPIC")
#add_cxx_flag_if_supported("-fno-exceptions")


option(COVERAGE "Build with coverage check" OFF)
if (COVERAGE)
    add_cxx_flag("--coverage")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_EXE_LINKER_FLAGS " -Wl,-O1 -Wl,--discard-all -Wl,--build-id=sha1")
endif()

# -----------------------------------------------------------------------------
# Uncomment these for static compilation under Linux (messes up Valgrind)
# -----------------------------------------------------------------------------
option(STATICCOMPILE "Compile to static executable (only works on linux)" OFF)
if(STATICCOMPILE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")

    set(NOMYSQL ON)
else()
    add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

option(SLOW_DEBUG "Use more debug flags" OFF)
IF(SLOW_DEBUG)
    add_definitions(-DSLOW_DEBUG)
endif()

option(ENABLE_TESTING "Enable testing" OFF)

# -----------------------------------------------------------------------------
# Add GIT version
# -----------------------------------------------------------------------------
function(SetVersionNumber PREFIX VERSION_MAJOR VERSION_MINOR VERSION_PATCH)
  set(${PREFIX}_VERSION_MAJOR ${VERSION_MAJOR} PARENT_SCOPE)
  set(${PREFIX}_VERSION_MINOR ${VERSION_MINOR} PARENT_SCOPE)
  set(${PREFIX}_VERSION_PATCH ${VERSION_PATCH} PARENT_SCOPE)
  set(${PREFIX}_VERSION
        "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
        PARENT_SCOPE)
endfunction()

find_program (GIT_EXECUTABLE git)
if (GIT_EXECUTABLE)
  include(GetGitRevisionDescription)
  get_git_head_revision(GIT_REFSPEC GIT_SHA1)
  MESSAGE(STATUS "GIT hash found: ${GIT_SHA1}")
else()
  set(GIT_SHA "GIT-hash-notfound")
endif()
set(CMS_FULL_VERSION "4.5.3")

string(REPLACE "." ";" CMS_FULL_VERSION_LIST ${CMS_FULL_VERSION})
SetVersionNumber("PROJECT" ${CMS_FULL_VERSION_LIST})
MESSAGE(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")
MESSAGE(STATUS "PROJECT_VERSION_MAJOR: ${PROJECT_VERSION_MAJOR}")
MESSAGE(STATUS "PROJECT_VERSION_MINOR: ${PROJECT_VERSION_MINOR}")
MESSAGE(STATUS "PROJECT_VERSION_PATCH: ${PROJECT_VERSION_PATCH}")

if (ENABLE_TESTING)
    set (boost_components "")
    set(boost_components ${boost_components} unit_test_framework)
    find_package( Boost 1.46 COMPONENTS ${boost_components} REQUIRED)
endif()

option(ALSO_BUILD_STATIC_LIB "Also build the static library" ON)


find_package (Threads REQUIRED)

option(STATS "Don't use statistics at all" OFF)
if (STATS)
    add_definitions( -DSTATS_NEEDED )
    if (NOT NOMYSQL)
        find_package(MySQL)
        IF (MYSQL_FOUND)
            MESSAGE(STATUS "OK, Found MySQL!")
            include_directories(${MySQL_INCLUDE_DIR})
            link_directories(${MySQL_LIB_DIR})
            add_definitions( -DUSE_MYSQL )
            set(USING_MYSQL ON)
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
            set(USING_SQLITE ON)
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
option(NOZLIB "Don't use zlib" OFF)
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
endif()

find_package(Valgrind)
if (VALGRIND_FOUND)
    message(STATUS "OK, Found Valgrind. Using valgrind client requests to mark freed clauses in pool as undefined")
    add_definitions(-DUSE_VALGRIND)
    include_directories(${VALGRIND_INCLUDE_DIR})
else()
    message(STATUS "Cannot find valgrind, we will not be able to mark memory pool objects as undefined")
endif()

#M4RI
option(NOM4RI "Don't use m4ri" OFF)
if (NOT NOM4RI)
    find_package(M4RI)
    IF (M4RI_FOUND)
        MESSAGE(STATUS "OK, Found M4RI lib at ${M4RI_LIBRARIES} and includes at ${M4RI_INCLUDE_DIRS}")
        add_definitions( -DUSE_M4RI )
    ELSE (M4RI_FOUND)
        MESSAGE(WARNING "Did not find M4RI, XOR detection&manipulation disabled")
    ENDIF (M4RI_FOUND)
endif()

#query definitions
get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
set(COMPILE_DEFINES)
foreach( d ${DirDefs} )
    # message( STATUS "Found Define: " ${d} )
    set(COMPILE_DEFINES "${COMPILE_DEFINES} -D${d}")
endforeach()
message(STATUS "All defines at startup: ${COMPILE_DEFINES}")


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

macro(cmsat_add_public_header LIBTARGET HEADER)
    if(EXISTS "${HEADER}")
        # Get existing PUBLIC_HEADER
        get_target_property(EXISTING_PUBLIC_HEADERS ${LIBTARGET} PUBLIC_HEADER)
        if(EXISTING_PUBLIC_HEADERS)
            list(APPEND EXISTING_PUBLIC_HEADERS "${HEADER}")
        else()
            # Do not append to empty list
            set(EXISTING_PUBLIC_HEADERS "${HEADER}")
        endif()
        set_target_properties(${LIBTARGET}
                              PROPERTIES
                              PUBLIC_HEADER "${EXISTING_PUBLIC_HEADERS}"
                             )
    else()
        message(FATAL_ERROR "Cannot add public header, file ${HEADER} does not exist.")
    endif()
endmacro()

if (ENABLE_TESTING)
    enable_testing()
endif()

# -----------------------------------------------------------------------------
# Provide an export name to be used by targets that wish to export themselves.
# -----------------------------------------------------------------------------
set(CRYPTOMINISAT4_EXPORT_NAME "cryptominisat4Targets")

add_subdirectory(src cmsat4-src)

# -----------------------------------------------------------------------------
# Look for python
# -----------------------------------------------------------------------------
option(ENABLE_PYTHON_INTERFACE "Enable Python interface" ON)

if (ENABLE_PYTHON_INTERFACE)
    find_package (PythonInterp 2.7)
    find_package (PythonLibs 2.7)
    if (PYTHON_EXECUTABLE AND PYTHON_LIBRARY AND PYTHON_INCLUDE_DIRS)
#     message(STATUS "PYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}")
#     message(STATUS "PYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}")
#     message(STATUS "PYTHON_INCLUDE_DIR:FILEPATH=${PYTHON_INCLUDE_DIR}")
#     message(STATUS "PYTHONLIBS_VERSION_STRING=${PYTHONLIBS_VERSION_STRING}")
        message(STATUS "OK, found python interpreter, libs and header files -> building python plugin")
        add_subdirectory(python py-lib)
    else()
        message(WARNING "Cannot find python interpreter, libs and header files -> not building python plugin")
    endif()
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
    add_test (NAME basic_test COMMAND tests/basic_test --log_level=test_suite)
    add_test (NAME assump_test COMMAND tests/assump_test --log_level=test_suite)
    add_test (NAME readme_test COMMAND tests/readme_test --log_level=test_suite)
    add_test (NAME heap_test COMMAND tests/heap_test --log_level=test_suite)
    add_test (NAME clause_test COMMAND tests/clause_test --log_level=test_suite)
    # too unstable to add (depends on CPU)
    #if (NOT STATS AND NOT SLOW_DEBUG)
        #add_test (NAME library_speed_test COMMAND tests/library_speed_test --log_level=test_suite)
    #endif()

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
set(CRYPTOMINISAT4_STATIC_DEPS
    ${SQLITE3_LIBRARIES}
    ${MYSQL_LIB}
    m4ri
)

# Export targets
set(MY_TARGETS libcryptominisat4 cryptominisat4_simple)
if (NOT ONLY_SIMPLE)
    set(MY_TARGETS ${MY_TARGETS} cryptominisat4)
endif()
if (ALSO_BUILD_STATIC_LIB)
    set(MY_TARGETS ${MY_TARGETS} libcryptominisat4_static)
endif()
export(TARGETS ${MY_TARGETS} FILE "${CMAKE_CURRENT_BINARY_DIR}/${CRYPTOMINISAT4_TARGETS_FILENAME}")

# Create cryptominisat4Config file
set(EXPORT_TYPE "Build-tree")
set(CONF_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include")
configure_file(cryptominisat4Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${CRYPTOMINISAT4_CONFIG_FILENAME}" @ONLY
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
   "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT4_CONFIG_FILENAME}" @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT4_CONFIG_FILENAME}"
    DESTINATION "${CRYPTOMINISAT4_INSTALL_CMAKE_DIR}"
)

# Install the export set for use with the install-tree
install(EXPORT ${CRYPTOMINISAT4_EXPORT_NAME} DESTINATION
    "${CRYPTOMINISAT4_INSTALL_CMAKE_DIR}"
)

