/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef CLAUSEVIVIFIER_H
#define CLAUSEVIVIFIER_H

#include <vector>
#include "constants.h"
#include "SolverTypes.h"
using std::vector;

class ThreadControl;
class Clause;

class ClauseVivifier {
    public:
        ClauseVivifier(ThreadControl* control);
        bool vivify();

    private:

        //Actual algorithms used
        bool vivifyClausesNormal();
        bool vivifyClausesCache(vector<Clause*>& clauses, bool learnt);
        void       makeNonLearntBin(const Lit lit1, const Lit lit2);

        ///Sort clauses according to size
        struct SortBySize
        {
            bool operator () (const Clause* x, const Clause* y);
        };

        //Working set
        ThreadControl* control;

        //Global status
        uint32_t numCalls;
        double totalTimeAsymm;
        double totalTimeCacheLearnt;
        double totalTimeCacheNonLearnt;
        uint32_t totalNumClShortenAsymm;
        uint32_t totalNumLitsRemAsymm;
        uint32_t totalNumLitsRemCacheLearnt;
        uint32_t totalNumLitsRemCacheNonLearnt;
};

#endif //CLAUSEVIVIFIER_H
