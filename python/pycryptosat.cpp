/*************
Python bindings to CryptoMiniSat (http://msoos.org)

Copyright (c) 2013, Ilan Schnell, Continuum Analytics, Inc.
            2014, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**********************************/

#define PYCRYPTOSAT_URL  "https://pypi.python.org/pypi/pycryptosat"

#include <Python.h>
#include <structmember.h>
#include <limits>
#include <algorithm>

#include "assert.h"
#include <cryptominisat5/cryptominisat.h>
using namespace CMSat;

#define IS_INT(x)  (PyInt_Check(x) || PyLong_Check(x))

#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION <= 5
#define PyUnicode_FromString  PyString_FromString
#endif

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wwrite-strings"

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SATSolver* cmsat;
} Solver;

static PyObject *outofconflerr = NULL;

static SATSolver* setup_solver(PyObject *args, PyObject *kwds)
{
    static char* kwlist[] = {"verbose", "confl_limit", "threads", NULL};

    int verbose = 0;
    int num_threads = 1;
    long confl_limit = std::numeric_limits<long>::max();
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ili", kwlist, &verbose, &confl_limit, &num_threads)) {
        return NULL;
    }
    if (verbose < 0) {
        PyErr_SetString(PyExc_ValueError, "verbosity must be at least 0");
        return NULL;
    }
    if (confl_limit < 0) {
        PyErr_SetString(PyExc_ValueError, "conflict limit must be at least 0");
        return NULL;
    }
    if (num_threads <= 0) {
        PyErr_SetString(PyExc_ValueError, "number of threads must be at least 1");
        return NULL;
    }

    SATSolver *cmsat = new SATSolver;
    cmsat->set_max_confl(confl_limit);
    cmsat->set_verbosity(verbose);
    cmsat->set_num_threads(num_threads);

    return cmsat;
}

static int convert_lit_to_sign_and_var(PyObject* lit, long& var, bool& sign)
{
    if (!IS_INT(lit))  {
        PyErr_SetString(PyExc_TypeError, "integer expected");
        return 0;
    }

    long val = PyLong_AsLong(lit);
    if (val == 0) {
        PyErr_SetString(PyExc_ValueError, "non-zero integer expected");
        return 0;
    }
    if (val > std::numeric_limits<int>::max()/2
        || val < std::numeric_limits<int>::min()/2
    ) {
        PyErr_Format(PyExc_ValueError, "integer %ld is too small or too large", val);
        return 0;
    }

    sign = false;
    if (val < 0) {
        val *= -1;
        sign = true;
    }
    val--;
    var = val;

    return 1;
}

static int parse_clause(
    Solver *self
    , PyObject *clause
    , std::vector<Lit>& lits
) {
    PyObject *iterator = PyObject_GetIter(clause);
    if (iterator == NULL) {
        PyErr_SetString(PyExc_TypeError, "interable object expected");
        return 0;
    }

    PyObject *lit;
    while ((lit = PyIter_Next(iterator)) != NULL) {
        long var;
        bool sign;
        int ret = convert_lit_to_sign_and_var(lit, var, sign);
        Py_DECREF(lit);
        if (!ret) {
            Py_DECREF(iterator);
            return 0;
        }

        if (var >= self->cmsat->nVars()) {
            for(long i = (long)self->cmsat->nVars(); i <= var ; i++) {
                self->cmsat->new_var();
            }
        }

        lits.push_back(Lit(var, sign));
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred()) {
        return 0;
    }

    return 1;
}

