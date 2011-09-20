# check for SSE extensions
include(CheckCXXSourceRuns)
if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
 set(SSE_FLAGS)

 set(CMAKE_REQUIRED_FLAGS "-msse3")
 check_cxx_source_runs("
  #include <pmmintrin.h>

  int main()
  {
     __m128d a, b;
     double vals[2] = {0};
     a = _mm_loadu_pd(vals);
     b = _mm_hadd_pd(a,a);
     _mm_storeu_pd(vals, b);
     return 0;
  }"
  HAVE_SSE3)

 set(CMAKE_REQUIRED_FLAGS "-msse2")
 check_cxx_source_runs("
  #include <emmintrin.h>

  int main()
  {
      __m128d a, b;
      double vals[2] = {0};
      a = _mm_loadu_pd(vals);
      b = _mm_add_pd(a,a);
      _mm_storeu_pd(vals,b);
      return 0;
   }"
   HAVE_SSE2)

 set(CMAKE_REQUIRED_FLAGS "-msse")
 check_cxx_source_runs("
  #include <xmmintrin.h>
  int main()
  {
      __m128 a, b;
      float vals[4] = {0};
      a = _mm_loadu_ps(vals);
      b = a;
      b = _mm_add_ps(a,b);
      _mm_storeu_ps(vals,b);
      return 0;
  }"
  HAVE_SSE)

 set(CMAKE_REQUIRED_FLAGS)

 if(HAVE_SSE3)
  message(STATUS "Using SSE3 extensions")
  set(SSE_FLAGS "-msse3 -mfpmath=sse")
  set(HAVE_MMX 1)
 elseif(HAVE_SSE2)
  message(STATUS "Using SSE2 extensions")
  set(SSE_FLAGS "-msse2 -mfpmath=sse")
  set(HAVE_MMX 1)
 elseif(HAVE_SSE)
  message(STATUS "Using SSE extensions")
  set(SSE_FLAGS "-msse -mfpmath=sse")
  set(HAVE_MMX 1)
 endif()

 add_definitions(${SSE_FLAGS})
elseif(MSVC)
 check_cxx_source_runs("
  #include <emmintrin.h>

  int main()
  {
      __m128d a, b;
      double vals[2] = {0};
      a = _mm_loadu_pd(vals);
      b = _mm_add_pd(a,a);
      _mm_storeu_pd(vals,b);
      return 0;
   }"
   HAVE_SSE2)
 if( HAVE_SSE2)
  message(STATUS "Using SSE2 extensions")
  add_definitions( "/arch:SSE2 /fp:fast -D__SSE__ -D__SSE2__" )
 endif()
endif()

add_definitions("-DNDEBUG")

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES (sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES (sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES (dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILES (inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES (memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES (mm_malloc.h HAVE_MM_MALLOC)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)



include_directories(${PROJECT_SOURCE_DIR}/m4ri/src)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu99")

add_library(m4ri
    src/brilliantrussian.c src/misc.c src/packedmatrix.c src/grayflex.c src/strassen.c
    src/permutation.c src/trsm.c src/pls.c src/solve.c src/pls_mmpf.c src/echelonform.c
)


#noistall_headers = src/m4ri.h src/brilliantrussian.h src/misc.h src/packedmatrix.h src/grayflex.h src/strassen.h src/parity.h src/permutation.h src/config.h src/trsm.h src/pls.h src/solve.h src/pls_mmpf.h src/echelonform.h src/xor.h
