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

#include "assert.h"
#include <cryptominisat3/cryptominisat.h>
using namespace CMSat;

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#endif

#ifdef IS_PY3K
#define PyInt_FromLong  PyLong_FromLong
#define IS_INT(x)  (PyLong_Check(x))
#else
#define IS_INT(x)  (PyInt_Check(x) || PyLong_Check(x))
#endif

#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION <= 5
#define PyUnicode_FromString  PyString_FromString
#endif

PyDoc_STRVAR(solve_doc,
"solve(clauses [, kwargs]) -> list\n\
\n\
Solve the SAT problem for the clauses, and return a solution as a\n\
list of integers, or one of the strings \"UNSAT\", \"UNKNOWN\".\n\
Please see " PYCRYPTOSAT_URL " for more details.");

static int add_clauses(SATSolver *cmsat, PyObject *clauses);

static SATSolver* setup_cryptominisat(PyObject *args, PyObject *kwds)
{
    static char* kwlist[] = {"clauses", "verbose", "confl_limit", NULL};

    PyObject *clauses;
    int verbose = 0;
    long confl_limit = std::numeric_limits<long>::max();
    if (!PyArg_ParseTupleAndKeywords(
        args, kwds, "O|iK:(iter)solve", kwlist,
        &clauses, &verbose, &confl_limit))
    {
        return NULL;
    }

    SolverConf conf;
    conf.verbosity = verbose;
    conf.maxConfl = confl_limit;

    SATSolver *cmsat = new SATSolver(conf);

    if (add_clauses(cmsat, clauses) < 0) {
        delete cmsat;
        return NULL;
    }

    return cmsat;
}

static int add_clause(SATSolver *cmsat, PyObject *clause)
{
    PyObject *iterator = PyObject_GetIter(clause);
    if (iterator == NULL)
        return -1;

    std::vector<Lit> lits;
    PyObject *lit;
    while ((lit = PyIter_Next(iterator)) != NULL) {
        if (!IS_INT(lit))  {
            Py_DECREF(lit);
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_TypeError, "integer expected");
            return -1;
        }
        long v = PyLong_AsLong(lit);
        Py_DECREF(lit);
        if (v == 0) {
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_ValueError, "non-zero integer expected");
            return -1;
        }
        if (v > std::numeric_limits<int>::max()/2
            || v < std::numeric_limits<int>::min()/2
        ) {
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_ValueError, "Integer is too small or too large");
            return -1;
        }

        bool sign = false;
        if (v < 0) {
            v *= -1;
            sign = true;
        }
        v--;

        if (v >= cmsat->nVars()) {
            for(unsigned long i = cmsat->nVars(); i <= v ; i++) {
                cmsat->new_var();
            }
        }

        lits.push_back(Lit(v, sign));
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred())
        return -1;

    cmsat->add_clause(lits);
    return 0;
}

static int add_clauses(SATSolver *cmsat, PyObject *clauses)
{
    PyObject *iterator;       /* clauses can be any iterable */
    PyObject *item;           /* each clause is an iterable of intergers */

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

static PyObject* solve(PyObject *self, PyObject *args, PyObject *kwds)
{
    SATSolver *cmsat;
    PyObject *result = NULL;    /* return value */

    cmsat = setup_cryptominisat(args, kwds);
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

/*********************** Solution Iterator *********************/

typedef struct {
    PyObject_HEAD
    SATSolver *cmsat;
    signed char *mem; //temp storage
} soliterobject;

PyDoc_STRVAR(itersolve_doc, "\
itersolve(clauses [, kwargs]) -> interator\n\n\
Solve the SAT problem for the clauses, and return an iterator over\n\
the solutions (which are lists of integers).\n\
Please see www.msoos.org for more details.");

static int block_solution(SATSolver *cmsat)
{
    std::vector<Lit> clause;

    const uint32_t vars = cmsat->nVars();
    for (uint32_t i = 0; i < vars; i++) {
        lbool val = cmsat->get_model()[i];
        if (val == l_Undef) {
            continue;
        }

        Lit lit(i, val == l_True);
        clause.push_back(lit);
    }

    cmsat->add_clause(clause);
    return 0;
}

static PyObject* soliter_next(soliterobject *it)
{
    PyObject *result = NULL;
    lbool res;

    //assert(SolIter_Check(it));

    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = it->cmsat->solve();
    Py_END_ALLOW_THREADS

    if (res == l_True) {
        result = get_solution(it->cmsat);
        if (result == NULL) {
            return NULL;
        }

        /* add inverse solution to the clauses, for next interation */
        if (block_solution(it->cmsat) < 0)
            return NULL;
    }
    return result;
}

static void soliter_dealloc(soliterobject *it)
{
    PyObject_GC_UnTrack(it);
    if (it->mem)
        PyMem_Free(it->mem);
    delete it->cmsat;
    PyObject_GC_Del(it);
}

static int soliter_traverse(soliterobject *it, visitproc visit, void *arg)
{
    return 0;
}

static PyTypeObject SolIter_Type = {
#ifdef IS_PY3K
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,                                        /* ob_size */
#endif
    "soliterator",                            /* tp_name */
    sizeof(soliterobject),                    /* tp_basicsize */
    0,                                        /* tp_itemsize */
    /* methods */
    (destructor) soliter_dealloc,             /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    PyObject_GenericGetAttr,                  /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                        /* tp_doc */
    (traverseproc) soliter_traverse,          /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    PyObject_SelfIter,                        /* tp_iter */
    (iternextfunc) soliter_next,              /* tp_iternext */
    0,                                        /* tp_methods */
};

//#define SolIter_Check(op)  PyObject_TypeCheck(op, &SolIter_Type)

static PyObject* itersolve(PyObject *self, PyObject *args, PyObject *kwds)
{
    soliterobject *it;          /* iterator to be returned */

    it = PyObject_GC_New(soliterobject, &SolIter_Type);
    if (it == NULL)
        return NULL;

    it->cmsat = setup_cryptominisat(args, kwds);
    if (it->cmsat == NULL)
        return NULL;

    it->mem = NULL;
    PyObject_GC_Track(it);
    return (PyObject *) it;
}

/*************************** Method definitions *************************/

/* declaration of methods supported by this module */
static PyMethodDef module_functions[] = {
    {"solve",     (PyCFunction) solve,     METH_VARARGS | METH_KEYWORDS, solve_doc},
    {"itersolve", (PyCFunction) itersolve, METH_VARARGS | METH_KEYWORDS, itersolve_doc},
    {NULL,        NULL}  /* sentinel */
};

PyDoc_STRVAR(module_doc, "\
pycryptosat: bindings to CryptoMiniSat\n\
============================\n\n\
There are two functions in this module, solve and itersolve.\n\
Please see " PYCRYPTOSAT_URL " for more details.");

/* initialization routine for the shared libary */
#ifdef IS_PY3K
static PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "pycryptosat", module_doc, -1, module_functions,
};
PyMODINIT_FUNC PyInit_pycryptosat(void)
#else
PyMODINIT_FUNC initpycryptosat(void)
#endif
{
    PyObject *m;

#ifdef IS_PY3K
    if (PyType_Ready(&SolIter_Type) < 0)
        return NULL;
    m = PyModule_Create(&moduledef);
    if (m == NULL)
        return NULL;
#else
    if (PyType_Ready(&SolIter_Type) < 0)
        return;
    m = Py_InitModule3("pycryptosat", module_functions, module_doc);
    if (m == NULL)
        return;
#endif

    PyModule_AddObject(m, "__version__", PyUnicode_FromString(SATSolver::get_version()));

#ifdef IS_PY3K
    return m;
#endif
}
