/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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
#include "simplifier.h"
#include "solver.h"

namespace CMSat {

/**
@brief Cleans clauses from false literals & removes satisfied clauses
*/
class ClauseCleaner
{
    public:
        ClauseCleaner(Solver* solver);

        void cleanClauses(vector<ClOffset>& cs);


        void treatImplicitClauses();
        void removeAndCleanAll();
        bool satisfied(const Clause& c) const;

    private:
        bool satisfied(const Watched& watched, Lit lit);
        bool cleanClause(ClOffset c);

        Solver* solver;
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
    treatImplicitClauses();
    cleanClauses(solver->longIrredCls);
    cleanClauses(solver->longRedCls);

#ifndef NDEBUG
    //Once we have cleaned the watchlists
    //no watchlist whose lit is set may be non-empty
    size_t wsLit = 0;
    for(watch_array::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        if (solver->value(lit) != l_Undef) {
            assert(it->empty());
        }
    }
#endif

    if (solver->conf.verbosity >= 1) {
        cout
        << "c [clean] T: "
        << std::fixed << std::setprecision(4)
        << (cpuTime() - myTime)
        << " s" << endl;
    }
}

} //end namespace

#endif //CLAUSECLEANER_H
