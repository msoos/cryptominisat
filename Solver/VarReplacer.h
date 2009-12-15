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

#include "SolverTypes.h"
#include "Clause.h"
#include "Vec.h"

#include <sys/types.h>
#include <map>
#include <vector>
using std::map;
using std::vector;

class Solver;

class VarReplacer
{
    public:
        VarReplacer(Solver* S);
        ~VarReplacer();
        void replace(vec<Lit>& ps, const bool xor_clause_inverted, const uint group);
        void extendModel() const;
        void performReplace();
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
        void replace_set(vec<Clause*>& set);
        void replace_set(vec<XorClause*>& cs, const bool isAttached);
        const bool handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2);
        const bool handleUpdatedClause(XorClause& c, const Var origVar1, const Var origVar2);
        void addBinaryXorClause(vec<Lit>& ps, const bool xor_clause_inverted, const uint group, const bool internal = false);
        
        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        
        vector<Lit> table;
        map<Var, vector<Var> > reverseTable;
        vec<Clause*> clauses;
        
        uint replacedLits;
        uint lastReplacedLits;
        uint replacedVars;
        uint lastReplacedVars;
        bool addedNewClause;
        Solver* S;
};

#endif //VARREPLACER_H
