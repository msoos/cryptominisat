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
    assert(handles.size() < PRED_MAX_MODELS);
    handles.push_back(nullptr);
    safe_xgboost(XGBoosterCreate(0, 0, &handles.back()))
    safe_xgboost(XGBoosterSetParam(handles.back(), "nthread", "1"))
}

//a model trained on another feature list would silently predict garbage:
//the count must match, and the names when the model carries them
void ClPredictorsXGB::check_num_features(const std::string& what)
{
    bst_ulong n = 0;
    safe_xgboost(XGBoosterGetNumFeature(handles.back(), &n))
    if (n != (bst_ulong)PRED_COLS) {
        std::cerr << "ERROR: the model " << what << " has " << n
            << " features, this binary computes " << PRED_COLS
            << " (its best_features.txt). Retrain, or rebuild with the list the model was trained on" << std::endl;
        exit(-1);
    }
    bst_ulong len = 0;
    const char** names = nullptr;
    safe_xgboost(XGBoosterGetStrFeatureInfo(handles.back(), "feature_name", &len, &names))
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

ClPredictorsXGB::~ClPredictorsXGB()
{
    for(auto& h: handles) {
        XGBoosterFree(h);
    }
}

int ClPredictorsXGB::load_models(const vector<std::string>& fnames,
                               const std::string& /*best_feats_fname*/)
{
    NoFPTraps no_traps;
    for(const auto& f: fnames) {
        new_handle();
        safe_xgboost(XGBoosterLoadModel(handles.back(), f.c_str()))
        check_num_features(f);
    }
    num_models = handles.size();
    return 1;
}

int ClPredictorsXGB::load_models_from_buffers(const vector<std::string>& tiers)
{
    NoFPTraps no_traps;
    for(const auto& t: tiers) {
        const EmbeddedModel* m = nullptr;
        for(unsigned i = 0; i < embedded_models_num; i++) {
            if (t == embedded_models[i].tier) m = &embedded_models[i];
        }
        if (m == nullptr) {
            std::cerr << "ERROR: no model for tier '" << t << "' is compiled in, only:";
            for(unsigned i = 0; i < embedded_models_num; i++) std::cerr << " " << embedded_models[i].tier;
            std::cerr << std::endl;
            return 1;
        }
        new_handle();
        safe_xgboost(XGBoosterLoadModelFromBuffer(handles.back(), m->data, m->len));
        check_num_features("compiled in for tier " + t);
    }
    num_models = handles.size();
    return 0;
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

//For checking in python using check_against_binary_dat
#if 0
        std::stringstream s;
        s << "bin_dump" << num_dumps << ".csv";
        std::ofstream f;
        f.open(s.str().c_str());
        float* data_ptr = data;
        for(uint32_t i = 0; i < num; i ++) {
            std::stringstream line;
            for(uint32_t i2 = 0; i2 < PRED_COLS; i2++) {
                line << std::setprecision(30) << *data_ptr;
                if (i2+1 < PRED_COLS) {
                    line << ",";
                }
                data_ptr++;
            }
            f << line.str() << endl;
        }
        f.close();
        num_dumps++;
#endif

    bst_ulong out_len;
    for(uint32_t i = 0; i < num_models; i++) {
        safe_xgboost(XGBoosterPredict(
            handles[i],
            dmat,
            0,  //0: normal prediction
            0,  //use all trees
            0,  //do not use for training
            &out_len,
            &out_result[i]
        ))
        assert(out_len == num);
    }
}

void ClPredictorsXGB::get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at)
{
    for(uint32_t i = 0; i < PRED_MAX_MODELS; i++) {
        extdata.pred_use[i] = i < num_models ? (double)out_result[i][at] : 0;
    }
}

void CMSat::ClPredictorsXGB::finish_all_predict()
{
    NoFPTraps no_traps;
    safe_xgboost(XGDMatrixFree(dmat))
}
