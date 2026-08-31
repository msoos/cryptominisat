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

#ifndef SLS_H_
#define SLS_H_

#include "solvertypes.h"

namespace CMSat {

class Solver;

class SLS {
public:
    SLS(Solver* solver);
    ~SLS() = default;

    // One local search round, as CaDiCaL's 'walk_round'. Overwrites the saved
    // phases whenever a new global minimum of unsatisfied clauses is reached.
    void run(const int64_t mems);

    // CaDiCaL's 'walk': a round with the effort relative to the search so far.
    void run_during_search();

    // CaDiCaL's 'local_search': rounds run before the CDCL loop starts.
    void run_initially();

    vector<vector<uint8_t>> run_alter(const int64_t mems, uint32_t num);

private:
    Solver* solver;

    int64_t effort() const;
    uint64_t approx_mem_needed() const;
    bool enough_mem() const;
};

} //end namespace CMSat

#endif //SLS_H_
