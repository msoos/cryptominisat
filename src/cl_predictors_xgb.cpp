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
#include "clause.h"
#include "solver.h"
#include <cmath>
#include <sstream>
#include <fstream>
extern char predictor_short_json[];
extern unsigned int predictor_short_json_len;

extern char predictor_long_json[];
extern unsigned int predictor_long_json_len;

extern char predictor_forever_json[];
extern unsigned int predictor_forever_json_len;

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
    handles.resize(3);
    safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::short_pred])))
    safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::long_pred])))
    safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::forever_pred])))

    for(int i = 0; i < 3; i++) {
        safe_xgboost(XGBoosterSetParam(handles[i], "nthread", "1"))
        //safe_xgboost(XGBoosterSetParam(handles[i], "verbosity", "3"))
    }
}

ClPredictorsXGB::~ClPredictorsXGB()
{
    for(auto& h: handles) {
        XGBoosterFree(h);
    }
}

int ClPredictorsXGB::load_models(const std::string& short_fname,
                               const std::string& long_fname,
                               const std::string& forever_fname,
                               const std::string& best_feats_fname)
{
    safe_xgboost(XGBoosterLoadModel(handles[predict_type::short_pred], short_fname.c_str()))
    safe_xgboost(XGBoosterLoadModel(handles[predict_type::long_pred], long_fname.c_str()))
    safe_xgboost(XGBoosterLoadModel(handles[predict_type::forever_pred], forever_fname.c_str()))
    return 1;
}

int ClPredictorsXGB::load_models_from_buffers()
{
    safe_xgboost(XGBoosterLoadModelFromBuffer(
        handles[predict_type::short_pred], predictor_short_json, predictor_short_json_len));
    safe_xgboost(XGBoosterLoadModelFromBuffer(
        handles[predict_type::long_pred], predictor_long_json, predictor_long_json_len));
    safe_xgboost(XGBoosterLoadModelFromBuffer(
        handles[predict_type::forever_pred], predictor_forever_json, predictor_forever_json_len))
    return 0;
}

void ClPredictorsXGB::predict_all(
    float* const data,
    const uint32_t num)
{
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
    safe_xgboost(XGBoosterPredict(
        handles[short_pred],
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result_short
    ))
    assert(out_len == num);

    safe_xgboost(XGBoosterPredict(
        handles[long_pred],
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result_long
    ))
    assert(out_len == num);

    safe_xgboost(XGBoosterPredict(
        handles[forever_pred],
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result_forever
    ))
    assert(out_len == num);
}

void ClPredictorsXGB::get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at)
{
    extdata.pred_short_use = (double)out_result_short[at];
    extdata.pred_long_use = (double)out_result_long[at];
    extdata.pred_forever_use = (double)out_result_forever[at];
}

void CMSat::ClPredictorsXGB::finish_all_predict()
{
    safe_xgboost(XGDMatrixFree(dmat))
}
