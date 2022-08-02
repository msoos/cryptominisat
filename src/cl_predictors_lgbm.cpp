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

#include "cl_predictors_lgbm.h"
#include "clause.h"
#include "solver.h"
#include <cmath>
extern char predictor_short_json[];
extern unsigned int predictor_short_json_len;

extern char predictor_long_json[];
extern unsigned int predictor_long_json_len;

extern char predictor_forever_json[];
extern unsigned int predictor_forever_json_len;

using namespace CMSat;

ClPredictorsLGBM::ClPredictorsLGBM()
{
//     safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::short_pred])))
//     safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::long_pred])))
//     safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::forever_pred])))
//
//     for(int i = 0; i < 3; i++) {
//         safe_xgboost(XGBoosterSetParam(handles[i], "nthread", "1"))
//         //safe_xgboost(XGBoosterSetParam(handles[i], "verbosity", "3"))
//     }
}

ClPredictorsLGBM::~ClPredictorsLGBM()
{
    for(uint32_t i = 0; i < 3; i++) {
        LGBM_BoosterFree(handle[i]);
    }
}

int ClPredictorsLGBM::load_models(const std::string& short_fname,
                               const std::string& long_fname,
                               const std::string& forever_fname,
                               const std::string& best_feats_fname)
{
    int ret;

    ret = LGBM_BoosterCreateFromModelfile(short_fname.c_str(), &num_iterations[0], &handle[0]);
    assert(ret == 0);

    ret = LGBM_BoosterCreateFromModelfile(long_fname.c_str(), &num_iterations[1], &handle[1]);
    assert(ret == 0);

    ret = LGBM_BoosterCreateFromModelfile(forever_fname.c_str(), &num_iterations[2], &handle[2]);
    assert(ret == 0);
    return 1;
}

int ClPredictorsLGBM::load_models_from_buffers()
{
    assert(false);
    exit(-1);
//     safe_xgboost(XGBoosterLoadModelFromBuffer(
//         handles[predict_type::short_pred], predictor_short_json, predictor_short_json_len));
//     safe_xgboost(XGBoosterLoadModelFromBuffer(
//         handles[predict_type::long_pred], predictor_long_json, predictor_long_json_len));
//     safe_xgboost(XGBoosterLoadModelFromBuffer(
//         handles[predict_type::forever_pred], predictor_forever_json, predictor_forever_json_len))
    return 1;
}

void ClPredictorsLGBM::predict_all(
    float* const data,
    const uint32_t num)
{
    for(uint32_t i = 0; i < 3; i ++) {
        out_result[i].resize(num);
        int64_t out_len;

        auto ret = LGBM_BoosterPredictForMat(
            handle[i],
            data,
            C_API_DTYPE_FLOAT32, //type of "data"
            num, //num rows
            PRED_COLS, //num features
            1, //row major
            C_API_PREDICT_NORMAL, // what should be predicted: normal, raw score, etc.
            0, //start iteration
            -1,
            "n_jobs=1", //other parameters for prediction (const char*)
            &out_len, //length of output
            out_result[i].data());
        assert(ret == 0);
        assert(out_len == num);
    }
}

/*void ClPredictorsLGBM::predict_all(
    float* data,
    uint32_t num)
{
    for(uint32_t i = 0; i < 3; i ++) {
        out_result[i].resize(num);
        double* out = out_result[i].data();
        int64_t out_len;

        for(uint32_t x = 0; x < num; x++) {
            auto ret = LGBM_BoosterPredictForMatSingleRow(
                handle[i],
                data,
                C_API_DTYPE_FLOAT32, //type of "data"
                PRED_COLS, //num features
                1, //row major
                C_API_PREDICT_NORMAL, // what should be predicted: normal, raw score, etc.
                0, //start iteration
                -1,
                "n_jobs=1", //other parameters for prediction (const char*)
                &out_len, //length of output
                out
            );
            assert(ret == 0);
            assert(out_len == 1);
            data += PRED_COLS;
            out ++;

        }
    }
}*/

void ClPredictorsLGBM::get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at)
{
    extdata.pred_short_use   = out_result[0][at];
    extdata.pred_long_use    = out_result[1][at];
    extdata.pred_forever_use = out_result[2][at];
}

void CMSat::ClPredictorsLGBM::finish_all_predict()
{
    //safe_xgboost(XGDMatrixFree(dmat))
}