static int parse_xor_clause(
    Solver *self
    , PyObject *clause
    , std::vector<uint32_t>& vars
) {
    PyObject *iterator = PyObject_GetIter(clause);
    if (iterator == NULL) {
        PyErr_SetString(PyExc_TypeError, "interable object expected");
        return 0;
    }

    PyObject *lit;
    while ((lit = PyIter_Next(iterator)) != NULL) {
        long var;
        bool sign;
        int ret = convert_lit_to_sign_and_var(lit, var, sign);
        Py_DECREF(lit);
        if (!ret) {
            Py_DECREF(iterator);
            return 0;
        }
        if (sign) {
            PyErr_SetString(PyExc_ValueError, "XOR clause must contiain only positive variables (not inverted literals)");
            Py_DECREF(iterator);
            return 0;
        }

        if (var >= self->cmsat->nVars()) {
            for(long i = (long)self->cmsat->nVars(); i <= var ; i++) {
                self->cmsat->new_var();
            }
        }

        vars.push_back(var);
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred()) {
        return 0;
    }

    return 1;
}

static PyObject* add_clause(Solver *self, PyObject *args, PyObject *kwds)
{
    static char* kwlist[] = {"clause", NULL};
    PyObject *clause;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &clause)) {
        return NULL;
    }

    std::vector<Lit> lits;
    if (!parse_clause(self, clause, lits)) {
        return 0;
    }
    self->cmsat->add_clause(lits);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* add_xor_clause(Solver *self, PyObject *args, PyObject *kwds)
{
    static char* kwlist[] = {"xor_clause", "rhs", NULL};
    PyObject *rhs;
    PyObject *clause;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &clause, &rhs)) {
        return NULL;
    }
    if (!PyBool_Check(rhs)) {
        PyErr_SetString(PyExc_TypeError, "rhs must be boolean");
        return NULL;
    }
    bool real_rhs = PyObject_IsTrue(rhs);

    std::vector<uint32_t> vars;
    if (!parse_xor_clause(self, clause, vars)) {
        return 0;
    }

    self->cmsat->add_xor_clause(vars, real_rhs);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* get_solution(SATSolver *cmsat)
{
    PyObject *tuple;

    unsigned max_idx = cmsat->nVars();
    tuple = PyTuple_New((Py_ssize_t) max_idx+1);
    if (tuple == NULL) {
        PyErr_SetString(PyExc_SystemError, "failed to create a tuple");
        return NULL;
    }

    Py_INCREF(Py_None);
    if (PyTuple_SetItem(tuple, (Py_ssize_t)0, Py_None) < 0) {
        PyErr_SetString(PyExc_SystemError, "failed to add 1st element to tuple");
        Py_DECREF(tuple);
        return NULL;
    }

    for (unsigned i = 0; i < max_idx; i++) {
        lbool v = cmsat->get_model()[i];
        PyObject *py_value = NULL;
        if (v == l_True) {
            Py_INCREF(Py_True);
            py_value = Py_True;
        } else if (v == l_False) {
            Py_INCREF(Py_False);
            py_value = Py_False;
        } else if (v == l_Undef) {
            Py_INCREF(Py_None);
            py_value = Py_None;
        }

        if (PyTuple_SetItem(tuple, (Py_ssize_t)i+1, py_value) < 0) {
            PyErr_SetString(PyExc_SystemError, "failed to add to tuple");
            Py_DECREF(tuple);
            return NULL;
        }
    }
    return tuple;
}

static PyObject* get_raw_solution(SATSolver *cmsat) {

    // Create tuple with the size of number of variables in model
    PyObject *tuple;

    unsigned max_idx = cmsat->nVars();
    tuple = PyTuple_New((Py_ssize_t) max_idx);
    if (tuple == NULL) {
        PyErr_SetString(PyExc_SystemError, "failed to create a tuple");
        return NULL;
    }

    // Add each variable in model to the tuple
    int sign;
    for (long var = 0; var != (long)max_idx; var++) {

        if (cmsat->get_model()[var] != l_Undef) {

            PyObject *py_value = NULL;
            sign = (cmsat->get_model()[var] == l_True) ? 1 : -1;
            py_value = PyInt_FromLong((var + 1) * sign);
            // Py_INCREF(py_value) ?????????????????????

            if (PyTuple_SetItem(tuple, (Py_ssize_t)var, py_value) < 0) {
                PyErr_SetString(PyExc_SystemError, "failed to add to tuple");
                Py_DECREF(tuple);
                return NULL;
            }
        }
    }
    return tuple;
}

