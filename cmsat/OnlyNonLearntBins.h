/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef ONLYNONLEARNTBINS_H
#define ONLYNONLEARNTBINS_H

#include "cmsat/Solver.h"

namespace CMSat {

/**
@brief Handles propagation, addition&removal of non-learnt binary clauses

It takes a snapshot of Solver's non-learnt binary clauses, builds its own
watchlists, and does everything itself.*/
class OnlyNonLearntBins
{
    public:
        OnlyNonLearntBins(Solver& solver);

        /**
        @brief For storing a binary clause
        */
        class WatchedBin {
        public:
            WatchedBin(Lit _impliedLit) : impliedLit(_impliedLit) {};
            Lit impliedLit;
        };

        //propagation
        bool propagate();

        //Management of clauses
        bool fill();

        //helper
        inline uint32_t getWatchSize(const Lit lit) const;
        inline const vec<vec<WatchedBin> >& getBinWatches() const;

    private:
        vec<vec<WatchedBin> > binwatches; ///<Internal wathclists for non-learnt binary clauses

        Solver& solver;
};

inline uint32_t OnlyNonLearntBins::getWatchSize(const Lit lit) const
{
    return binwatches[lit.toInt()].size();
}

inline const vec<vec<OnlyNonLearntBins::WatchedBin> >& OnlyNonLearntBins::getBinWatches() const
{
    return binwatches;
}

}

#endif //ONLYNONLEARNTBINS_H
