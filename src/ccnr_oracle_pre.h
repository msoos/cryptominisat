/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file <soos.mate@gmail.com>

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

#pragma once

#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>
#include "solvertypes.h"
#include "oracle/utils.h"

using std::pair;
using std::make_pair;
using std::vector;

namespace CMSat  {

class Solver;

class OracleLS;

class CCNROraclePre {
public:
   CCNROraclePre (Solver* solver);
   ~CCNROraclePre();

   void init(const vector<vector<sspp::Lit>>& cls, uint32_t _num_vars, vector<int8_t>* _assump_map);
   void adjust_assumps(const vector<int>& assumps_changed);
   bool run(int64_t mems_limit = 30LL*1000LL);

private:
    OracleLS* ls = nullptr;

    void add_this_clause(const vector<sspp::Lit>& cl, int cl_num);
    vector<int> yals_lits;
};

}