static PyObject* nb_vars(Solver *self)
{
    return PyInt_FromLong(self->cmsat->nVars());
}

/*
static PyObject* nb_clauses(Solver *self)
{
    // Private attribute => need to make a public method
    return PyInt_FromLong(self->cmsat->data->solvers.size());
}*/

static int parse_assumption_lits(PyObject* assumptions, SATSolver* cmsat, std::vector<Lit>& assumption_lits)
{
    PyObject *iterator = PyObject_GetIter(assumptions);
    if (iterator == NULL) {
        PyErr_SetString(PyExc_TypeError, "interable object expected");
        return 0;
    }

    PyObject *lit;
    while ((lit = PyIter_Next(iterator)) != NULL) {
        long var;
        bool sign;
        int ret = convert_lit_to_sign_and_var(lit, var, sign);
        Py_DECREF(lit);
        if (!ret) {
            Py_DECREF(iterator);
            return 0;
        }

        if (var >= cmsat->nVars()) {
            Py_DECREF(iterator);
            PyErr_Format(PyExc_ValueError, "Variable %ld not used in clauses", var+1);
            return 0;
        }

        assumption_lits.push_back(Lit(var, sign));
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred()) {
        return 0;
    }

    return 1;
}

static PyObject* solve(Solver *self, PyObject *args, PyObject *kwds)
{
    PyObject* assumptions = NULL;
    static char* kwlist[] = {"assumptions", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &assumptions)) {
        return NULL;
    }

    std::vector<Lit> assumption_lits;
    if (assumptions) {
        if (!parse_assumption_lits(assumptions, self->cmsat, assumption_lits)) {
            return 0;
        }
    }

    PyObject *result = NULL;

    result = PyTuple_New((Py_ssize_t) 2);
    if (result == NULL) {
        PyErr_SetString(PyExc_SystemError, "failed to create a tuple");
        return NULL;
    }

    lbool res;
    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = self->cmsat->solve(&assumption_lits);
    Py_END_ALLOW_THREADS

    if (res == l_True) {
        PyObject* solution = get_solution(self->cmsat);
        if (!solution) {
            Py_DECREF(result);
            return NULL;
        }
        Py_INCREF(Py_True);
        PyTuple_SetItem(result, 0, Py_True);
        PyTuple_SetItem(result, 1, solution);
    } else if (res == l_False) {
        Py_INCREF(Py_False);
        PyTuple_SetItem(result, 0, Py_False);
        Py_INCREF(Py_None);
        PyTuple_SetItem(result, 1, Py_None);
    } else if (res == l_Undef) {
        Py_DECREF(result);
        return PyErr_SetFromErrno(outofconflerr);
    }

    return result;
}

static PyObject* is_satisfiable(Solver *self)
{
    lbool ret = l_True;

    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    ret = self->cmsat->solve();
    Py_END_ALLOW_THREADS

    if (ret == l_True) {
        Py_INCREF(Py_True);
        return Py_True;
    } else if (ret == l_False) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    // l_Undef
    return PyErr_SetFromErrno(outofconflerr);
}

static PyObject* msolve_selected(Solver *self, PyObject *args, PyObject *kwds)
{
    int max_nr_of_solutions;
    int raw_solutions_activated = true;
    PyObject *var_selected;

    static char* kwlist[] = {"max_nr_of_solutions", "var_selected", "raw", NULL};
    // Use 'p' wildcard on version 3.3+ of Python
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO|ii", kwlist,
                                     &max_nr_of_solutions,
                                     &var_selected,
                                     &raw_solutions_activated)) {
        return NULL;
    }

    // sort using a custom function object
