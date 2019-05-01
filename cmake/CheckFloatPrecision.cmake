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

 check_c_source_runs("
  #include <stdio.h>
  #include <string.h>
  #include <machine/ieeefp.h>
 
  double div (double a, double b) {
    fp_prec_t fpu_oldprec;
    volatile double result;
 
    fpu_oldprec = fpgetprec();
    fpsetprec(FP_PD);
    result = a / b;
    fpsetprec(fpu_oldprec);
    return result;
  }
 
  int main (int argc, char **argv) {
    double d = div (2877.0, 1000000.0);
    char buf[255];
    sprintf(buf, \"%.30f\", d);
    // see if the result is actually in double precision
    return strncmp(buf, \"0.00287699\", 10) == 0 ? 0 : 1;
  }
 " HAVE_FPSETPREC)

 check_c_source_runs("
  #include <stdio.h>
  #include <string.h>
  #include <float.h>
 
  double div (double a, double b) {
    unsigned int fpu_oldcw;
    volatile double result;
 
    fpu_oldcw = _controlfp(0, 0);
    _controlfp(_PC_53, _MCW_PC);
    result = a / b;
    _controlfp(fpu_oldcw, _MCW_PC);
    return result;
  }
 
  int main (int argc, char **argv) {
    double d = div (2877.0, 1000000.0);
    char buf[255];
    sprintf(buf, \"%.30f\", d);
    // see if the result is actually in double precision
    return strncmp(buf, \"0.00287699\", 10) == 0 ? 0 : 1;
  }
 " HAVE__CONTROLFP)

 check_c_source_runs("
  #include <stdio.h>
  #include <string.h>
  #include <float.h>
 
  double div (double a, double b) {
    unsigned int fpu_oldcw, fpu_cw;
    volatile double result;
 
    _controlfp_s(&fpu_cw, 0, 0);
    fpu_oldcw = fpu_cw;
    _controlfp_s(&fpu_cw, _PC_53, _MCW_PC);
    result = a / b;
    _controlfp_s(&fpu_cw, fpu_oldcw, _MCW_PC);
    return result;
  }
 
  int main (int argc, char **argv) {
    double d = div (2877.0, 1000000.0);
    char buf[255];
    sprintf(buf, \"%.30f\", d);
    // see if the result is actually in double precision
    return strncmp(buf, \"0.00287699\", 10) == 0 ? 0 : 1;
  }
 " HAVE__CONTROLFP_S)

 check_c_source_runs("
  #include <stdio.h>
  #include <string.h>
  
  double div (double a, double b) {
    unsigned int oldcw, cw;
    volatile double result;
    
    __asm__ __volatile__ (\"fnstcw %0\" : \"=m\" (*&oldcw));
    cw = (oldcw & ~0x0 & ~0x300) | 0x200;
    __asm__ __volatile__ (\"fldcw %0\" : : \"m\" (*&cw));
    
    result = a / b;
    
    __asm__ __volatile__ (\"fldcw %0\" : : \"m\" (*&oldcw));
    
    return result;
  }
 
  int main (int argc, char **argv) {
    double d = div (2877.0, 1000000.0);
    char buf[255];
    sprintf(buf, \"%.30f\", d);
    // see if the result is actually in double precision
    return strncmp(buf, \"0.00287699\", 10) == 0 ? 0 : 1;
  }
 " HAVE_FPU_INLINE_ASM_X86)
endmacro(check_float_precision)
