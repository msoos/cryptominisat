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

#ifndef CLAUSEVIVIFIER_H
#define CLAUSEVIVIFIER_H

#include "cmsat/Solver.h"

namespace CMSat {

class ClauseVivifier {
    public:
        ClauseVivifier(Solver& solver);
        bool vivifyClauses();
        bool vivifyClauses2(vec<Clause*>& clauses);

    private:

        /**
        @brief Records data for asymmBranch()

        Clauses are ordered accurding to sze in asymmBranch() and then some are
        checked if we could shorten them. This value records that between calls
        to asymmBranch() where we stopped last time in the list
        */
        uint32_t lastTimeWentUntil;

        /**
        @brief Sort clauses according to size
        */
        struct sortBySize
        {
            bool operator () (const Clause* x, const Clause* y)
            {
              return (x->size() > y->size());
            }
        };

        void makeNonLearntBin(const Lit lit1, const Lit lit2, const bool learnt);

        uint32_t numCalls;
        Solver& solver;
};

}

#endif //CLAUSEVIVIFIER_H
