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

#ifndef _DISTILLERLITREM_H_
#define _DISTILLERLITREM_H_

#include <vector>
#include "clause.h"
#include "constants.h"
#include "solvertypes.h"
#include "cloffset.h"
#include "watcharray.h"

namespace CMSat {

using std::vector;

class Solver;
class Clause;

class DistillerLitRem {
    public:
        explicit DistillerLitRem(Solver* solver);
        bool distill_lit_rem();

        struct Stats
        {
            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator+=(const Stats& other);
            void print_short(const Solver* solver) const;
            void print(const size_t nVars) const;

            double time_used = 0.0;
            uint64_t timeOut = 0;
            uint64_t zeroDepthAssigns = 0;
            uint64_t numLitsRem = 0;
            uint64_t checkedClauses = 0;
            uint64_t potentialClauses = 0;
            uint64_t cls_tried = 0;
            uint64_t numCalled = 0;
            uint64_t numClShorten = 0;
        };

        const Stats& get_stats() const;
        double mem_used() const;

    private:
        bool distill_long_cls_all(vector<ClOffset>& offs, double time_mult);
        bool go_through_clauses(vector<ClOffset>& cls, uint32_t at);
        ClOffset try_distill_clause_and_return_new(
            ClOffset offset
            , const ClauseStats* const stats
            , uint32_t at
        );
        Solver* solver;

        //For distill
        vector<Lit> lits;
        uint64_t oldBogoProps;
        int64_t maxNumProps;
        int64_t orig_maxNumProps;

        //Global status
        Stats runStats;
        Stats globalStats;
        size_t numCalls = 0;

};

inline const DistillerLitRem::Stats& DistillerLitRem::get_stats() const
{
    return globalStats;
}

} //end namespace

#endif //_DISTILLERLITREM_H_
