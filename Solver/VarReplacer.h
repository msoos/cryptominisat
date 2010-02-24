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

#ifndef VARREPLACER_H
#define VARREPLACER_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include <map>
#include <vector>
using std::map;
using std::vector;

#include "Solver.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "Vec.h"

class VarReplacer
{
    public:
        VarReplacer(Solver& solver);
        ~VarReplacer();
        const bool performReplace(const bool always = false);
        template<class T>
        const bool replace(T& ps, const bool xor_clause_inverted, const uint group);
        void extendModel() const;
        const uint getNumReplacedLits() const;
        const uint getNumReplacedVars() const;
        const uint getNumLastReplacedVars() const;
        const uint getNewToReplaceVars() const;
        const vector<Var> getReplacingVars() const;
        const vector<Lit>& getReplaceTable() const;
        const vec<Clause*>& getClauses() const;
        void newClause();
        void newVar();
    
    private:
        const bool performReplaceInternal();
        
        void replace_set(vec<Clause*>& set);
        void replace_set(vec<XorClause*>& cs, const bool isAttached);
        const bool handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2);
        const bool handleUpdatedClause(XorClause& c, const Var origVar1, const Var origVar2);
        template<class T>
        void addBinaryXorClause(T& ps, const bool xor_clause_inverted, const uint group, const bool internal = false);
        
        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        
        vector<Lit> table;
        map<Var, vector<Var> > reverseTable;
        vec<Clause*> clauses;
        
        uint replacedLits;
        uint replacedVars;
        uint lastReplacedVars;
        bool addedNewClause;
        Solver& solver;
};

inline const bool VarReplacer::performReplace(const bool always)
{
    //uint32_t limit = std::min((uint32_t)((double)solver.order_heap.size()*PERCENTAGEPERFORMREPLACE), FIXCLEANREPLACE);
    uint32_t limit = (uint32_t)((double)solver.order_heap.size()*PERCENTAGEPERFORMREPLACE);
    if ((always && getNewToReplaceVars() > 0) || getNewToReplaceVars() > limit)
        return performReplaceInternal();
    
    return true;
}

inline const uint VarReplacer::getNumReplacedLits() const
{
    return replacedLits;
}

inline const uint VarReplacer::getNumReplacedVars() const
{
    return replacedVars;
}

inline const uint VarReplacer::getNumLastReplacedVars() const
{
    return lastReplacedVars;
}

inline const uint VarReplacer::getNewToReplaceVars() const
{
    return replacedVars-lastReplacedVars;
}

inline const vector<Lit>& VarReplacer::getReplaceTable() const
{
    return table;
}

inline const vec<Clause*>& VarReplacer::getClauses() const
{
    return clauses;
}

#endif //VARREPLACER_H
