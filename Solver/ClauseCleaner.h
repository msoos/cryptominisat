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

#ifndef CLAUSECLEANER_H
#define CLAUSECLEANER_H

#include "constants.h"
#include "Subsumer.h"
#include "ThreadControl.h"

/**
@brief Cleans clauses from false literals & removes satisfied clauses
*/
class ClauseCleaner
{
    public:
        ClauseCleaner(ThreadControl* control);

        enum ClauseSetType {clauses, binaryClauses, learnts};

        void cleanClauses(vector<Clause*>& cs, ClauseSetType type, const uint32_t limit = 0);


        void removeSatisfiedBins(const uint32_t limit = 0);
        void removeAndCleanAll();
        bool satisfied(const Clause& c) const;

    private:
        bool satisfied(const Watched& watched, Lit lit);
        bool cleanClause(Clause*& c);

        uint32_t lastNumUnitarySat[6]; ///<Last time we cleaned from satisfied clauses, this many unitary clauses were known
        uint32_t lastNumUnitaryClean[6]; ///<Last time we cleaned from satisfied clauses&false literals, this many unitary clauses were known

        ThreadControl* control;
};

/**
@brief Removes all satisfied clauses, and cleans false literals

There is a heuristic in place not to try to clean all the time. However,
this limit can be overridden with "nolimit"
@p nolimit set this to force cleaning&removing. Useful if a really clean
state is needed, which is important for certain algorithms
*/
inline void ClauseCleaner::removeAndCleanAll()
{
    double myTime = cpuTime();
    removeSatisfiedBins(0);
    cleanClauses(control->clauses, ClauseCleaner::clauses, 0);
    cleanClauses(control->learnts, ClauseCleaner::learnts, 0);

    if (control->conf.verbosity >= 1) {
        cout
        << "c [clean] T: "
        << std::fixed << std::setprecision(2)
        << (cpuTime() - myTime)
        << " s" << endl;
    }
}

#endif //CLAUSECLEANER_H
