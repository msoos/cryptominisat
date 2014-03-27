/*
  Copyright (c) 2013, Ilan Schnell, Continuum Analytics, Inc., Mate Soos
  Python bindings to CryptoMiniSat (http://msoos.org)
  This file is published an MIT license.
*/

#define PYCRYPTOSAT_URL  "https://pypi.python.org/pypi/pycosat"

#include <python2.7/Python.h>

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

/* Add the inverse of the (current) solution to the clauses.
   This function is essentially the same as the function blocksol in app.c
   in the picosat source. */
static int blocksol(CryptoMiniSat *cmsat, signed char *mem)
{
    int max_idx, i;

    max_idx = picosat_variables(cmsat);
    if (mem == NULL) {
        mem = PyMem_Malloc(max_idx + 1);
        if (mem == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }
    for (i = 1; i <= max_idx; i++)
        mem[i] = (picosat_deref(cmsat, i) > 0) ? 1 : -1;

    for (i = 1; i <= max_idx; i++)
        picosat_add(cmsat, (mem[i] < 0) ? i : -i);

    picosat_add(cmsat, 0);
    return 0;
}

static int add_clause(MainSolver *cmsat, PyObject *clause)
{
    PyObject *iterator;         /* each clause is an iterable of literals */
    PyObject *lit;              /* the literals are integers */
    int v;

    iterator = PyObject_GetIter(clause);
    if (iterator == NULL)
        return -1;

    while ((lit = PyIter_Next(iterator)) != NULL) {
        if (!IS_INT(lit))  {
            Py_DECREF(lit);
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_TypeError, "integer expected");
            return -1;
        }
        v = PyLong_AsLong(lit);
        Py_DECREF(lit);
        if (v == 0) {
            Py_DECREF(iterator);
            PyErr_SetString(PyExc_ValueError, "non-zero integer expected");
            return -1;
        }
        picosat_add(cmsat, v);
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred())
        return -1;
    picosat_add(cmsat, 0);
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

static MainSolver* setup_picosat(PyObject *args, PyObject *kwds)
{
    MainSolver *cmsat;
    PyObject *clauses;          /* iterable of clauses */
    int vars = -1, verbose = 0;
    unsigned long long prop_limit = 0;
    static char* kwlist[] = {"clauses",
                             "vars", "verbose", "prop_limit", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|iiK:(iter)solve", kwlist,
                                     &clauses,
                                     &vars, &verbose, &prop_limit))
        return NULL;

    cmsat = cmsat_minit(NULL, py_malloc, py_realloc, py_free);
    picosat_set_verbosity(cmsat, verbose);
    if (vars != -1)
        picosat_adjust(cmsat, vars);

    if (prop_limit)
        picosat_set_propagation_limit(cmsat, prop_limit);

    if (add_clauses(cmsat, clauses) < 0) {
        picosat_reset(cmsat);
        return NULL;
    }

    if (verbose >= 2)
        picosat_print(cmsat, stdout);

    return cmsat;
}

/* read the solution from the picosat object and return a Python list */
static PyObject* get_solution(MainSolver *cmsat)
{
    PyObject *list;
    int max_idx, i, v;

    max_idx = picosat_variables(cmsat);
    list = PyList_New((Py_ssize_t) max_idx);
    if (list == NULL) {
        picosat_reset(cmsat);
        return NULL;
    }
    for (i = 1; i <= max_idx; i++) {
        v = picosat_deref(cmsat, i);
        assert(v == -1 || v == 1);
        if (PyList_SetItem(list, (Py_ssize_t) (i - 1),
                           PyInt_FromLong((long) (v * i))) < 0) {
            Py_DECREF(list);
            picosat_reset(cmsat);
            return NULL;
        }
    }
    return list;
}

static PyObject* solve(PyObject *self, PyObject *args, PyObject *kwds)
{
    MainSolver *cmsat;
    PyObject *result = NULL;    /* return value */
    int res;

    cmsat = setup_picosat(args, kwds);
    if (cmsat == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = picosat_sat(cmsat, -1);
    Py_END_ALLOW_THREADS

    switch (res) {
    case PICOSAT_SATISFIABLE:
        result = get_solution(cmsat);
        break;

    case PICOSAT_UNSATISFIABLE:
        result = PyUnicode_FromString("UNSAT");
        break;

    case PICOSAT_UNKNOWN:
        result = PyUnicode_FromString("UNKNOWN");
        break;

    default:
        PyErr_Format(PyExc_SystemError, "CryptoMiniSat return value: %d", res);
    }

    picosat_reset(cmsat);
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

    it->cmsat = setup_picosat(args, kwds);
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
    int res;

    assert(SolIter_Check(it));

    Py_BEGIN_ALLOW_THREADS      /* release GIL */
    res = picosat_sat(it->cmsat, -1);
    Py_END_ALLOW_THREADS

    switch (res) {
    case PICOSAT_SATISFIABLE:
        result = get_solution(it->cmsat);
        if (result == NULL) {
            PyErr_SetString(PyExc_SystemError, "failed to create list");
            return NULL;
        }
        /* add inverse solution to the clauses, for next interation */
        if (blocksol(it->cmsat, it->mem) < 0)
            return NULL;
        break;

    case PICOSAT_UNSATISFIABLE:
    case PICOSAT_UNKNOWN:
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
    picosat_reset(it->cmsat);
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
Please see " PYCOSAT_URL " for more details.");

/* initialization routine for the shared libary */
#ifdef IS_PY3K
static PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "pycosat", module_doc, -1, module_functions,
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
    m = Py_InitModule3("pycosat", module_functions, module_doc);
    if (m == NULL)
        return;
#endif

#ifdef PYCOSAT_VERSION
    PyModule_AddObject(m, "__version__",
                       PyUnicode_FromString(PYCOSAT_VERSION));
#endif

#ifdef IS_PY3K
    return m;
#endif
}
