/******************************************
Copyright (c) 2014, Mate Soos

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

#include <cryptominisat4/cryptominisat.h>
#include "src/time_mem.h"
#include <assert.h>
#include <vector>
#include <iostream>

using std::cout;
using std::endl;
using std::vector;
using namespace CMSat;

int main()
{
    SATSolver s;
    s.set_no_simplify();

    const size_t num = 100*1000ULL;

    s.new_vars(num);
    vector<Lit> lits(3);
    for(size_t i = 2; i < num; i++) {
        if (i % 100 != 0)
            continue;

        lits[0] = Lit(i-2, false);
        lits[1] = Lit(i-1, false);
        lits[2] = Lit(i, true);
        s.add_clause(lits);
    }
    cout << "Adding of clauses finished" << endl;

    s.solve();
    double start = cpuTime();
    vector<Lit> assumptions(1);
    for(size_t i = 0; i < 600; i++) {
        assumptions[0] = Lit(i, false);
        s.solve(&assumptions);
    }
    double end = cpuTime();
    cout << "T: " << (end-start) << endl;
    assert(end-start < 15);
}