//      struct {
//          bool operator()(Lit a, Lit b)
//          {
//              return a < b;
//          }
//      } customLess;

    std::vector<Lit> var_lits;
    if (!parse_clause(self, var_selected, var_lits)) {
        return 0;
    }

    // Debug
    std::cout << "Nb max solutions: " << max_nr_of_solutions << std::endl;
    std::cout << "Nb literals: " << var_lits.size() << std::endl;
    std::cout << "Raw sols ?: " << raw_solutions_activated << std::endl;
//     for (unsigned long i = 0; i < var_lits.size(); i++) {
//         std::cout << "real value: " << var_lits[i]
//                   << "; x: " << var_lits[i].toInt()
//                   << "; sign: " << var_lits[i].sign()
//                   << "; var: " << var_lits[i].var()
//                   //<< "; toInt as long " << PyLong_AsLong(var_lits[i])
//                   << '\n';
//     }

//      std::sort(var_lits.begin(), var_lits.end(), customLess);
//
//     for (unsigned long i = 0; i < var_lits.size(); i++) {
//         std::cout << var_lits[i] << ';';
//     }
//     std::cout << std::endl;

    int current_nr_of_solutions = 0;
    lbool ret = l_True;
    std::vector<Lit>::iterator it;

////////////////////////
// Search in vector
//     it = std::find(var_lits.begin(), var_lits.end(), Lit(21, false));
//     std::cout <<  Lit(21, false).toInt() << std::endl;
//
//     if (it != var_lits.end())
//         std::cout << "Element found in myvector: " << *it << '\n';
//     else
//         std::cout << "Element not found in myvector\n";
//
//    return NULL;
////////////////////////

    PyObject *solutions = NULL;
    solutions = PyList_New(0);
    if (solutions == NULL) {
        PyErr_SetString(PyExc_SystemError, "failed to create a list");
        return NULL;
    }

    while((current_nr_of_solutions < max_nr_of_solutions) && (ret == l_True)) {

        Py_BEGIN_ALLOW_THREADS      /* release GIL */
        ret = self->cmsat->solve();
        Py_END_ALLOW_THREADS

        current_nr_of_solutions++;

        std::cout << "state : " << ret << std::endl;
        std::cout << "current sol: " << current_nr_of_solutions << std::endl;

        if(ret == l_True) {

            // Memorize the solution
            PyObject* solution = NULL;
            if (!raw_solutions_activated) {
                // Solution in v5 format
                solution = get_solution(self->cmsat);
            } else {
                // Solution in v2.9 format
                solution = get_raw_solution(self->cmsat);
            }

            if (!solution) { // CF dans SOLVE comment c'est géré !! => ajout de None si aps de sol !
                PyErr_SetString(PyExc_SystemError, "no solution");
                Py_DECREF(solutions);
                return NULL;
            }
            // Add solution
            PyList_Append(solutions, solution);

            // Prepare next statement
            if (current_nr_of_solutions < max_nr_of_solutions) {

                std::vector<Lit> ban_solution;
                for (long var = 0; var != (long)self->cmsat->nVars(); var++) {
                    // Search var+1 in [var_lits.begin(), var_lits.end()[
                    // false : > 0; true : < 0 sign !!
                    it = std::find(var_lits.begin(), var_lits.end(), Lit(var, false));
                    //std::cout << var << " search " << Lit(var, false) << "; found ?" << (it != var_lits.end()) << std::endl;

                    if ((self->cmsat->get_model()[var] != l_Undef) && (it != var_lits.end())) {
                        //std::cout << "ici: " << var << std::endl;
                        ban_solution.push_back(
                            Lit(var,
                                (self->cmsat->get_model()[var] == l_True) ? true : false)
                        );
                    }
                }

                // Ban current solution for the next run
                self->cmsat->add_clause(ban_solution);

    //               for (unsigned long i = 0; i < ban_solution.size(); i++) {
    //                   std::cout << ban_solution[i] << ';';
    //               }
    //               std::cout << std::endl;
            }
        } else if (ret == l_False) {
            std::cout << "No more solution or limitation reached" << std::endl;
            //Py_INCREF(Py_None);
            //PyList_Append(solutions, Py_None);
        } else if (ret == l_Undef) {
            std::cout << "Nothing to do => sol undef" << std::endl;
            Py_DECREF(solutions);
            return PyErr_SetFromErrno(outofconflerr);
        }
    }
    // Return list of all solutions
    return solutions;
}

