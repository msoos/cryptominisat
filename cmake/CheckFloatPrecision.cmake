# Floating point precision checks
#
# This file contains floating point precision checks
#
# This file is released under public domain - or - in countries where this is
# not possible under the following license:
#
#    Permission is hereby granted, free of charge, to any person obtaining a
#    copy of this software, to deal in the software without restriction,
#    including without limitation the rights to use, copy, modify, merge,
#    publish, distribute, sublicense, and/or sell copies of the software,
#    and to permit persons to whom the software is furnished to do so, subject
#    to no condition whatsoever.
#
#    This software is provided AS IS, without warranty of any kind, express or
#    implied.

# FIXME:
#  This file was only built for x86 and x86_64 platforms but it does not check
#  for the platform since CMake does not provide a viable variable.

include(CheckCSourceRuns)

macro(check_float_precision)
 check_c_source_runs("
  #include <stdio.h>
  #include <string.h>
  #include <fpu_control.h>
 
  double div (double a, double b) {
    fpu_control_t fpu_oldcw, fpu_cw;
    volatile double result;
 
    _FPU_GETCW(fpu_oldcw);
    fpu_cw = (fpu_oldcw & ~_FPU_EXTENDED & ~_FPU_SINGLE) | _FPU_DOUBLE;
    _FPU_SETCW(fpu_cw);
    result = a / b;
    _FPU_SETCW(fpu_oldcw);
    return result;
  }
 
  int main (int argc, char **argv) {
    double d = div (2877.0, 1000000.0);
    char buf[255];
    sprintf(buf, \"%.30f\", d);
    // see if the result is actually in double precision
    return strncmp(buf, \"0.00287699\", 10) == 0 ? 0 : 1;
  }
 " HAVE__FPU_SETCW)

endmacro(check_float_precision)
