/*
  Copyright (c) 2013, Ilan Schnell, Continuum Analytics, Inc.
                2014, Mate Soos
  Python bindings to CryptoMiniSat (http://msoos.org)
  This file is published under an MIT style license (see LICENSE)

  CryptoMiniSat is published under the LGPLv2 license, so the actual compiled
  package is under LGPLv2.
*/

#define PYCRYPTOSAT_URL  "https://pypi.python.org/pypi/pycryptosat"

#include <Python.h>
#include <structmember.h>

#include "assert.h"
#include <cryptominisat3/cryptominisat.h>
using namespace CMSat;

#define IS_INT(x)  (PyInt_Check(x) || PyLong_Check(x))

#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION <= 5
#define PyUnicode_FromString  PyString_FromString
#endif

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SATSolver* cmsat;
} Solver;

static SATSolver* setup_solver(PyObject *args, PyObject *kwds)
{
    static char* kwlist[] = {"verbose", "confl_limit", NULL};

    int verbose = 0;
    long confl_limit = std::numeric_limits<long>::max();
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|il", kwlist, &verbose, &confl_limit)) {
        return NULL;
    }

    SolverConf conf;
    conf.verbosity = verbose;
    conf.maxConfl = confl_limit;

    SATSolver *cmsat = new SATSolver(conf);
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
        PyErr_Format(PyExc_ValueError, "integer %ld is too small or too large", var);
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

static PyObject* add_clause(Solver *self, PyObject *args, PyObject *kwds)
{
    PyObject *clause;
    static char* kwlist[] = {"clause", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &clause)) {
        return NULL;
    }

    PyObject *iterator = PyObject_GetIter(clause);
    if (iterator == NULL) {
        PyErr_SetString(PyExc_TypeError, "interable object expected");
        return 0;
    }

    std::vector<Lit> lits;
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

    self->cmsat->add_clause(lits);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* get_solution(SATSolver *cmsat)
{
    PyObject *list;

    uint32_t max_idx = cmsat->nVars();
    list = PyList_New((Py_ssize_t) max_idx);
    if (list == NULL) {
        PyErr_SetString(PyExc_SystemError, "failed to create list");
        return NULL;
    }
    for (long i = 0; i < max_idx; i++) {
        long v = cmsat->get_model()[i] == l_True ? 1 : -1;

        assert(v == -1 || v == 1);
        if (PyList_SetItem(list, (Py_ssize_t)i, PyInt_FromLong((long) (v * (i+1)))) < 0) {
            PyErr_SetString(PyExc_SystemError, "failed to create list");
            Py_DECREF(list);
            return NULL;
        }
    }
    return list;
}

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

    lbool res;
    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = self->cmsat->solve(&assumption_lits);
    Py_END_ALLOW_THREADS

    if (res == l_True) {
        result = get_solution(self->cmsat);
    } else if (res == l_False) {
        result = PyUnicode_FromString("UNSAT");
    } else if (res == l_Undef) {
        result = PyUnicode_FromString("UNKNOWN");
    }

    return result;
}

/*************************** Method definitions *************************/

static PyMethodDef module_methods[] = {
    //{"solve",     (PyCFunction) full_solve,  METH_VARARGS | METH_KEYWORDS, "my new solver stuff"},
    {NULL,        NULL}  /* sentinel */
};

static PyMethodDef Solver_methods[] = {
    {"solve",     (PyCFunction) solve,       METH_VARARGS | METH_KEYWORDS, "solves the system"},
    {"add_clause",(PyCFunction) add_clause,  METH_VARARGS | METH_KEYWORDS, "adds a cluse to the system"},
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
        //self->cmsat = new SATSolver;
        self->cmsat = NULL;
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
    "Solver objects",           /* tp_doc */
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
}