/*************************** Method definitions *************************/

static PyMethodDef module_methods[] = {
    //{"solve",     (PyCFunction) full_solve,  METH_VARARGS | METH_KEYWORDS, "my new solver stuff"},
    {NULL,        NULL}  /* sentinel */
};

static PyMethodDef Solver_methods[] = {
    {"solve",     (PyCFunction) solve,       METH_VARARGS | METH_KEYWORDS, "solves the system"},
    {"add_clause",(PyCFunction) add_clause,  METH_VARARGS | METH_KEYWORDS, "adds a clause to the system"},
    {"add_xor_clause",(PyCFunction) add_xor_clause,  METH_VARARGS | METH_KEYWORDS, "adds an XOR clause to the system"},
    {"nb_vars", (PyCFunction) nb_vars, METH_VARARGS | METH_KEYWORDS, "returns number of variables"},
    //{"nb_clauses", (PyCFunction) nb_clauses, METH_VARARGS | METH_KEYWORDS, "returns number of clauses"},
    {"msolve_selected", (PyCFunction) msolve_selected, METH_VARARGS | METH_KEYWORDS, "solve selected variables"},
    {"is_satisfiable", (PyCFunction) is_satisfiable, METH_VARARGS | METH_KEYWORDS, "return satisfiability of the system"},
    {NULL,        NULL}  /* sentinel */
};

static void
Solver_dealloc(Solver* self)
{
    delete self->cmsat;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Solver_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Solver *self;

    self = (Solver *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->cmsat = setup_solver(args, kwds);
        if (self->cmsat == NULL) {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *)self;
}

static int
Solver_init(Solver *self, PyObject *args, PyObject *kwds)
{
    self->cmsat = setup_solver(args, kwds);
    if (!self->cmsat) {
        return -1;
    }
    return 0;
}

static PyMemberDef Solver_members[] = {
    /*{"first", T_OBJECT_EX, offsetof(Noddy, first), 0,
     "first name"},
    {"last", T_OBJECT_EX, offsetof(Noddy, last), 0,
     "last name"},
    {"number", T_INT, offsetof(Noddy, number), 0,
     "noddy number"},*/
    {NULL}  /* Sentinel */
};

static const char solver_create_docstring[] = "Create Solver object.\n"
"Supported arguments: verbose, clause_limit, threads.\n"
"   'verbose' -- integer. 0: nothing printed. 15: very verbose. Default: 0\n"
"   'confl_limit' -- integer. Abort after this many conflicts. Default: never abort.\n"
"   'threads' -- integer. Number of threads to use. Default: 1"
;

static PyTypeObject pycryptosat_SolverType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pycryptosat.Solver",             /*tp_name*/
    sizeof(Solver),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Solver_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    solver_create_docstring,           /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    Solver_methods,             /* tp_methods */
    Solver_members,            /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Solver_init,      /* tp_init */
    0,                         /* tp_alloc */
    Solver_new,                 /* tp_new */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initpycryptosat(void)
{
    PyObject* m;

    pycryptosat_SolverType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pycryptosat_SolverType) < 0)
        return;

    m = Py_InitModule3("pycryptosat", module_methods,
                       "Example module that creates an extension type.");

    Py_INCREF(&pycryptosat_SolverType);
    PyModule_AddObject(m, "Solver", (PyObject *)&pycryptosat_SolverType);
    PyModule_AddObject(m, "__version__", PyUnicode_FromString(SATSolver::get_version()));

    outofconflerr = PyErr_NewExceptionWithDoc("Solver.OutOfConflicts", "Ran out of the number of conflicts", NULL, NULL);
    Py_INCREF(outofconflerr);
    PyModule_AddObject(m, "OutOfConflicts",  outofconflerr);
}
