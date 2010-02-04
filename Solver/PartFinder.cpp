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

#include "PartFinder.h"

#include "Solver.h"
#include "Gaussian.h"
#include "GaussianConfig.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "VarReplacer.h"

#include <set>
#include <map>
#include <iomanip>
#include <math.h>
#include "FailedVarSearcher.h"
using std::set;
using std::map;

//#define VERBOSE_DEBUG

using std::cout;
using std::endl;

//#define PART_FINDING

PartFinder::PartFinder(Solver& _solver) :
    solver(_solver)
{
}

const uint PartFinder::findParts()
{
    double time = cpuTime();
    
    table.clear();
    table.resize(solver.nVars(), var_Undef);
    reverseTable.clear();
    part_no = 0;
    
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    solver.clauseCleaner->cleanClauses(solver.binaryClauses, ClauseCleaner::xorclauses);
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::xorclauses);
    
    addToPart(&(solver.clauses));
    addToPart(&(solver.binaryClauses));
    addToPart((vec<Clause*>*)(&(solver.xorclauses)));
    addToPart(&(solver.varReplacer->getClauses()));
    
    setParts();
    
    std::cout << "Time:" << cpuTime() - time << endl;
    
    
    return 0;
}

void PartFinder::addToPart(vec<Clause*> const* cs2)
{
    const vec<Clause*>& cs = *cs2;
    
    for (Clause* const* c = cs.getData(), *const*end = c + cs.size(); c != end; c++) {
        set<uint> tomerge;
        vector<Var> newSet;
        for (Lit *l = &(**c)[0], *end2 = l + (**c).size(); l != end2; l++) {
            if (table[l->var()] != var_Undef)
                tomerge.insert(table[l->var()]);
            else
                newSet.push_back(l->var());
        }
        if (tomerge.size() == 1) {
            const uint into = *tomerge.begin();
            map<uint, vector<Var> >::iterator intoReverse = reverseTable.find(into);
            for (uint i = 0; i < newSet.size(); i++) {
                intoReverse->second.push_back(newSet[i]);
                table[newSet[i]] = into;
            }
            continue;
        }
        
        for (set<uint>::iterator it = tomerge.begin(); it != tomerge.end(); it++) {
            newSet.insert(newSet.end(), reverseTable[*it].begin(), reverseTable[*it].end());
            reverseTable.erase(*it);
        }
        for (uint i = 0; i < newSet.size(); i++)
            table[newSet[i]] = part_no;
        reverseTable[part_no] = newSet;
        part_no++;
    }
}

const uint PartFinder::setParts()
{
    vector<uint> numClauseInPart(part_no, 0);
    vector<uint> sumXorSizeInPart(part_no, 0);
    
    calcIn(&solver.clauses, numClauseInPart, sumXorSizeInPart);
    calcIn(&solver.binaryClauses, numClauseInPart, sumXorSizeInPart);
    calcIn((vec<Clause*>*)(&(solver.xorclauses)), numClauseInPart, sumXorSizeInPart);
    calcIn(&(solver.varReplacer->getClauses()), numClauseInPart, sumXorSizeInPart);
 
    uint parts = 0;
    for (uint i = 0; i < numClauseInPart.size(); i++) {
        if (sumXorSizeInPart[i] == 0) continue;
        std::cout << "!!! part " << parts << " size:" << numClauseInPart[i] << std::endl;
        std::cout << "!!! part " << parts << " sum size:" << sumXorSizeInPart[i]  << std::endl;
        parts++;
    }
    std::cout << "Num interesting parts:" << parts << endl;
    
    if (parts > 1) {
        #ifdef VERBOSE_DEBUG
        for (map<uint, vector<Var> >::iterator it = reverseTable.begin(), end = reverseTable.end(); it != end; it++) {
            cout << "-- set begin --" << endl;
            for (vector<Var>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
                cout << *it2 << ", ";
            }
            cout << "-------" << endl;
        }
        #endif
    }
    
    return part_no;
}

void PartFinder::calcIn(vec<Clause*>const* cs2, vector<uint>& numClauseInPart, vector<uint>& sumXorSizeInPart)
{
    const vec<Clause*>& cs = *cs2;
    
    for (Clause*const* c = cs.getData(), *const*end = c + cs.size(); c != end; c++) {
        Clause& x = **c;
        const uint part = table[x[0].var()];
        assert(part < part_no);
        
        //for stats
        numClauseInPart[part]++;
        sumXorSizeInPart[part] += x.size();
    }
}

