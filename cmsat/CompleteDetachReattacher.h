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

#include "cmsat/Solver.h"

namespace CMSat {

/**
@brief Helper class to completely detaches all(or only non-native) clauses, and then re-attach all

Used in classes that (may) do a lot of clause-changning, in which case
detaching&reattaching of clauses would be neccessary to do
individually, which is \b very slow

A main use-case is the following:
-# detach all clauses
-# play around with all clauses as desired. Cannot call solver.propagate() here
-# attach all clauses again

A somewhat more complicated, but more interesting use-case is the following:
-# detach only non-natively stored clauses from watchlists
-# play around wil all clauses as desired. 2- and 3-long clauses can still
be propagated with solver.propagate() -- this is quite a nice trick, in fact
-# detach all clauses (i.e. also native ones)
-# attach all clauses
*/
class CompleteDetachReatacher
{
    public:
        CompleteDetachReatacher(Solver& solver);
        bool reattachNonBins();
        void detachNonBinsNonTris(const bool removeTri);

    private:
        class ClausesStay {
            public:
                ClausesStay() :
                    learntBins(0)
                    , nonLearntBins(0)
                    , tris(0)
                {}

                ClausesStay& operator+=(const ClausesStay& other) {
                    learntBins += other.learntBins;
                    nonLearntBins += other.nonLearntBins;
                    tris += other.tris;
                    return *this;
                }

                uint32_t learntBins;
                uint32_t nonLearntBins;
                uint32_t tris;
        };
        const ClausesStay clearWatchNotBinNotTri(vec<Watched>& ws, const bool removeTri = false);

        void cleanAndAttachClauses(vec<Clause*>& cs);
        void cleanAndAttachClauses(vec<XorClause*>& cs);
        bool cleanClause(Clause*& ps);
        bool cleanClause(XorClause& ps);

        Solver& solver;
};

}
