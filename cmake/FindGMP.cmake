find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GMP QUIET gmp)
endif()

set(GMP_DEFINITIONS ${PC_GMP_CFLAGS_OTHER})

find_path(
    GMP_INCLUDE_DIR
    NAMES gmpxx.h
    HINTS ${PC_GMP_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)
set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})
set(GMP_PC_ADD_CFLAGS "-I${GMP_INCLUDE_DIR}")

# scikit-build-core sets CMAKE_FIND_LIBRARY_SUFFIXES to ".a" for manylinux
# wheel builds to encourage static linking, but gmp-devel on RHEL/AlmaLinux
# only ships libgmp.so (no libgmp.a). Add .so as fallback — but only when not
# doing a fully static executable build (STATICCOMPILE=ON), since the static
# linker rejects .so files.
set(_gmp_old_suffixes "${CMAKE_FIND_LIBRARY_SUFFIXES}")
if(NOT STATICCOMPILE)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so" ".dylib" ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

find_library(
    GMPXX_LIBRARY
    NAMES gmpxx
    HINTS ${PC_GMP_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

find_library(
    GMP_LIBRARY
    NAMES gmp
    HINTS ${PC_GMP_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

set(CMAKE_FIND_LIBRARY_SUFFIXES "${_gmp_old_suffixes}")
set(GMP_LIBRARIES ${GMPXX_LIBRARY} ${GMP_LIBRARY})
set(GMP_PC_ADD_LIBS "-lgmpxx -lgmp")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP
  DEFAULT_MSG
  GMPXX_LIBRARY
  GMP_LIBRARY
  GMP_INCLUDE_DIR)
mark_as_advanced(GMPXX_LIBRARY GMP_LIBRARY GMP_INCLUDE_DIR)
