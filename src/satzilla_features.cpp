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

#include "satzilla_features.h"
#include <iostream>

using std::cout;
using std::endl;
using namespace CMSat;

void SatZillaFeatures::print_stats(const std::string& prefix) const
{
    cout << prefix << "[satzilla_features]"
    << " numVars " << numVars
    << " numClauses " << numClauses
    << " var_cl_ratio " << var_cl_ratio
    << " binary " << binary
    << " horn " << horn
    << " avg_confl_size " << avg_confl_size
    << " avg_confl_glue " << avg_confl_glue
    << " avg_num_resolutions " << avg_num_resolutions
    << " learnt_bins_per_confl " << learnt_bins_per_confl
    << " avg_branch_depth " << avg_branch_depth
    << " avg_trail_depth_delta " << avg_trail_depth_delta
    << " avg_branch_depth_delta " << avg_branch_depth_delta
    << " props_per_confl " << props_per_confl
    << " confl_per_restart " << confl_per_restart
    << " decisions_per_conflict " << decisions_per_conflict
    << " red_glue_distr_mean " << red_glue_distr_mean
    << " red_glue_distr_var " << red_glue_distr_var
    << " red_size_distr_mean " << red_size_distr_mean
    << " red_size_distr_var " << red_size_distr_var
    << endl;
}
