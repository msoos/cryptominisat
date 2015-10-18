%module solver

%{
#include "solver.h"
#include "cnf.h"
#include "solvertypes.h"
using namespace CMSat;
%}

%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"
%include "solver.h"
