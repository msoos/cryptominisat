/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************************/

#ifndef CLAUSECLEANER_H
#define CLAUSECLEANER_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "Solver.h"
#include "Subsumer.h"

class ClauseCleaner
{
    public:
        ClauseCleaner(Solver& solver);
        
        enum ClauseSetType {clauses, xorclauses, learnts, binaryClauses, simpClauses};
        
        void cleanClauses(vec<Clause*>& cs, ClauseSetType type, const uint limit = 0);
        void cleanClausesBewareNULL(vec<ClauseSimp>& cs, ClauseSetType type, Subsumer& subs, const uint limit = 0);
        void cleanClauses(vec<XorClause*>& cs, ClauseSetType type, const uint limit = 0);
        void removeSatisfied(vec<Clause*>& cs, ClauseSetType type, const uint limit = 0);
        void removeSatisfied(vec<XorClause*>& cs, ClauseSetType type, const uint limit = 0);
        void removeAndCleanAll();
        bool satisfied(const Clause& c) const;
        bool satisfied(const XorClause& c) const;
        
    private:
        const bool cleanClause(Clause& c);
        const bool cleanClauseBewareNULL(ClauseSimp& c, Subsumer& subs);
        const bool cleanClause(XorClause& c);
        
        uint lastNumUnitarySat[5];
        uint lastNumUnitaryClean[5];
        
        Solver& solver;
};

inline void ClauseCleaner::removeAndCleanAll()
{
    //uint limit = std::min((uint)((double)solver.order_heap.size() * PERCENTAGECLEANCLAUSES), FIXCLEANREPLACE);
    uint limit = (double)solver.order_heap.size() * PERCENTAGECLEANCLAUSES;
    
    removeSatisfied(solver.binaryClauses, ClauseCleaner::binaryClauses, limit);
    cleanClauses(solver.clauses, ClauseCleaner::clauses, limit);
    cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses, limit);
    cleanClauses(solver.learnts, ClauseCleaner::learnts, limit);
}

#endif //CLAUSECLEANER_H
