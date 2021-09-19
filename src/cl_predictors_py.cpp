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
            Py_DecRef(pRet[i]);
        }
    }
    Py_Finalize();
}

int ClPredictorsPy::load_models(const std::string& short_fname,
                                const std::string& long_fname,
                                const std::string& forever_fname,
                                const std::string& module_fname)
{

//     std::vector<wchar_t*> wargv;
//     for(uint32_t i=0; i < argc; i++) {
//         wargv.push_back(charToWChar(argv[i]));
//
//     }
//     PySys_SetArgv(argc, wargv.data());
    Py_Initialize();
    import_array();
    wchar_t *tmp = charToWChar(module_fname.c_str());
    PySys_SetArgv(1, &tmp);

    //Initialize Python
//     std::wstring pypath(Py_GetPath());
//     std::wcout << L"path: " << pypath << std::endl;
    //pypath = pypath + L":" + PY_SITEPACKAGES + L":" + MX_PYMODULE_DIR + L":" + NUMPY_PYPATH;

//     std::wstring widestr = std::wstring(module_fname.begin(), module_fname.end());
//     pypath += L":" + widestr;
//     Py_SetPath(pypath.c_str());

//     PyObject* sysPath = PySys_GetObject((char*)"path");
//     PyObject* programName = PyUnicode_FromString("ml_module.py");
//     PyList_Append(sysPath, programName);
//     Py_DECREF(programName);

    std::wstring pypath2(Py_GetPath());
    std::wcout << L"path: " << pypath2 << std::endl;

    //Py_Initialize();
    //import_array();


    /*
    // Convert the file name to a Python string.
    PyObject* pName = PyUnicode_FromString("ml_module");

    //Add dir to sys path
    PyObject* programName = PyUnicode_FromString("ml_module.py");
    PyList_Append(sysPath, programName);
    Py_DECREF(programName);

    // Import the file as a Python module.
    PyObject *pModule = PyImport_Import(pName);
    */

    PyObject* pName = PyUnicode_FromString("ml_module");
    /* Error checking of pName left out */
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (pModule == NULL) {
        PyErr_Print();
        cout << "ERROR: Failed to load \"" + module_fname + "\"!" << endl;
        exit(-1);
    }

    // Create a dictionary for the contents of the module.
    pDict = PyModule_GetDict(pModule);
    pFunc = PyDict_GetItemString(pDict, "predict");

    //Load models
    PyObject * load_models = PyDict_GetItemString(pDict, "load_models");
    PyObject *pArgs = PyTuple_New(3);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(short_fname.c_str()));
    PyTuple_SetItem(pArgs, 1, PyUnicode_FromString(long_fname.c_str()));
    PyTuple_SetItem(pArgs, 2, PyUnicode_FromString(forever_fname.c_str()));

    PyObject_CallObject(load_models, pArgs);
    return 1;
}

int ClPredictorsPy::load_models_from_buffers()
{
    cout << "ERROR: it is not possible to load models from buffer in Python mode" << endl;
    exit(-1);
    return 0;
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
    dims[1] = PRED_COLS;
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
    Py_DecRef(pResult);

//     //Test data transfer
//     for(uint32_t i =0; i < 20; i++) {
//         cout << std::setprecision(20) <<
//         out_result[0][i]
//         << std::setprecision(2)
//         << endl;
//     }
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
    if (pRet[0] != NULL) {
        for(uint32_t i=0; i < 3; i++) {
            assert(pRet[i]);
            Py_DecRef(pRet[i]);
            pRet[i] = NULL;
        }
    }
}


