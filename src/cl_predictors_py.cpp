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
    const double   sum_uip1_per_time_ranking,
    const double   sum_props_per_time_ranking,
    const double   sum_uip1_per_time_ranking_rel,
    const double   sum_props_per_time_ranking_rel,
    const ReduceCommonData& commdata,
    const Solver* solver,
    float* at)
{
    int x = 0;

    const ClauseStatsExtra& extra_stats = solver->red_stats_extra[cl->stats.extra_pos];
    uint32_t last_touched_any_diff = sumConflicts - (uint64_t)cl->stats.last_touched_any;
    double time_inside_solver = sumConflicts - (uint64_t)extra_stats.introduced_at_conflict;

    at[x++] = cl->stats.is_ternary_resolvent;
    at[x++] = cl->stats.which_red_array;
    at[x++] = cl->stats.last_touched_any;
    at[x++] = act_ranking_rel;
    at[x++] = uip1_ranking_rel;
    at[x++] = prop_ranking_rel;
    at[x++] = last_touched_any_diff;
    at[x++] = time_inside_solver;
    at[x++] = cl->stats.props_made;
    at[x++] = commdata.avg_props;
    at[x++] = commdata.avg_uip;
    at[x++] = solver->hist.conflSizeHistLT.avg();
    at[x++] = solver->hist.glueHistLT.avg();
    at[x++] = extra_stats.sum_props_made;
    at[x++] = extra_stats.discounted_props_made;
    at[x++] = extra_stats.discounted_props_made2;
    at[x++] = extra_stats.discounted_props_made3;
    at[x++] = extra_stats.discounted_uip1_used;
    at[x++] = extra_stats.discounted_uip1_used2;
    at[x++] = extra_stats.discounted_uip1_used3;
    at[x++] = extra_stats.sum_uip1_used;
    at[x++] = cl->stats.uip1_used;
    at[x++] = cl->size();
    at[x++] = sum_uip1_per_time_ranking;
    at[x++] = sum_props_per_time_ranking;
    at[x++] = sum_uip1_per_time_ranking_rel;
    at[x++] = sum_props_per_time_ranking_rel;
    at[x++] = cl->distilled;

    //Ternary resolvents lack glue and antecedent data
    if (cl->stats.is_ternary_resolvent) {
        for(int i = 0; i < 14; i++) {
            at[x++] = missing_val;
        }
    } else {
        at[x++] = extra_stats.glueHist_avg;
        at[x++] = extra_stats.antecedents_binIrred;
        at[x++] = extra_stats.glueHistLT_avg;
        at[x++] = extra_stats.glueHist_longterm_avg;
        at[x++] = extra_stats.num_antecedents;
        at[x++] = extra_stats.overlapHistLT_avg;
        at[x++] = extra_stats.conflSizeHist_avg;
        at[x++] = extra_stats.antecedents_binred;
        at[x++] = extra_stats.num_total_lits_antecedents;
        at[x++] = extra_stats.numResolutionsHistLT_avg;
        at[x++] = cl->stats.glue;
        at[x++] = extra_stats.orig_glue;
        at[x++] = extra_stats.glue_before_minim;
        at[x++] = extra_stats.trail_depth_level;
    }
    assert(x == NUM_RAW_FEATS);

    return NUM_RAW_FEATS;
}


ClPredictorsPy::ClPredictorsPy()
{
    for(uint32_t i=0; i < 3; i++) {
        ret_data[i] = NULL;
    }
}

ClPredictorsPy::~ClPredictorsPy()
{
    //Free if we didn't already
    if (ret_data[0]) {
        for(uint32_t i=0; i < 3; i++) {
            assert(ret_data[i]);
            Py_DECREF(ret_data[i]);
        }
    }
    Py_DECREF(pFunc);

    Py_Finalize();
}

int ClPredictorsPy::load_models(const std::string& short_fname,
                                const std::string& long_fname,
                                const std::string& forever_fname,
                                const std::string& best_feats_fname)
{
    Py_Initialize();
    import_array();
    wchar_t *tmp = charToWChar(best_feats_fname.c_str());
    PySys_SetArgv(1, &tmp);

    std::wstring pypath2(Py_GetPath());
    //std::wcout << L"path: " << pypath2 << std::endl;

    PyObject* pName = PyUnicode_FromString("ml_module");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (pModule == NULL) {
        PyErr_Print();
        cout << "ERROR: Failed to load ml_module from the same place as \"" + best_feats_fname + "\"!" << endl;
        exit(-1);
    }

    // Create a dictionary for the contents of the module.
    pDict = PyModule_GetDict(pModule);
    pFunc = PyDict_GetItemString(pDict, "predict");

    // Set up features
    PyObject *set_up_features = PyDict_GetItemString(pDict, "set_up_features");
    pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(best_feats_fname.c_str()));
    PyObject* ret = PyObject_CallObject(set_up_features, pArgs);
    if (ret == NULL) {
        PyErr_Print();
        cout << "ERROR: Failed to set up features!" << endl;
        exit(-1);
    }
    //Py_DECREF(set_up_features);
    Py_DECREF(pArgs);
    Py_DECREF(ret);


    //Load models
    PyObject* load_models = PyDict_GetItemString(pDict, "load_models");
    pArgs = PyTuple_New(3);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(short_fname.c_str()));
    PyTuple_SetItem(pArgs, 1, PyUnicode_FromString(long_fname.c_str()));
    PyTuple_SetItem(pArgs, 2, PyUnicode_FromString(forever_fname.c_str()));
    ret = PyObject_CallObject(load_models, pArgs);
    if (ret == NULL) {
        PyErr_Print();
        cout << "ERROR: Failed to load models !" << endl;
        exit(-1);
    }
    //Py_DECREF(load_models);
    Py_DECREF(pArgs);
    Py_DECREF(ret);
    //Py_DECREF(pModule);
    //Py_DECREF(pDict);
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

    // Create NumPy 2D array with data
    npy_intp dims[2];
    dims[0] = num;
    dims[1] = NUM_RAW_FEATS;
    pArray = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT, data);
    assert(pArray != NULL);

    // Tuple to hold the arguments to the method
    pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, pArray);

    // Call the function with the arguments
    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
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
        ret_data[i] = (PyArrayObject *)PyArray_ContiguousFromObject(pRet[i], NPY_DOUBLE, 1, 1);

        //NOTE: PyArray_DATA has no effect on the reference count of the array it is applied to
        //      see: https://stackoverflow.com/questions/37919094/decrefing-after-a-call-to-pyarray-data
        //      So no decrefing needed for this one
        out_result[i] = (double*)PyArray_DATA(ret_data[i]);
        assert(PyArray_SIZE(ret_data[i]) == num);
        //Py_DECREF(pRet[i]);
    }

    //This should decrement all elements in the array, so we incremented it above.
    Py_DECREF(pResult);
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
    if (ret_data[0] != NULL) {
        for(uint32_t i=0; i < 3; i++) {
//             assert(pRet[i]);
//             Py_DECREF(pRet[i]);
//             pRet[i] = NULL;

            assert(ret_data[i]);
            Py_DECREF(ret_data[i]);
            ret_data[i] = NULL;
        }
    }
}


