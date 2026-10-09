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

#include "cl_predictors_abs.h"
#include "solver.h"
#include "predict_features_gen.h"

using namespace CMSat;

int ClPredictorsAbst::get_step_size()
{
    return PRED_COLS;
}

//The features of the best_features file, code generated at build time
//(scripts/crystal/gen_pred_features.py) so training and solving agree
int ClPredictorsAbst::set_up_input(
    const CMSat::Clause* const cl,
    const uint64_t sum_conflicts,
    const double   act_ranking_rel,
    const double   uip1_ranking_rel,
    const double   prop_ranking_rel,
    const double   /*sum_uip1_per_time_ranking*/,
    const double   /*sum_props_per_time_ranking*/,
    const double   sum_uip1_per_time_ranking_rel,
    const double   sum_props_per_time_ranking_rel,
    const ReduceCommonData& commdata,
    const Solver* solver,
    float* at)
{
    const ClauseStatsExtra& extra_stats = solver->red_stats_extra[cl->stats.extra_pos];
    assert(cl->stats.last_touched_any <= sum_conflicts);
    assert(extra_stats.introduced_at_conflict <= sum_conflicts);
    const predgen::In in {cl, extra_stats, solver, commdata, sum_conflicts,
        act_ranking_rel, uip1_ranking_rel, prop_ranking_rel,
        sum_uip1_per_time_ranking_rel, sum_props_per_time_ranking_rel};
    predgen::fill_features(in, missing_val, at);
    if (dump) {
        dump_raw.resize(dump_raw.size() + predgen::NUM_RAW);
        predgen::fill_raw(in, dump_raw.data() + dump_raw.size() - predgen::NUM_RAW);
    }
    return PRED_COLS;
}

void ClPredictorsAbst::open_dump(const std::string& fname)
{
    dump = fopen(fname.c_str(), "wb");
    if (dump == nullptr) {
        std::cerr << "ERROR: cannot open --preddump file '" << fname << "'" << std::endl;
        exit(-1);
    }
    const uint32_t head[2] = {(uint32_t)predgen::NUM_RAW, (uint32_t)PRED_COLS};
    fwrite(head, sizeof(uint32_t), 2, dump);
}

//per reduce: num, the conflicts so far, then of each clause the ID it is
//tracked by (0: not tracked), the raw columns, the features, the prediction
void ClPredictorsAbst::write_dump(const float* feats, const vector<double>& preds, const vector<int64_t>& ids, uint64_t conflicts)
{
    const uint32_t num = preds.size();
    assert(dump_raw.size() == (size_t)num * predgen::NUM_RAW);
    assert(ids.size() == (size_t)num);
    fwrite(&num, sizeof(uint32_t), 1, dump);
    fwrite(&conflicts, sizeof(uint64_t), 1, dump);
    fwrite(ids.data(), sizeof(int64_t), ids.size(), dump);
    fwrite(dump_raw.data(), sizeof(double), dump_raw.size(), dump);
    fwrite(feats, sizeof(float), (size_t)num * PRED_COLS, dump);
    fwrite(preds.data(), sizeof(double), preds.size(), dump);
    dump_raw.clear();
}
