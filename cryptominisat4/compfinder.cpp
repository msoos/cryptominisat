/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
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
#include "compfinder.h"
#include "time_mem.h"
#include "cloffset.h"
#include "solver.h"
#include "varreplacer.h"
#include "clausecleaner.h"
#include "clauseallocator.h"

using namespace CMSat;

using std::set;
using std::map;
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG



//#define PART_FINDING

CompFinder::CompFinder(Solver* _solver) :
    timeUsed(0)
    , timedout(false)
    , solver(_solver)
{
}

bool CompFinder::findComps()
{
    const double myTime = cpuTime();

    table.clear();
    table.resize(solver->nVars(), std::numeric_limits<uint32_t>::max());
    reverseTable.clear();
    comp_no = 0;
    used_comp_no = 0;

    solver->clauseCleaner->removeAndCleanAll();

    if (solver->conf.doFindAndReplaceEqLits
        && !solver->varReplacer->performReplace()
    ) {
        return false;
    }

    //Add the clauses to the sets
    timeUsed = 0;
    timedout = false;
    addToCompClauses(solver->longIrredCls);
    addToCompImplicits();

    //We timed-out while searching, internal datas are wrong!
    if (timedout) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [comp] Timed out finding components, BP: "
            << std::setprecision(2) << std::fixed
            << (double)timeUsed/(1000.0*1000.0)
            << "M T: "
            << std::setprecision(2) << std::fixed
            << cpuTime() - myTime
            << endl;
        }

        return solver->okay();
    }

    //const uint32_t comps = setComps();

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
        || (solver->conf.verbosity  >=1 && used_comp_no > 1)
    ) {
        cout
        << "c [comp] Found component(s): " <<  reverseTable.size()
        << " BP: "
        << std::setprecision(2) << std::fixed
        << (double)timeUsed/(1000.0*1000.0)<< "M"
        << " time: "
        << std::setprecision(2) << std::fixed << cpuTime() - myTime
        << " s"
        << endl;

        if (reverseTable.size() == 1) {
            return solver->okay();
        }

        size_t notPrinted = 0;
        size_t totalSmallSize = 0;
        size_t i = 0;
        size_t print_limit = 300;
        for(map<uint32_t, vector<Var> >::const_iterator
            it = reverseTable.begin(), end = reverseTable.end()
            ; it != end
            ; it++, i++
        ) {
            if (it->second.size() < print_limit || solver->conf.verbosity >= 3) {
                totalSmallSize += it->second.size();
                notPrinted++;
            } else {
                cout
                << "c [comp] large component " << std::setw(5) << i
                << " size: " << std::setw(10) << it->second.size()
                << endl;
            }
        }

        if (solver->conf.verbosity < 3 && notPrinted > 0) {
            cout
            << "c [comp] Unprinted small (<" << print_limit << " var) components:" << notPrinted
            << " vars: " << totalSmallSize
            << endl;
        }
    }

    return true;
}

void CompFinder::addToCompClauses(const vector<ClOffset>& cs)
{
    for (ClOffset offset: cs) {
        if (timeUsed/(1000ULL*1000ULL) > solver->conf.compFindLimitMega) {
            timedout = true;
            break;
        }
        timeUsed += 10;
        Clause* cl = solver->clAllocator.getPointer(offset);
        addToCompClause(*cl);
    }
}

