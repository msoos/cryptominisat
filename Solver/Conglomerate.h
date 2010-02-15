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

#ifndef CONGLOMERATE_H
#define CONGLOMERATE_H

#include <vector>
#include <map>
#include <set>
#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "Clause.h"
#include "VarReplacer.h"
#include "Solver.h"

using std::vector;
using std::pair;
using std::map;
using std::set;

class Solver;

class Conglomerate
{
public:
    Conglomerate(Solver& solver);
    ~Conglomerate();
    const bool conglomerateXors(); ///<Conglomerate XOR-s that are attached using a variable
    void addRemovedClauses(); ///<Add clauses that have been removed. Used if solve() is called multiple times
    void doCalcAtFinish(); ///<Calculate variables removed during conglomeration
    const vec<XorClause*>& getCalcAtFinish() const;
    vec<XorClause*>& getCalcAtFinish();
    const vector<bool>& getRemovedVars() const;
    void newVar();
    const uint getFound();
    const uint getFoundBin();
    
private:
    
    struct ClauseSetSorter {
        bool operator () (const pair<XorClause*, uint32_t>& a, const pair<XorClause*, uint32_t>& b) {
            return a.first->size() < b.first->size();
        }
    };
    
    const bool conglomerateXorsInternal(const bool noblock);
    void removeVar(const Var var);
    void processClause(XorClause& x, uint32_t num, Var remove_var);
    void blockVars();
    void fillVarToXor();
    void clearDouble(vector<Lit>& ps) const;
    void clearToRemove();
    void clearLearntsFromToRemove();
    bool dealWithNewClause(vector<Lit>& ps, const bool inverted, const uint old_group);
    
    typedef map<uint, vector<pair<XorClause*, uint32_t> > > varToXorMap;
    varToXorMap varToXor; 
    vector<bool> blocked;
    vector<bool> toRemove;
    vector<bool> removedVars;
    
    vec<XorClause*> calcAtFinish;
    uint found;
    uint replacedShorter;
    
    Solver& solver;
};

inline const vector<bool>& Conglomerate::getRemovedVars() const
{
    return removedVars;
}

inline const uint Conglomerate::getFound()
{
    return found;
}

inline const vec<XorClause*>& Conglomerate::getCalcAtFinish() const
{
    return calcAtFinish;
}

inline vec<XorClause*>& Conglomerate::getCalcAtFinish()
{
    return calcAtFinish;
}


#endif //CONGLOMERATE_H
