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
#include "FailedLitSearcher.h"

//#define VERBOSE_DEBUG

using namespace CMSat;
using std::set;
using std::map;

//#define PART_FINDING

PartFinder::PartFinder(Solver& _solver) :
    solver(_solver)
{
}

const bool PartFinder::findParts()
{
    assert(solver.conf.doReplace);

    double time = cpuTime();

    table.clear();
    table.resize(solver.nVars(), std::numeric_limits<uint32_t>::max());
    reverseTable.clear();
    part_no = 0;

    solver.clauseCleaner->removeAndCleanAll(true);
    if (!solver.ok) return false;
    while (solver.varReplacer->getNewToReplaceVars() > 0) {
        if (solver.conf.doReplace && !solver.varReplacer->performReplace(true))
            return false;
        solver.clauseCleaner->removeAndCleanAll(true);
        if (!solver.ok) return false;
    }

    addToPart(solver.clauses);
    addToPartBins();
    addToPart(solver.xorclauses);

    const uint32_t parts = setParts();

    #ifndef NDEBUG
    for (map<uint32_t, vector<Var> >::const_iterator it = reverseTable.begin(); it != reverseTable.end(); it++) {
        for (uint32_t i2 = 0; i2 < it->second.size(); i2++) {
            assert(table[(it->second)[i2]] == it->first);
        }
    }
    #endif

    if (solver.conf.verbosity  >= 3 || (solver.conf.verbosity  >=1 && parts > 1)) {
        std::cout << "c Found parts: " << std::setw(10) <<  parts
        << " time: " << std::setprecision(2) << std::setw(4) << cpuTime() - time
        << " s"
        << std::endl;
    }

    return true;
}

template<class T>
void PartFinder::addToPart(const vec<T*>& cs)
{
    for (T* const* c = cs.getData(), * const*end = c + cs.size(); c != end; c++) {
        if ((*c)->learnt()) continue;
        addToPartClause(**c);
    }
}

void PartFinder::addToPartBins()
{
    vec<Lit> lits(2);
    uint32_t wsLit = 0;
    for (const vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        lits[0] = lit;
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                if (it2->getLearnt()) continue;
                lits[1] = it2->getOtherLit();
                addToPartClause(lits);
            }
        }
    }
}

template<class T>
void PartFinder::addToPartClause(T& cl)
{
    set<uint32_t> tomerge;
    vector<Var> newSet;
    for (const Lit *l = cl.getData(), *end2 = cl.getDataEnd(); l != end2; l++) {
        if (table[l->var()] != std::numeric_limits<uint32_t>::max())
            tomerge.insert(table[l->var()]);
        else
            newSet.push_back(l->var());
    }
    if (tomerge.size() == 1) {
        //no trees to merge, only merge the clause into one tree

        const uint32_t into = *tomerge.begin();
        map<uint32_t, vector<Var> >::iterator intoReverse = reverseTable.find(into);
        for (uint32_t i = 0; i < newSet.size(); i++) {
            intoReverse->second.push_back(newSet[i]);
            table[newSet[i]] = into;
        }
        return;
    }

    for (set<uint32_t>::iterator it = tomerge.begin(); it != tomerge.end(); it++) {
        newSet.insert(newSet.end(), reverseTable[*it].begin(), reverseTable[*it].end());
        reverseTable.erase(*it);
    }

    for (uint32_t i = 0; i < newSet.size(); i++)
        table[newSet[i]] = part_no;
    reverseTable[part_no] = newSet;
    part_no++;
}

const uint32_t PartFinder::setParts()
{
    vector<uint32_t> numClauseInPart(part_no, 0);
    vector<uint32_t> sumLitsInPart(part_no, 0);

    calcIn(solver.clauses, numClauseInPart, sumLitsInPart);
    calcInBins(numClauseInPart, sumLitsInPart);
    calcIn(solver.xorclauses, numClauseInPart, sumLitsInPart);

    uint32_t parts = 0;
    for (uint32_t i = 0; i < numClauseInPart.size(); i++) {
        if (sumLitsInPart[i] == 0) continue;
        if (solver.conf.verbosity  >= 3 || ( solver.conf.verbosity  >= 1 && reverseTable.size() > 1) ) {
            std::cout << "c Found part " << std::setw(8) << i
            << " vars: " << std::setw(10) << reverseTable[i].size()
            << " clauses:" << std::setw(10) << numClauseInPart[i]
            << " lits size:" << std::setw(10) << sumLitsInPart[i]
            << std::endl;
        }
        parts++;
    }

    if (parts > 1) {
        #ifdef VERBOSE_DEBUG
        for (map<uint32_t, vector<Var> >::iterator it = reverseTable.begin(), end = reverseTable.end(); it != end; it++) {
            cout << "-- set begin --" << endl;
            for (vector<Var>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
                cout << *it2 << ", ";
            }
            cout << "-------" << endl;
        }
        #endif
    }

    return parts;
}

void PartFinder::calcInBins(vector<uint32_t>& numClauseInPart, vector<uint32_t>& sumLitsInPart)
{
    uint32_t wsLit = 0;
    for (const vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                if (it2->getLearnt()) continue;

                const uint32_t part = table[lit.var()];
                assert(part < part_no);
                numClauseInPart[part]++;
                sumLitsInPart[part] += 2;
            }
        }
    }
}

template<class T>
void PartFinder::calcIn(const vec<T*>& cs, vector<uint32_t>& numClauseInPart, vector<uint32_t>& sumLitsInPart)
{
    for (T*const* c = cs.getData(), *const*end = c + cs.size(); c != end; c++) {
        if ((*c)->learnt()) continue;
        T& x = **c;
        const uint32_t part = table[x[0].var()];
        assert(part < part_no);

        //for stats
        numClauseInPart[part]++;
        sumLitsInPart[part] += x.size();
    }
}
