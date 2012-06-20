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

#ifndef CLAUSECLEANER_H
#define CLAUSECLEANER_H

#include "cmsat/constants.h"
#include "cmsat/Solver.h"
#include "cmsat/Subsumer.h"
#include "cmsat/XorSubsumer.h"

namespace CMSat {

/**
@brief Cleans clauses from false literals & removes satisfied clauses
*/
class ClauseCleaner
{
    public:
        ClauseCleaner(Solver& solver);

        enum ClauseSetType {clauses, binaryClauses, xorclauses, learnts};

        void cleanClauses(vec<Clause*>& cs, ClauseSetType type, const uint32_t limit = 0);

        void cleanClauses(vec<XorClause*>& cs, ClauseSetType type, const uint32_t limit = 0);
        void removeSatisfiedBins(const uint32_t limit = 0);
        //void removeSatisfied(vec<Clause*>& cs, ClauseSetType type, const uint32_t limit = 0);
        //void removeSatisfied(vec<XorClause*>& cs, ClauseSetType type, const uint32_t limit = 0);
        void removeAndCleanAll(const bool nolimit = false);
        bool satisfied(const Clause& c) const;
        bool satisfied(const XorClause& c) const;

    private:
        bool satisfied(const Watched& watched, Lit lit);
        bool cleanClause(XorClause& c);
        bool cleanClause(Clause*& c);

        uint32_t lastNumUnitarySat[6]; ///<Last time we cleaned from satisfied clauses, this many unitary clauses were known
        uint32_t lastNumUnitaryClean[6]; ///<Last time we cleaned from satisfied clauses&false literals, this many unitary clauses were known

        Solver& solver;
};

/**
@brief Removes all satisfied clauses, and cleans false literals

There is a heuristic in place not to try to clean all the time. However,
this limit can be overridden with "nolimit"
@p nolimit set this to force cleaning&removing. Useful if a really clean
state is needed, which is important for certain algorithms
*/
inline void ClauseCleaner::removeAndCleanAll(const bool nolimit)
{
    //uint32_t limit = std::min((uint32_t)((double)solver.order_heap.size() * PERCENTAGECLEANCLAUSES), FIXCLEANREPLACE);
    uint32_t limit = (double)solver.order_heap.size() * PERCENTAGECLEANCLAUSES;
    if (nolimit) limit = 0;

    removeSatisfiedBins(limit);
    cleanClauses(solver.clauses, ClauseCleaner::clauses, limit);
    cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses, limit);
    cleanClauses(solver.learnts, ClauseCleaner::learnts, limit);
}

}

#endif //CLAUSECLEANER_H
