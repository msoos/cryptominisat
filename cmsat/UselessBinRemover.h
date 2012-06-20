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

#ifndef USELESSBINREMOVER_H
#define USELESSBINREMOVER_H

#include "cmsat/Vec.h"
#include "cmsat/Solver.h"
#include "cmsat/constants.h"

namespace CMSat {

/**
@brief Removes binary clauses that are effectively useless to have

These binary clauses are useless, because for example clauses:
a V b
-b V c

exist, so the binary clause:
a V c

is useless in every possible way. Here, we remove such claues. Unfortunately,
currently we only remove useless non-learnt binary clauses. Learnt useless
binary clauses are not removed.

\todo Extend such that it removes learnt useless binary clauses as well
*/
class UselessBinRemover {
    public:
        UselessBinRemover(Solver& solver);
        bool removeUslessBinFull();

    private:
        bool failed; ///<Has the previous propagation failed? (=conflict)
        uint32_t extraTime; ///<Time that cannot be meausured in bogoprops (~propagation time)

        //Remove useless binaries
        bool fillBinImpliesMinusLast(const Lit origLit, const Lit lit, vec<Lit>& wrong);
        bool removeUselessBinaries(const Lit lit);
        void removeBin(const Lit lit1, const Lit lit2);
        /**
        @brief Don't delete the same binary twice, and don't assume that deleted binaries still exist

        Not deleting the same binary twice is easy to understand. He hard part
        is the second thought. Basically, once we set "a->b" to be deleted, for
        instance, we should not check whether any one-hop neighbours of "a" can
        be reached from "b", since the binary clause leading to "b" has already
        been deleted (and, by the way, checked, since the place where we can
        reach "b" from already checked for all places we can visit from "b")
        */
        vec<char> toDeleteSet;
        vec<Lit> oneHopAway; ///<Lits that are one hop away from selected lit (sometimes called origLit)
        /**
        @brief Binary clauses to be removed are gathered here

        We only gather the second lit, the one we can reach from selected lit
        (calld origLit some places) see fillBinImpliesMinusLast() for details
        */
        vec<Lit> wrong;

        Solver& solver; ///<The solver class e want to remove useless binary clauses from
};

}

#endif //USELESSBINREMOVER_H
