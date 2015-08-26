/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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
#include "sqlstats.h"

using namespace CMSat;

using std::set;
using std::map;
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG



//#define PART_FINDING

CompFinder::CompFinder(Solver* _solver) :
    timedout(false)
    , seen(_solver->seen)
    , solver(_solver)
{
}

void CompFinder::time_out_print(const double myTime) const
{
    if (solver->conf.verbosity >= 2) {
        cout
        << "c [comp] Timed out finding components "
        << "BP: "
        << std::setprecision(2) << std::fixed
        << (double)(orig_bogoprops-bogoprops_remain)/(1000.0*1000.0)
        << "M"
        << solver->conf.print_times(cpuTime() - myTime)
        << endl;
    }
}

void CompFinder::print_found_components() const
{
    size_t notPrinted = 0;
    size_t totalSmallSize = 0;
    size_t i = 0;
    size_t print_limit = 300;
    for(map<uint32_t, vector<Var> >::const_iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; ++it, i++
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

bool CompFinder::reverse_table_is_correct() const
{
    for (map<uint32_t, vector<Var> >::const_iterator
        it = reverseTable.begin()
        ; it != reverseTable.end()
        ; ++it
    ) {
        for (size_t i2 = 0; i2 < it->second.size(); i2++) {
            assert(table[(it->second)[i2]] == it->first);
        }
    }

    return true;
}

bool CompFinder::find_components()
{
    const double myTime = cpuTime();

    table.clear();
    table.resize(solver->nVars(), std::numeric_limits<uint32_t>::max());
    reverseTable.clear();
    comp_no = 0;
    used_comp_no = 0;

    solver->clauseCleaner->remove_and_clean_all();

    if (solver->conf.doFindAndReplaceEqLits
        && !solver->varReplacer->replace_if_enough_is_found()
    ) {
        return false;
    }

    //Add the clauses to the sets
    bogoprops_remain =
        solver->conf.comp_find_time_limitM*1000ULL*1000ULL
        *solver->conf.global_timeout_multiplier;
    orig_bogoprops = bogoprops_remain;
    timedout = false;
    add_clauses_to_component(solver->longIrredCls);
    addToCompImplicits();
    print_and_add_to_sql_result(myTime);

    return solver->okay();
}

void CompFinder::print_and_add_to_sql_result(const double myTime) const
{
    const double time_used = cpuTime() - myTime;
    const double time_remain = calc_percentage(bogoprops_remain, orig_bogoprops);

    if (timedout) {
        time_out_print(myTime);
    } else {
        assert(reverse_table_is_correct());

        if (solver->conf.verbosity >= 2
            || (solver->conf.verbosity >=1 && used_comp_no > 1)
        ) {
            cout
            << "c [comp] Found component(s): " <<  reverseTable.size()
            << " BP: "
            << std::setprecision(2) << std::fixed
            << (double)(orig_bogoprops-bogoprops_remain)/(1000.0*1000.0)<< "M"
            << " T-r: " << time_remain*100.0 << "%"
            << solver->conf.print_times(time_used)
            << endl;

            if (reverseTable.size() != 1) {
                print_found_components();
            }
        }
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "compfinder"
            , time_used
            , timedout
            , time_remain
        );
     }
}

void CompFinder::add_clauses_to_component(const vector<ClOffset>& cs)
{
    for (ClOffset offset: cs) {
        if (bogoprops_remain <= 0) {
            timedout = true;
            break;
        }
        bogoprops_remain -= 10;
        Clause* cl = solver->cl_alloc.ptr(offset);
        add_clause_to_component(*cl);
    }
}

void CompFinder::addToCompImplicits()
{
    vector<Lit> lits;
    vector<uint16_t>& seen = solver->seen;

    for (size_t var = 0; var < solver->nVars(); var++) {
        if (bogoprops_remain <= 0) {
            timedout = true;
            break;
        }

        bogoprops_remain -= 2;
        Lit lit(var, false);
        lits.clear();
        lits.push_back(lit);
        for(int sign = 0; sign < 2; sign++) {
            lit = Lit(var, sign);
            watch_subarray ws = solver->watches[lit.toInt()];

            //If empty, skip
            if (ws.empty())
                continue;

            bogoprops_remain -= ws.size() + 10;
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
                ; ++it
            ) {
                seen[it->var()] = 0;
            }

            add_clause_to_component(lits);
        }

    }
}

template<class T>
bool CompFinder::belong_to_same_component(const T& cl)
{
    if (table[cl[0].var()] != std::numeric_limits<uint32_t>::max()) {
        bogoprops_remain -= cl.size()/2 + 1;
        const uint32_t comp = table[cl[0].var()];

        for (const Lit l: cl) {
            if (table[l.var()] != comp) {
                return false;
            }
        }

        return true;
    }

    return false;
}

template<class T>
void CompFinder::fill_newset_and_tomerge(const T& cl)
{
    bogoprops_remain -= cl.size()*2;

    for (const Lit lit: cl) {
        if (table[lit.var()] != std::numeric_limits<uint32_t>::max()
        ) {
            if (!seen[table[lit.var()]]) {
                tomerge.push_back(table[lit.var()]);
                seen[table[lit.var()]] = 1;
            }
        } else {
            newSet.push_back(lit.var());
        }
    }
}

void CompFinder::merge_newset_into_single_component()
{
    const uint32_t into = tomerge[0];
    seen[into] = 0;
    map<uint32_t, vector<Var> >::iterator intoReverse
        = reverseTable.find(into);

    //Put the new lits into this set
    for (const Var v: newSet) {
        intoReverse->second.push_back(v);
        table[v] = into;
    }
}

template<class T>
void CompFinder::add_clause_to_component(const T& cl)
{
    assert(cl.size() > 1);
    tomerge.clear();
    newSet.clear();

    if (belong_to_same_component(cl)) {
        return;
    }

    fill_newset_and_tomerge(cl);

    //no sets to merge, only merge the clause into one tree
    if (tomerge.size() == 1) {
        merge_newset_into_single_component();
        return;
    }

    //Expensive merging coming up
    bogoprops_remain -= 20;

    //Delete tables to merge and put their elements into newSet
    for (const uint32_t merge: tomerge) {
        //Clear seen
        seen[merge] = 0;

        //Find in reverseTable
        bogoprops_remain -= reverseTable.size()*2;
        map<uint32_t, vector<Var> >::iterator it2 = reverseTable.find(merge);
        assert(it2 != reverseTable.end());

        //Add them all
        bogoprops_remain -= it2->second.size();
        newSet.insert(
            newSet.end()
            , it2->second.begin()
            , it2->second.end()
        );

        //Delete this comp
        bogoprops_remain -= reverseTable.size();
        reverseTable.erase(it2);
        used_comp_no--;
    }

    //No literals lie outside of already seen components
    if (newSet.empty())
        return;

    //Mark all lits not belonging to seen components as belonging to comp_no
    bogoprops_remain -= newSet.size();
    for (const Var v: newSet) {
        table[v] = comp_no;
    }

    reverseTable[comp_no] = newSet;
    comp_no++;
    used_comp_no++;
}
