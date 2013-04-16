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


#include <set>
#include <map>
#include <iomanip>
#include <iostream>
#include "partfinder.h"
#include "time_mem.h"
#include "cloffset.h"
#include "solver.h"
#include "varreplacer.h"
#include "clausecleaner.h"

using namespace CMSat;

using std::set;
using std::map;
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG



//#define PART_FINDING

PartFinder::PartFinder(Solver* _solver) :
    solver(_solver)
{
}

bool PartFinder::findParts()
{
    double time = cpuTime();

    table.clear();
    table.resize(solver->nVars(), std::numeric_limits<uint32_t>::max());
    reverseTable.clear();
    part_no = 0;
    used_part_no = 0;

    solver->clauseCleaner->removeAndCleanAll();

    if (!solver->varReplacer->performReplace())
        return false;

    //Add the clauses to the sets
    addToPartClauses(solver->longIrredCls);
    addToPartImplicits();

    //const uint32_t parts = setParts();

    #ifndef NDEBUG
    for (map<uint32_t, vector<Var> >::const_iterator
        it = reverseTable.begin()
        ; it != reverseTable.end()
        ; it++
    ) {
        for (size_t i2 = 0; i2 < it->second.size(); i2++) {
            assert(table[(it->second)[i2]] == it->first);
        }
    }
    #endif

    if (solver->conf.verbosity  >= 2
        || (solver->conf.verbosity  >=1 && used_part_no > 1)
    ) {
        cout
        << "c Found parts: " <<  reverseTable.size()
        << " time: "
        << std::setprecision(2) << std::fixed << cpuTime() - time
        << " s"
        << endl;

        size_t notPrinted = 0;
        size_t totalSmallSize = 0;
        size_t i = 0;
        for(map<uint32_t, vector<Var> >::const_iterator
            it = reverseTable.begin(), end = reverseTable.end()
            ; it != end
            ; it++, i++
        ) {
            if (it->second.size() < 300 || solver->conf.verbosity >= 3) {
                totalSmallSize += it->second.size();
                notPrinted++;
            } else {
                cout
                << "c large part " << std::setw(5) << i
                << " size: " << std::setw(10) << it->second.size()
                << endl;
            }
        }

        if (solver->conf.verbosity < 3) {
            cout
            << "c Not printed total small (<300 vars) parts:" << notPrinted
            << " vars: " << totalSmallSize
            << endl;
        }
    }

    return true;
}

void PartFinder::addToPartClauses(const vector<ClOffset>& cs)
{
    for (vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = solver->clAllocator->getPointer(*it);
        addToPartClause(*cl);
    }
}

void PartFinder::addToPartImplicits()
{
    vector<Lit> lits;

    size_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        //If empty, skip
        if (it->empty())
            continue;

        Lit lit = Lit::toLit(wsLit);
        for(vec<Watched>::const_iterator
            it2 = it->begin(), end2 = it->end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBinary()
                //Only non-learnt
                && !it2->learnt()
                //Only do each binary once
                && lit < it2->lit1()
            ) {
                lits.clear();
                lits.push_back(lit);
                lits.push_back(it2->lit1());
                addToPartClause(lits);
            }

            if (it2->isTri()
                //Non-learnt
                && !it2->learnt()
                //Only do each tri once
                && lit < it2->lit2()
                && it2->lit1() < it2->lit2()
            ) {
                lits.clear();
                lits.push_back(lit);
                lits.push_back(it2->lit1());
                lits.push_back(it2->lit2());
                addToPartClause(lits);
            }
        }
    }
}

template<class T>
void PartFinder::addToPartClause(const T& cl)
{
    set<uint32_t> tomerge;
    vector<Var> newSet;

    //Where should each literal go?
    for (typename T::const_iterator
        it = cl.begin(), end = cl.end()
        ; it != end
        ; it++
    ) {
        if (table[it->var()] != std::numeric_limits<uint32_t>::max()) {
            tomerge.insert(table[it->var()]);
        } else {
            newSet.push_back(it->var());
        }
    }

    //no trees to merge, only merge the clause into one tree
    if (tomerge.size() == 1) {

        const uint32_t into = *tomerge.begin();
        map<uint32_t, vector<Var> >::iterator intoReverse
            = reverseTable.find(into);

        //Put the new lits into this set
        for (vector<Var>::const_iterator
            it = newSet.begin(), end = newSet.end()
            ; it != end
            ; it++
        ) {
            intoReverse->second.push_back(*it);
            table[*it] = into;
        }
        return;
    }

    //Delete tables to merge and put their elements into newSet
    for (set<uint32_t>::iterator
        it = tomerge.begin(), end = tomerge.end()
        ; it != end
        ; it++
    ) {
        //Add them all
        newSet.insert(
            newSet.end()
            , reverseTable[*it].begin()
            , reverseTable[*it].end()
        );

        //Delete this part
        reverseTable.erase(*it);
        used_part_no--;
    }

    //Mark all these lits as belonging to part_no
    for (size_t i = 0; i < newSet.size(); i++) {
        table[newSet[i]] = part_no;
    }

    reverseTable[part_no] = newSet;
    part_no++;
    used_part_no++;
}

/*
const uint32_t PartFinder::setParts()
{
    vector<uint32_t> numClauseInPart(part_no, 0);
    vector<uint32_t> sumLitsInPart(part_no, 0);

    calcIn(solver->longIrredCls, numClauseInPart, sumLitsInPart);
    calcInImplicits(numClauseInPart, sumLitsInPart);

    uint32_t parts = 0;
    for (uint32_t i = 0; i < numClauseInPart.size(); i++) {

        //Nothing in here
        if (sumLitsInPart[i] == 0)
            continue;

        if (solver->conf.verbosity  >= 3
            || ( solver->conf.verbosity  >= 1 && reverseTable.size() > 1)
        ) {
            cout
            << "c Found part " << std::setw(8) << i
            << " vars: " << std::setw(10) << reverseTable[i].size()
            << " clauses:" << std::setw(10) << numClauseInPart[i]
            << " lits size:" << std::setw(10) << sumLitsInPart[i]
            << endl;
        }
        parts++;
    }

    if (parts > 1) {
        #ifdef VERBOSE_DEBUG
        for (map<uint32_t, vector<Var> >::iterator
            it = reverseTable.begin(), end = reverseTable.end()
            ; it != end
            ; it++
        ) {
            cout << "-- set begin --" << endl;
            for (vector<Var>::iterator
                it2 = it->second.begin(), end2 = it->second.end()
                ; it2 != end2
                ; it2++
            ) {
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
        for (const Watched *it2 = ws.getData(), *end2 = ws.getDataEnd(); it2 != end2; it2++) {
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
*/

