/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
***********************************************/

#include "cl_predictors_py.h"
#include "clause.h"
#include "solver.h"
#include <cmath>

static wchar_t* charToWChar(const char* text)
{
    const size_t size = strlen(text) + 1;
    wchar_t* wText = new wchar_t[size];
    mbstowcs(wText, text, size);
    return wText;
}

extern char predictor_short_json[];
extern unsigned int predictor_short_json_len;

extern char predictor_long_json[];
extern unsigned int predictor_long_json_len;

extern char predictor_forever_json[];
extern unsigned int predictor_forever_json_len;


using namespace CMSat;

int ClPredictorsPy::set_up_input(
    const CMSat::Clause* const cl,
    const uint64_t sumConflicts,
    const double   act_ranking_rel,
    const double   uip1_ranking_rel,
    const double   prop_ranking_rel,
    const double   sum_uip1_per_time_ranking_rel,
    const double   sum_props_per_time_ranking_rel,
    const ReduceCommonData& commdata,
    const Solver* solver,
    float* at)
{
    int x = 0;

    const ClauseStatsExtra& extra_stats = solver->red_stats_extra[cl->stats.extra_pos];
    uint32_t last_touched_diff = sumConflicts - (uint64_t)cl->stats.last_touched;
    double time_inside_solver = sumConflicts - (uint64_t)extra_stats.introduced_at_conflict;

    at[x++] = extra_stats.orig_glue;
    at[x++] = cl->stats.last_touched;
    at[x++] = act_ranking_rel;
    at[x++] = uip1_ranking_rel;
    at[x++] = prop_ranking_rel;
    at[x++] = last_touched_diff;
    at[x++] = time_inside_solver;
    at[x++] = cl->stats.props_made;
    at[x++] = commdata.avg_props;
    at[x++] = commdata.avg_glue;
    at[x++] = commdata.avg_uip;
    at[x++] = extra_stats.sum_props_made;
    at[x++] = extra_stats.discounted_props_made;
    at[x++] = extra_stats.discounted_uip1_used;
    at[x++] = extra_stats.sum_uip1_used;
    at[x++] = cl->stats.uip1_used;
    at[x++] = cl->stats.glue;

    //Ternary resolvents lack glue and antecedent data
    if (cl->stats.is_ternary_resolvent) {
        at[x++] = missing_val;
        at[x++] = missing_val;
        at[x++] = missing_val;
        at[x++] = missing_val;
        at[x++] = missing_val;
        at[x++] = missing_val;
        at[x++] = missing_val;
        at[x++] = missing_val;
    } else {
        at[x++] = extra_stats.glueHist_avg;
        at[x++] = extra_stats.antecedents_binIrred;
        at[x++] = extra_stats.glueHistLT_avg;
        at[x++] = extra_stats.glueHist_longterm_avg;
        at[x++] = extra_stats.num_antecedents;
        at[x++] = extra_stats.overlapHistLT_avg;
        at[x++] = extra_stats.conflSizeHist_avg;
        at[x++] = extra_stats.antecedents_binred;
    }
    assert(x == NUM_RAW_FEATS);
    //at[x++] = sum_uip1_per_time_ranking_rel;
    //at[x++] = sum_props_per_time_ranking_rel;

    return NUM_RAW_FEATS;
}


ClPredictorsPy::ClPredictorsPy()
{
    for(uint32_t i=0; i < 3; i++) {
        pRet[i] = NULL;
    }
}

ClPredictorsPy::~ClPredictorsPy()
{
    //Free if we didn't already
    if (pRet[0]) {
        for(uint32_t i=0; i < 3; i++) {
            assert(pRet[i]);
            Py_DECREF(pRet[i]);
        }
    }
    Py_Finalize();
}

