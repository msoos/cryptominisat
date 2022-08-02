/******************************************
Copyright (c) 2020, Mate Soos

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

#include "src/cl_predictors_abs.h"
#include "src/cl_predictors_xgb.h"
#include "src/clause.h"
#include "cryptominisat5/solvertypesmini.h"
#include <vector>
#include <string>
#include <fstream>
#include <complex>
using std::vector;
using std::string;
using namespace CMSat;
ClPredictorsAbst* pred;

//TODO read in
struct Dat {
    uint32_t props_made;
    uint32_t orig_glue;
    uint32_t glue;
    uint32_t glue_before_minim;
    uint32_t sum_uip1_used;
    uint32_t num_antecedents;
    uint32_t num_total_lits_antecedents;
    uint32_t uip1_used;
    float    numResolutionsHistLT_avg;
    float    glueHist_longterm_avg;
    float    conflSizeHistLT_avg;
    float    branchDepthHistQueue_avg;
    double   act_ranking_rel;
    uint32_t size;
    uint64_t sumConflicts; //time_inside_the_solver
    float    correct_val;

    void print()
    {
        cout << "props_made: "  << props_made << endl;
        cout << "orig_glue: " << orig_glue << endl;
        cout << "glue: " << glue << endl;
        cout << "glue_before_minim: " << glue_before_minim << endl;
        cout << "sum_uip1_used: "  << sum_uip1_used << endl;
        cout << "num_antecedents: " << num_antecedents << endl;
        cout << "num_total_lits_antecedents: " << num_total_lits_antecedents << endl;
        cout << "uip1_used: " << uip1_used << endl;
        cout << "numResolutionsHistLT_avg: " << numResolutionsHistLT_avg << endl;
        cout << "glueHist_longterm_avg: " << glueHist_longterm_avg << endl;
        cout << "conflSizeHistLT_avg: " << conflSizeHistLT_avg << endl;
        cout << "branchDepthHistQueue_avg: "  << branchDepthHistQueue_avg << endl;
        cout << "act_ranking_rel: "  << act_ranking_rel << endl;
        cout << "size: "  << size << endl;
        cout << "sumConflicts: " << sumConflicts << endl;
        cout << "correct_val: "  << correct_val << endl;
    }
};

std::ifstream infile;

bool get_val(Dat& dat)
{
    std::string line;
    std::getline(infile, line);
    if (!infile) {
        return false;
    }
    std::istringstream iss(line);

    if (!(
        iss
        >> dat.props_made
        >> dat.orig_glue
        >> dat.glue
        >> dat.glue_before_minim
        >> dat.sum_uip1_used
        >> dat.num_antecedents
        >> dat.num_total_lits_antecedents
        >> dat.uip1_used
        >> dat.numResolutionsHistLT_avg
        >> dat.glueHist_longterm_avg
        >> dat.conflSizeHistLT_avg
        >> dat.branchDepthHistQueue_avg
        >> dat.act_ranking_rel
        >> dat.size
        >> dat.sumConflicts
        >> dat.correct_val
    )) {
        assert(false);
    }
    return true;
}

float get_predict(Clause* cl, const Dat& dat, predict_type pred_type)
{
    cl->stats.props_made = dat.props_made;
    cl->stats.orig_glue  = dat.orig_glue;
    cl->stats.numResolutionsHistLT_avg = dat.numResolutionsHistLT_avg;
    cl->stats.glue_before_minim = dat.glue_before_minim;
    cl->stats.sum_uip1_used = dat.sum_uip1_used;
    cl->stats.glue = dat.glue;
    cl->stats.num_antecedents = dat.num_antecedents;
    cl->stats.num_total_lits_antecedents = dat.num_total_lits_antecedents;
    cl->stats.glueHist_longterm_avg = dat.glueHist_longterm_avg;
//     cl->stats.conflSizeHistLT_avg = dat.conflSizeHistLT_avg;
//     cl->stats.branchDepthHistQueue_avg = dat.branchDepthHistQueue_avg;
    cl->stats.uip1_used = dat.uip1_used;
    cl->resize(dat.size);


    ReduceCommonData commondata(0, 0, 0, 0, 0);
    float val = pred->predict(
        pred_type,
        cl,
        dat.sumConflicts, //this is the age
        dat.act_ranking_rel,
        0,
        0,
        commondata
    );
    return val;
}

void run_one(string fname, predict_type pred_type)
{
    vector<Lit> lits;
    lits.resize(5000);
    Clause* cl;

    infile.open(fname.c_str());
    if (!infile) {
        cout << "ERROR: couldn't open file " << fname << endl;
        exit(-1);
    }
    BASE_DATA_TYPE* mem = (BASE_DATA_TYPE*) malloc((5000+1000)*sizeof(BASE_DATA_TYPE));

    double error = 0;
    uint32_t num_done = 0;
    while(true) {
        Dat dat;
        cl = new (mem) Clause(lits, 0);
        bool ok = get_val(dat);
        if (!ok) {
            break;
        }
        num_done++;
//         dat.print();
        float pred = get_predict(cl, dat, pred_type);
//         cout << "predicted: " << pred << endl;
//         cout << " --- " << endl;
        error += std::pow(pred-dat.correct_val,2);

    }
    free(mem);
    cout << "For file " << fname << " error is: " << error/(double)num_done << " done: " << num_done << endl;
    infile.close();
}

int main()
{
    pred = new ClPredictors;
    string base = "../src/predict/";
    string s = "predictor_short.json";
    string l = "predictor_long.json";
    string f = "predictor_forever.json";
    pred->load_models(base+s, base+l, base+f);

    run_one("ml_perf_test.txt-short", predict_type::short_pred);
    run_one("ml_perf_test.txt-long", predict_type::long_pred);
    run_one("ml_perf_test.txt-forever", predict_type::forever_pred);

    delete pred;
    return 0;
}
