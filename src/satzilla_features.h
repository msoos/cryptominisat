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

#ifndef SOLVE_FEATURES_H_
#define SOLVE_FEATURES_H_

#include <string>
#include <cstdint>

namespace CMSat {

//A lean summary of the instance and of the search so far, dumped to the
//`satzilla_features` table every --everypred conflicts in STATS builds
struct SatZillaFeatures
{

    //instance: irredundant clauses only
    double numVars = 0;
    double numClauses = 0;
    double var_cl_ratio = 0;
    double binary = 0; //ratio of binary clauses
    double horn = 0;   //ratio of horn clauses

    //conflicts
    double avg_confl_size = 0.0;
    double avg_confl_glue = 0.0;
    double avg_num_resolutions = 0.0;
    double learnt_bins_per_confl = 0;

    //search
    double avg_branch_depth = 0.0;
    double avg_trail_depth_delta = 0.0;
    double avg_branch_depth_delta = 0.0;
    double props_per_confl = 0.0;
    double confl_per_restart = 0.0;
    double decisions_per_conflict = 0.0;

    //learnt clause DB
    double red_glue_distr_mean = 0;
    double red_glue_distr_var = 0;
    double red_size_distr_mean = 0;
    double red_size_distr_var = 0;
};

}

#endif //SOLVE_FEATURES_H_
