/*
  Copyright (c) 2013, Ilan Schnell, Continuum Analytics, Inc., Mate Soos
  Python bindings to CryptoMiniSat (http://msoos.org)
  This file is published an MIT license.
*/

#define PYCRYPTOSAT_URL  "https://pypi.python.org/pypi/pycosat"

#include <python2.7/Python.h>

#include "assert.h"
#include "../cryptominisat3/cryptominisat.h"
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

// Add the inverse of the solution to the clauses.
static int blocksol(MainSolver *cmsat)
{
    std::vector<Lit> clause;

    const uint32_t max_vars = cmsat->nVars();
    for (uint32_t i = 0; i < max_vars; i++) {
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

static int add_clause(MainSolver *cmsat, PyObject *clause)
{
    PyObject *iterator;         /* each clause is an iterable of literals */
    PyObject *lit;              /* the literals are integers */

    iterator = PyObject_GetIter(clause);
    if (iterator == NULL)
        return -1;

    std::vector<Lit> clause;
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
        if (v > std::numeric_limits<int>::max()
            || v < std::numeric_limits<int>::min()
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
        clause.push_back(Lit(v-1, sign));
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred())
        return -1;

    cmsat->add_clause(clause);
    return 0;
}

static int add_clauses(MainSolver *cmsat, PyObject *clauses)
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

static MainSolver* setup_cryptominisat(PyObject *args, PyObject *kwds)
{
    MainSolver *cmsat;
    PyObject *clauses;          /* iterable of clauses */
    int verbose = 0;
    unsigned long long prop_limit = 0;
    static char* kwlist[] = {"clauses", "verbose", "prop_limit", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|iK:(iter)solve", kwlist,
                                     &clauses, &verbose, &prop_limit))
    {
        return NULL;
    }

    cmsat = new MainSolver;
    //picosat_set_verbosity(cmsat, verbose);

    //if (prop_limit)
    //    picosat_set_propagation_limit(cmsat, prop_limit);

    if (add_clauses(cmsat, clauses) < 0) {
        delete cmsat;
        return NULL;
    }

    //if (verbose >= 2)
    //    picosat_print(cmsat, stdout);

    return cmsat;
}

/* read the solution from the picosat object and return a Python list */
static PyObject* get_solution(MainSolver *cmsat)
{
    PyObject *list;

    uint32_t max_idx = cmsat->nVars();
    list = PyList_New((Py_ssize_t) max_idx);
    if (list == NULL) {
        delete cmsat;
        return NULL;
    }
    for (uint32_t i = 0; i < max_idx; i++) {
        int v = cmsat->get_model()[i] == l_True ? 1 : -1;

        assert(v == -1 || v == 1);
        if (PyList_SetItem(list, (Py_ssize_t)i, PyInt_FromLong((long) (v * (i+1)))) < 0) {
            Py_DECREF(list);
            delete cmsat;
            return NULL;
        }
    }
    return list;
}

static PyObject* solve(PyObject *self, PyObject *args, PyObject *kwds)
{
    MainSolver *cmsat;
    PyObject *result = NULL;    /* return value */

    cmsat = setup_cryptominisat(args, kwds);
    if (cmsat == NULL)
        return NULL;

    lbool res;
    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = cmsat->solve();
    Py_END_ALLOW_THREADS

    switch (res) {
        case l_True:
            result = get_solution(cmsat);
            break;

        case l_False:
            result = PyUnicode_FromString("UNSAT");
            break;

        case l_Undef:
            result = PyUnicode_FromString("UNKNOWN");
            break;

        default:
            PyErr_Format(PyExc_SystemError, "CryptoMiniSat return value: %d", res);
    }

    delete cmsat;
    return result;
}

PyDoc_STRVAR(solve_doc,
"solve(clauses [, kwargs]) -> list\n\
\n\
Solve the SAT problem for the clauses, and return a solution as a\n\
list of integers, or one of the strings \"UNSAT\", \"UNKNOWN\".\n\
Please see " PYCOSAT_URL " for more details.");

/*********************** Solution Iterator *********************/

typedef struct {
    PyObject_HEAD
    MainSolver *cmsat;
    signed char *mem;           /* temporary storage */
} soliterobject;

static PyTypeObject SolIter_Type;

#define SolIter_Check(op)  PyObject_TypeCheck(op, &SolIter_Type)

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

PyDoc_STRVAR(itersolve_doc,
"itersolve(clauses [, kwargs]) -> interator\n\
\n\
Solve the SAT problem for the clauses, and return an iterator over\n\
the solutions (which are lists of integers).\n\
Please see " PYCOSAT_URL " for more details.");

static PyObject* soliter_next(soliterobject *it)
{
    PyObject *result = NULL;    /* return value */
    lbool res;

    assert(SolIter_Check(it));

    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = it->cmsat->solve();
    Py_END_ALLOW_THREADS

    switch (res) {
        case l_True:
            result = get_solution(it->cmsat);
            if (result == NULL) {
                PyErr_SetString(PyExc_SystemError, "failed to create list");
                return NULL;
            }
            /* add inverse solution to the clauses, for next interation */
            if (blocksol(it->cmsat, it->mem) < 0)
                return NULL;
            break;

        case l_False:
        case l_Undef:
            /* no more solutions -- stop iteration */
            break;
    default:
        PyErr_Format(PyExc_SystemError, "CryptoMiniSat return value: %d", res);
    }
    return result;
}

static void soliter_dealloc(soliterobject *it)
{
    PyObject_GC_UnTrack(it);
    if (it->mem)
        PyMem_Free(it->mem);
    delete it->cmsat;
    it->cmsat = NULL;
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

/*************************** Method definitions *************************/

/* declaration of methods supported by this module */
static PyMethodDef module_functions[] = {
    {"solve",     (PyCFunction) solve,     METH_VARARGS | METH_KEYWORDS,
      solve_doc},
    {"itersolve", (PyCFunction) itersolve, METH_VARARGS | METH_KEYWORDS,
      itersolve_doc},
    {NULL,        NULL}  /* sentinel */
};

PyDoc_STRVAR(module_doc, "\
pycosat: bindings to CryptoMiniSat\n\
============================\n\n\
There are two functions in this module, solve and itersolve.\n\
Please see " PYCRYPTOSAT_URL " for more details.");

/* initialization routine for the shared libary */
#ifdef IS_PY3K
static PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "pycryptosat", module_doc, -1, module_functions,
};
PyMODINIT_FUNC PyInit_pycosat(void)
#else
PyMODINIT_FUNC initpycosat(void)
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

#ifdef PYCOSAT_VERSION
    PyModule_AddObject(m, "__version__",
                       PyUnicode_FromString(MainSolver::get_version());
#endif

#ifdef IS_PY3K
    return m;
#endif
}
