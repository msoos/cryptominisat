# SYNOPSIS
#
#   AX_M4RI_CFLAGS
#
# DESCRIPTION
#
#   Defines M4RI_CFLAGS which contains the CFLAGS used for building
#   the copy of M4RI we're linking against.
#
# LAST MODIFICATION
#
# 2011-10-03 
#
# COPYLEFT
#
#   Copyright (c) 2009,2010 Martin Albrecht <martinralbrecht@googlemail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AX_M4RI_CFLAGS], 
[ AC_PREREQ(2.59)
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PROG_SED])

  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $CRYPTOMINISAT4_M4RI_CFLAGS"
  AC_CACHE_CHECK(for M4RI CFLAGS, ax_cv_m4ri_cflags,
  [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <m4ri/m4ri_config.h>
      ]], 
      [[
  FILE *f;
  f = fopen("conftest_m4ri_cflags", "w"); if (!f) return 1;
  fprintf(f,"%s %s",__M4RI_SIMD_CFLAGS, __M4RI_OPENMP_CFLAGS);
  fclose(f);
  return 0;
   ]])],
    [ax_cv_m4ri_cflags=`cat conftest_m4ri_cflags`; rm -f conftest_m4ri_cflags; CFLAGS="$save_CFLAGS"],
    [ax_cv_m4ri_cflags=""; rm -f conftest_m4ri_cflags; CFLAGS="$save_CFLAGS"],
    [ax_cv_m4ri_cflags=""; CFLAGS="$save_CFLAGS"])])])
])
