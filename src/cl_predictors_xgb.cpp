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

#include "cl_predictors_xgb.h"
#include <iostream>
#include "clause.h"
#include "solver.h"
#include "predict_features_gen.h"
#include <cmath>
#include <sstream>
#include <fstream>

#define safe_xgboost(call) {  \
  int err = (call); \
  if (err != 0) { \
    fprintf(stderr, "%s:%d: error in %s: %s\n", __FILE__, __LINE__, #call, XGBGetLastError());  \
    exit(1); \
  } \
}

using namespace CMSat;

ClPredictorsXGB::ClPredictorsXGB()
{
}

void ClPredictorsXGB::new_handle()
{
    assert(handle == nullptr);
    safe_xgboost(XGBoosterCreate(0, 0, &handle))
    safe_xgboost(XGBoosterSetParam(handle, "nthread", "1"))
}

//a model trained on another feature list would silently predict garbage:
//the count must match, and the names when the model carries them
void ClPredictorsXGB::check_num_features(const std::string& what)
{
    bst_ulong n = 0;
    safe_xgboost(XGBoosterGetNumFeature(handle, &n))
    if (n != (bst_ulong)PRED_COLS) {
        std::cerr << "ERROR: the model " << what << " has " << n
            << " features, this binary computes " << PRED_COLS
            << " (its best_features.txt). Retrain, or rebuild with the list the model was trained on" << std::endl;
        exit(-1);
    }
    bst_ulong len = 0;
    const char** names = nullptr;
    safe_xgboost(XGBoosterGetStrFeatureInfo(handle, "feature_name", &len, &names))
    if (len == 0) return; //trained without names, the count is all there is
    for(bst_ulong i = 0; i < len && i < (bst_ulong)PRED_COLS; i++) {
        if (std::string(names[i]) != predgen::feature_names[i]) {
            std::cerr << "ERROR: the model " << what << " was trained on feature " << i
                << " = '" << names[i] << "', this binary computes '" << predgen::feature_names[i]
                << "' there (its best_features.txt). Retrain, or rebuild with the list the model was trained on"
                << std::endl;
            exit(-1);
        }
    }
}

//the training ranges and provenance the model carries as attributes, if any
void ClPredictorsXGB::read_attrs()
{
    auto attr = [&](const char* name) -> std::string {
        const char* out = nullptr;
        int ok = 0;
        safe_xgboost(XGBoosterGetAttr(handle, name, &out, &ok))
        return ok ? std::string(out) : std::string();
    };
    auto nums = [](const std::string& str) {
        vector<double> ret;
        std::stringstream ss(str);
        std::string tok;
        while (ss >> tok) ret.push_back(tok == "nan" ? NAN : std::stod(tok));
        return ret;
    };
    feature_lo = nums(attr("feature_lo"));
    feature_hi = nums(attr("feature_hi"));
    if (feature_lo.size() != (size_t)PRED_COLS || feature_hi.size() != (size_t)PRED_COLS) {
        feature_lo.clear();
        feature_hi.clear();
    }
    const std::string frame = attr("train_frame");
    if (!frame.empty()) {
        provenance = attr("train_rows") + " rows of " + frame + ", " + attr("train_date")
            + ", gathered by " + attr("gathered_by");
    }
}

ClPredictorsXGB::~ClPredictorsXGB()
{
    if (handle) XGBoosterFree(handle);
}

void ClPredictorsXGB::load_model(const std::string& fname)
{
    NoFPTraps no_traps;
    new_handle();
    safe_xgboost(XGBoosterLoadModel(handle, fname.c_str()))
    check_num_features(fname);
    read_attrs();
}

void ClPredictorsXGB::load_embedded_model()
{
    NoFPTraps no_traps;
    new_handle();
    safe_xgboost(XGBoosterLoadModelFromBuffer(handle, predictor_disc_json, predictor_disc_json_len));
    check_num_features("compiled in");
    read_attrs();
}

void ClPredictorsXGB::predict_all(
    float* const data,
    const uint32_t num)
{
    NoFPTraps no_traps;
    safe_xgboost(XGDMatrixCreateFromMat(data, num, PRED_COLS, missing_val, &dmat))
    if (num == 0) {
        return;
    }

    bst_ulong out_len;
    safe_xgboost(XGBoosterPredict(
        handle,
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result
    ))
    assert(out_len == num);
}

void ClPredictorsXGB::get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at)
{
    extdata.pred_use = (double)out_result[at];
}

void CMSat::ClPredictorsXGB::finish_all_predict()
{
    NoFPTraps no_traps;
    safe_xgboost(XGDMatrixFree(dmat))
}
