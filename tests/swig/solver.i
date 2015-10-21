%module(naturalvar=1) solver
%include "typemaps.i"

%{
#   define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS
%}

// Convert from Python --> C
%typemap(in) uint32_t, int, long {
    $1 = PyInt_AsLong($input);
}

%typemap(in) bool {
    if ($input == Py_True)
        $1 = true;
    else
        $1 = false;
}

%typemap(in) (uint32_t a, bool b) {
    $1 = PyInt_AsLong(PyList_GetItem($input,0));
    if (PyList_GetItem($input,1) == True)
        $2 = true;
    else
        $2 = false;
}

// Convert from C --> Python
%typemap(out) uint32_t {
    $result = PyInt_FromLong($1);
}

%ignore Lit();

%{
#include "solver.h"
#include "cnf.h"
#include "solvertypes.h"
using namespace CMSat;
%}

%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"



namespace std {
    %template(IntVector) vector<int>;
    %template(DoubleVector) vector<double>;
    %template(StringVector) vector<string>;
    %template(ConstCharVector) vector<const char*>;
    %template(LitVector) vector<Lit>;
}


%include "solver.h"
%include "cryptominisat4/solvertypesmini.h"
