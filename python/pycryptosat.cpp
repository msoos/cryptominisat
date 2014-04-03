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

static int add_clauses(SATSolver *cmsat, PyObject *clauses);

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
        if (!IS_INT(lit))  {
            Py_DECREF(lit);
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_TypeError, "integer expected");
            return 0;
        }
        long v = PyLong_AsLong(lit);
        Py_DECREF(lit);
        if (v == 0) {
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_ValueError, "non-zero integer expected");
            return 0;
        }
        if (v > std::numeric_limits<int>::max()/2
            || v < std::numeric_limits<int>::min()/2
        ) {
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_ValueError, "Integer is too small or too large");
            return 0;
        }

        bool sign = false;
        if (v < 0) {
            v *= -1;
            sign = true;
        }
        v--;

        if (v >= self->cmsat->nVars()) {
            for(unsigned long i = self->cmsat->nVars(); i <= v ; i++) {
                self->cmsat->new_var();
            }
        }

        lits.push_back(Lit(v, sign));
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred()) {
        return 0;
    }

    self->cmsat->add_clause(lits);

    Py_INCREF(Py_None);
    return Py_None;
}

/*static int add_clauses(SATSolver *cmsat, PyObject *clauses)
{
    PyObject *iterator;       //clauses can be any iterable
    PyObject *item;           // each clause is an iterable of intergers

    iterator = PyObject_GetIter(clauses);
    if (iterator == NULL)
        return -1;

    while ((item = PyIter_Next(iterator)) != NULL) {
        if (add_clause(cmsat, item) < 0) {
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred())
        return -1;
    return 0;
}*/

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

static PyObject* solve(Solver *self)
{
    PyObject *result = NULL;

    lbool res;
    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = self->cmsat->solve();
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

static PyObject* full_solve(PyObject *self, PyObject *args, PyObject *kwds)
{
    SATSolver *cmsat;
    PyObject *result = NULL;

    cmsat = setup_solver(args, kwds);
    if (cmsat == NULL)
        return NULL;

    lbool res;
    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = cmsat->solve();
    Py_END_ALLOW_THREADS

    if (res == l_True) {
        result = get_solution(cmsat);
    } else if (res == l_False) {
        result = PyUnicode_FromString("UNSAT");
    } else if (res == l_Undef) {
        result = PyUnicode_FromString("UNKNOWN");
    }
    delete cmsat;

    return result;
}

/*************************** Method definitions *************************/

static PyMethodDef module_methods[] = {
    //{"solve",     (PyCFunction) full_solve,  METH_VARARGS | METH_KEYWORDS, "my new solver stuff"},
    {NULL,        NULL}  /* sentinel */
};

static PyMethodDef Solver_methods[] = {
    {"solve",     (PyCFunction) solve,       METH_NOARGS, "solves the system"},
    {"add_clause",(PyCFunction) add_clause,  METH_VARARGS | METH_KEYWORDS, "adds a cluse to the system"},
    {NULL,        NULL}  /* sentinel */
};

PyDoc_STRVAR(module_doc, "\
pycryptosat: bindings to CryptoMiniSat\n\
============================\n\n\
There are two functions in this module, solve and itersolve.\n\
Please see " PYCRYPTOSAT_URL " for more details.");

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
