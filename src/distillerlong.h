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

#ifndef _DISTILLERLONG_H_
#define _DISTILLERLONG_H_

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

class DistillerLong {
    public:
        explicit DistillerLong(Solver* solver);
        bool distill(const bool red, bool only_rem_cl = false);

        struct Stats
        {
            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator+=(const Stats& other);
            void print(const size_t nVars) const;

            double time_used = 0.0;
            uint64_t timeOut = 0;
            uint64_t zeroDepthAssigns = 0;
            uint64_t numClShorten = 0;
            uint64_t numLitsRem = 0;
            uint64_t checkedClauses = 0;
            uint64_t potentialClauses = 0;
            uint64_t numCalled = 0;
            uint64_t clRemoved = 0;
        };

        const Stats& get_stats() const;
        double mem_used() const;

    private:
        ClOffset try_distill_clause_and_return_new(
            ClOffset offset
            , const ClauseStats* const stats
            , const bool also_remove, const bool only_remove
        );
        bool distill_long_cls_all(
            vector<ClOffset>& offs, double time_mult,
            bool also_remove,
            bool only_remove,
            bool red, uint32_t red_lev = numeric_limits<uint32_t>::max());
        bool go_through_clauses(vector<ClOffset>& cls, const bool also_remove, const bool only_remove);
        Solver* solver;

        //For distill
        vector<uint64_t> lit_counts;
        vector<Lit> lits;
        uint64_t oldBogoProps;
        int64_t maxNumProps;
        int64_t orig_maxNumProps;

        //Global status
        Stats runStats;
        Stats globalStats;
        size_t numCalls_red = 0;
        size_t numCalls_irred = 0;

};

inline const DistillerLong::Stats& DistillerLong::get_stats() const
{
    return globalStats;
}

} //end namespace

#endif //_DISTILLERLONG_H_