void CompFinder::addToCompImplicits()
{
    vector<Lit> lits;
    vector<uint16_t>& seen = solver->seen;

    for (size_t var = 0; var < solver->nVars(); var++) {
        if (timeUsed/(1000ULL*1000ULL) > solver->conf.compFindLimitMega) {
            timedout = true;
            break;
        }

        timeUsed += 2;
        Lit lit(var, false);
        lits.clear();
        lits.push_back(lit);
        for(int sign = 0; sign < 2; sign++) {
            lit = Lit(var, sign);
            watch_subarray ws = solver->watches[lit.toInt()];

            //If empty, skip
            if (ws.empty())
                continue;

            timeUsed += ws.size() + 10;
            for(watch_subarray::const_iterator
                it2 = ws.begin(), end2 = ws.end()
                ; it2 != end2
                ; it2++
            ) {
                if (it2->isBinary()
                    //Only irred
                    && !it2->red()
                    //Only do each binary once
                    && lit < it2->lit2()
                ) {
                    if (!seen[it2->lit2().var()]) {
                        lits.push_back(it2->lit2());
                        seen[it2->lit2().var()] = 1;
                    }
                }

                if (it2->isTri()
                    //irredundant
                    && !it2->red()
                    //Only do each tri once
                    && lit < it2->lit3()
                    && it2->lit2() < it2->lit3()
                ) {
                    if (!seen[it2->lit2().var()]) {
                        lits.push_back(it2->lit2());
                        seen[it2->lit2().var()] = 1;
                    }

                    if (!seen[it2->lit3().var()]) {
                        lits.push_back(it2->lit3());
                        seen[it2->lit3().var()] = 1;
                    }
                }
            }
        }

        if (lits.size() > 1) {
            //Clear seen
            for(vector<Lit>::const_iterator
                it = lits.begin(), end = lits.end()
                ; it != end
                ; it++
            ) {
                seen[it->var()] = 0;
            }

            addToCompClause(lits);
        }

    }
}

template<class T>
void CompFinder::addToCompClause(const T& cl)
{
    assert(cl.size() > 1);
    tomerge.clear();
    newSet.clear();
    vector<uint16_t>& seen = solver->seen;
    timeUsed += cl.size()/2 + 1;

    //Do they all belong to the same place?
    bool allsame = false;
    if (table[cl[0].var()] != std::numeric_limits<uint32_t>::max()) {
        uint32_t comp = table[cl[0].var()];
        allsame = true;
        for (typename T::const_iterator
            it = cl.begin(), end = cl.end()
            ; it != end
            ; it++
        ) {
            if (table[it->var()] != comp) {
                allsame = false;
                break;
            }
        }
    }

    //They already all belong to the same comp, skip
    if (allsame) {
        return;
    }

    //Where should each literal go?
    timeUsed += cl.size()*2;
    for (typename T::const_iterator
        it = cl.begin(), end = cl.end()
        ; it != end
        ; it++
    ) {
        if (table[it->var()] != std::numeric_limits<uint32_t>::max()
        ) {
            if (!seen[table[it->var()]]) {
                tomerge.push_back(table[it->var()]);
                seen[table[it->var()]] = 1;
            }
        } else {
            newSet.push_back(it->var());
        }
    }

    //no trees to merge, only merge the clause into one tree
    if (tomerge.size() == 1) {
        const uint32_t into = tomerge[0];
        seen[into] = 0;
        map<uint32_t, vector<Var> >::iterator intoReverse
            = reverseTable.find(into);

        //Put the new lits into this set
        for (Var v: newSet) {
            intoReverse->second.push_back(v);
            table[v] = into;
        }
        return;
    }

    //Expensive merging coming up
    timeUsed += 20;

    //Delete tables to merge and put their elements into newSet
    for (const uint32_t merge: tomerge) {
        //Clear seen
        seen[merge] = 0;

        //Find in reverseTable
        timeUsed += reverseTable.size()*2;
        map<uint32_t, vector<Var> >::iterator it2 = reverseTable.find(merge);
        assert(it2 != reverseTable.end());

        //Add them all
        timeUsed += it2->second.size();
        newSet.insert(
            newSet.end()
            , it2->second.begin()
            , it2->second.end()
        );

        //Delete this comp
        timeUsed += reverseTable.size();
        reverseTable.erase(it2);
        used_comp_no--;
    }

    //No literals lie outside of already seen components
    if (newSet.empty())
        return;

    //Mark all lits not belonging to seen components as belonging to comp_no
    timeUsed += newSet.size();
    for (const Var v: newSet) {
        table[v] = comp_no;
    }

    reverseTable[comp_no] = newSet;
    comp_no++;
    used_comp_no++;
}
