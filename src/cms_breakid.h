/******************************************
Copyright (c) 2019, Mate Soos

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

#ifndef CMS_BREAKID_H
#define CMS_BREAKID_H

#include <vector>
#include "cryptominisat5/solvertypesmini.h"

using std::vector;

namespace BID {
class BreakID;
}

namespace CMSat {

class Solver;

class BreakID {
public:
    BreakID(Solver* solver);
    bool doit();
    void finished_solving();

private:
    void break_symms();

    enum class add_cl_ret {added_cl, skipped_cl, unsat};
    template<class T>
    add_cl_ret add_this_clause(const T& cl);
    vector<Lit> brkid_lits;

    bool already_called = false;
    uint32_t symm_var = var_Undef;
    Solver* solver;
    BID::BreakID* breakid = NULL;
};

}

#endif