int ClPredictorsPy::load_models(const std::string& short_fname,
                                const std::string& long_fname,
                                const std::string& forever_fname,
                                const std::string& module_fname)
{
    Py_Initialize();
    import_array();
    wchar_t *tmp = charToWChar(module_fname.c_str());
    PySys_SetArgv(1, &tmp);

    std::wstring pypath2(Py_GetPath());
    //std::wcout << L"path: " << pypath2 << std::endl;

    PyObject* pName = PyUnicode_FromString("ml_module");
    PyObject* pModule = PyImport_Import(pName);
//     Py_DECREF(pName);
    if (pModule == NULL) {
        PyErr_Print();
        cout << "ERROR: Failed to load \"" + module_fname + "\"!" << endl;
        exit(-1);
    }

    // Create a dictionary for the contents of the module.
    pDict = PyModule_GetDict(pModule);
    pFunc = PyDict_GetItemString(pDict, "predict");

    // Set up features
    PyObject * set_up_features = PyDict_GetItemString(pDict, "set_up_features");
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(module_fname.c_str()));
    PyObject_CallObject(set_up_features, pArgs);
//     Py_DECREF(set_up_features);
//     Py_DECREF(pArgs);


    //Load models
    PyObject * load_models = PyDict_GetItemString(pDict, "load_models");
    pArgs = PyTuple_New(3);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(short_fname.c_str()));
    PyTuple_SetItem(pArgs, 1, PyUnicode_FromString(long_fname.c_str()));
    PyTuple_SetItem(pArgs, 2, PyUnicode_FromString(forever_fname.c_str()));
    PyObject_CallObject(load_models, pArgs);
//     Py_DECREF(load_models);
//     Py_DECREF(pArgs);
    return 1;
}

int ClPredictorsPy::load_models_from_buffers()
{
    cout << "ERROR: it is not possible to load models from buffer in Python mode" << endl;
    exit(-1);
    return 1;
}

void ClPredictorsPy::predict_all(
    float* const data,
    const uint32_t num)
{
    if (num == 0) {
        return;
    }

//     // Test data transfer
//     cout << "data[0][0]: " << data[0] << endl;
//     cout << "data[1][0]: " << data[PRED_COLS] << endl;
//     cout << "data[1][1]: " << data[PRED_COLS+1] << endl;

    // Create NumPy 2D array with data
    npy_intp dims[2];
    dims[0] = num;
    dims[1] = NUM_RAW_FEATS;
    PyObject *pArray = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT, data);
    assert(pArray != NULL);

    // Tuple to hold the arguments to the method
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, pArray);

    // Call the function with the arguments
    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
    if(pResult == NULL) {
        PyErr_Print();
        cout << "Calling the add method failed" << endl;
        exit(-1);
    }

    if (pResult->ob_type != &PyList_Type) {
        cout << "ERROR: you didn't return List" << endl;
        exit(-1);
    }

    // See: https://numpy.org/devdocs/user/c-info.beyond-basics.html#basic-iteration
    uint32_t num_elems = PyList_Size(pResult);
    assert(num_elems == 3);
    for(uint32_t i = 0; i < 3; i++) {
        pRet[i] = PyList_GetItem(pResult, i);
        PyArrayObject* a_dat = (PyArrayObject *)PyArray_ContiguousFromObject(pRet[i], NPY_DOUBLE, 1, 1);
        out_result[i] = (double*)PyArray_DATA(a_dat);
        assert(PyArray_SIZE(a_dat) == num);
        Py_IncRef(pRet[i]);
    }

    //This should decrement all elements in the array, so we incremented it above.
    Py_DECREF(pResult);

//     //Test data transfer
//     for(uint32_t i =0; i < 20; i++) {
//         cout << std::setprecision(20) <<
//         out_result[0][i]
//         << std::setprecision(2)
//         << endl;
//     }
//     Py_DECREF(pResult);
}

void ClPredictorsPy::get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at)
{
    extdata.pred_short_use   = out_result[0][at];
    extdata.pred_long_use    = out_result[1][at];
    extdata.pred_forever_use = out_result[2][at];
}

void CMSat::ClPredictorsPy::finish_all_predict()
{
    //Free previous result
    if (pRet[0] != NULL) {
        for(uint32_t i=0; i < 3; i++) {
            assert(pRet[i]);
            Py_DECREF(pRet[i]);
            pRet[i] = NULL;
        }
    }
}


