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

#include "xorfinder.h"
#include "time_mem.h"
#include "solver.h"
#include "occsimplifier.h"
#include "clauseallocator.h"
#include "sqlstats.h"

#include <limits>

using namespace CMSat;
using std::cout;
using std::endl;

XorFinder::XorFinder(OccSimplifier* _occsimplifier, Solver* _solver) :
    occsimplifier(_occsimplifier)
    , solver(_solver)
    , seen(_solver->seen)
    , seen2(_solver->seen2)
{
}

void XorFinder::find_xors_based_on_long_clauses()
{
    vector<Lit> lits;
    for (vector<ClOffset>::iterator
        it = occsimplifier->clauses.begin()
        , end = occsimplifier->clauses.end()
        ; it != end && xor_find_time_limit > 0
        ; ++it
    ) {
        ClOffset offset = *it;
        Clause* cl = solver->cl_alloc.ptr(offset);
        xor_find_time_limit -= 3;

        //Already freed
        if (cl->freed() || cl->getRemoved()) {
            continue;
        }

        //Too large -> too expensive
        if (cl->size() > solver->conf.maxXorToFind) {
            continue;
        }

        //If not tried already, find an XOR with it
        if (!cl->stats.marked_clause) {
            cl->stats.marked_clause = true;

            lits.resize(cl->size());
            std::copy(cl->begin(), cl->end(), lits.begin());
            findXor(lits, cl->abst);
        }
    }
}

void XorFinder::find_xors_based_on_short_clauses()
{
    assert(solver->ok);

    vector<Lit> lits;
    for (size_t wsLit = 0, end = 2*solver->nVars(); wsLit < end; wsLit++) {
        const Lit lit = Lit::toLit(wsLit);
        assert(solver->watches.size() > wsLit);
        watch_subarray_const ws = solver->watches[lit];

        xor_find_time_limit -= (int64_t)ws.size()*3;

        //cannot use iterators because findXor may update the watchlist
        for (size_t i = 0, size = solver->watches[lit].size()
            ; i < size
            ; i++
        ) {
            const Watched& w = ws[i];

            //Only care about tertiaries
            if (!w.isTri())
                continue;

            //Only bother about each tri-clause once
            if (lit > w.lit2()
                || w.lit2() > w.lit3()
            ) {
                continue;
            }

            lits.resize(3);
            lits[0] = lit;
            lits[1] = w.lit2();
            lits[2] = w.lit3();

            findXor(lits, calcAbstraction(lits));
        }
    }
}

void XorFinder::find_xors()
{
    runStats.clear();
    runStats.numCalls = 1;

    xors.clear();
    assert(solver->watches.get_smudged_list().empty());
    double myTime = cpuTime();
    const int64_t orig_xor_find_time_limit =
        1000LL*1000LL*solver->conf.xor_finder_time_limitM
        *solver->conf.global_timeout_multiplier;

    xor_find_time_limit = orig_xor_find_time_limit;

    #ifdef DEBUG_MARKED_CLAUSE
    assert(solver->no_marked_clauses());
    #endif

    find_xors_based_on_long_clauses();
    find_xors_based_on_short_clauses();

    //Cleanup
    for(ClOffset offset: occsimplifier->clauses) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        cl->stats.marked_clause = false;
    }
    solver->clean_occur_from_idx_types_only_smudged();

    //Print stats
    const bool time_out = (xor_find_time_limit < 0);
    const double time_remain = float_div(xor_find_time_limit, orig_xor_find_time_limit);
    runStats.findTime = cpuTime() - myTime;
    runStats.time_outs += time_out;
    assert(runStats.foundXors == xors.size());
    solver->sumStats.num_xors_found_last = xors.size();
    print_found_xors();

    if (solver->conf.verbosity >= 1) {
        runStats.print_short(solver);
    }
    globalStats += runStats;

    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "xorfind"
            , cpuTime() - myTime
            , time_out
            , time_remain
        );
    }
}

void XorFinder::print_found_xors()
{
    if (solver->conf.verbosity >= 5) {
        cout << "c Found XORs: " << endl;
        for(vector<Xor>::const_iterator
            it = xors.begin(), end = xors.end()
            ; it != end
            ; ++it
        ) {
            cout << "c " << *it << endl;
        }
    }
}

void XorFinder::findXor(vector<Lit>& lits, cl_abst_type abst)
{
    //Set this clause as the base for the FoundXors
    //fill 'seen' with variables
    FoundXors foundCls(lits, abst, seen);

    //cout << __PRETTY_FUNCTION__ << " called with " << lits << endl;

    //Try to match on all literals
    for (const Lit lit: lits) {
        findXorMatch(solver->watches[lit], lit, foundCls);
        findXorMatch(solver->watches[~lit], ~lit, foundCls);

        //More expensive
        //findXorMatchExt(solver->watches[lit], lit, foundCls);
        //findXorMatchExt(solver->watches[~lit], ~lit, foundCls);

        //TODO stamping
        /*if (solver->conf.useCacheWhenFindingXors) {
            findXorMatch(solver->implCache[(*l).toInt()].lits, *l, foundCls);
            findXorMatch(solver->implCache[(~*l).toInt()].lits, ~(*l), foundCls);
        }*/

        xor_find_time_limit -= 5;
        if (foundCls.foundAll())
            break;
    }

    xor_find_time_limit -= 5;
    if (foundCls.foundAll()) {
        std::sort(lits.begin(), lits.end());
        Xor found_xor(lits, foundCls.getRHS());

        //Have we found this XOR clause already?
        bool already_inside = false;
        for (const Watched ws: solver->watches[lits[0].unsign()]) {
            if (ws.isIdx()
                && xors[ws.get_idx()] == found_xor
            ) {
                already_inside = true;
                break;
            }
        }

        //If XOR clause is new, add it
        if (!already_inside) {
            add_found_xor(found_xor);
        }
    }

    //Clear 'seen'
    for (const Lit tmp_lit: lits) {
        seen[tmp_lit.var()] = 0;
    }
}

void XorFinder::add_found_xor(const Xor& found_xor)
{
    xor_find_time_limit -= 20;
    xors.push_back(found_xor);
    runStats.foundXors++;
    runStats.sumSizeXors += found_xor.vars.size();
    uint32_t thisXorIndex = xors.size()-1;
    Lit attach_point = Lit(found_xor.vars[0], false);
    solver->watches[attach_point].push(Watched(thisXorIndex));
    solver->watches.smudge(attach_point);
}

//TODO stamping
/*void XorFinder::findXorMatch(
    const vector<LitExtra>& lits
    , const Lit lit
    , FoundXors& foundCls
) const {

    for (vector<LitExtra>::const_iterator
        it = lits.begin(), end = lits.end()
        ; it != end
        ; ++it
    )  {
        if (seen[it->getLit().var()]) {
            foundCls.add(lit, it->getLit());
        }
    }
}*/

void XorFinder::findXorMatchExt(
    watch_subarray_const occ
    , Lit lit
    , FoundXors& foundCls
) {
    //seen2 is clear

    for (watch_subarray::const_iterator
        it = occ.begin(), end = occ.end()
        ; it != end
        ; ++it
    ) {
        //Deal with binary
        if (it->isBin()) {
            if (seen[it->lit2().var()]) {
                tmpClause.clear();
                tmpClause.push_back(lit);
                tmpClause.push_back(it->lit2());
                if (tmpClause[0] > tmpClause[1])
                    std::swap(tmpClause[0], tmpClause[1]);

                foundCls.add(tmpClause, varsMissing);
            }

            continue;
        }

        assert(it->isClause() && "This algo has not been updated to deal with TRI, sorry");

        //Deal with clause
        const ClOffset offset = it->get_offset();
        Clause& cl = *solver->cl_alloc.ptr(offset);
        if (cl.freed() || cl.getRemoved())
            continue;

        //Must not be larger than the original clauses
        if (cl.size() > foundCls.getSize())
            continue;

        tmpClause.clear();
        //cout << "Orig clause: " << foundCls.getOrigCl() << endl;

        bool rhs = true;
        uint32_t i = 0;
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++, i++) {

            //If this literal is not meant to be inside the XOR
            //then try to find a replacement for it from the cache
            if (!seen[l->var()]) {
                bool found = false;
                //TODO stamping
                const vector<LitExtra>& cache = solver->implCache[~lit].lits;
                for(vector<LitExtra>::const_iterator it2 = cache.begin(), end2 = cache.end()
                    ; it2 != end2 && !found
                    ; it2++
                ) {
                    if (seen[l->var()] && !seen2[l->var()]) {
                        found = true;
                        seen2[l->var()] = true;
                        rhs ^= l->sign();
                        tmpClause.push_back(it2->getLit());
                        //cout << "Added trans lit: " << tmpClause.back() << endl;
                    }
                }

                //Didn't find replacement
                if (!found)
                    goto end;
            }
            else
            //Fine, it's inside the orig clause, but we might have already added this lit
            {
                if (!seen2[l->var()]) {
                    seen2[l->var()] = true;
                    rhs ^= l->sign();
                    tmpClause.push_back(*l);
                    //cout << "Added lit: " << tmpClause.back() << endl;
                } else {
                    goto end; //HACK: we don't want both 'lit' and '~lit' end up in the clause
                }
            }
        }

        //either the invertedness has to match, or the size must be smaller
        if (rhs != foundCls.getRHS() && cl.size() == foundCls.getSize())
            goto end;

        //If the size of this clause is the same of the base clause, then
        //there is no point in using this clause as a base for another XOR
        //because exactly the same things will be found.
        if (cl.size() == foundCls.getSize()) {
            cl.stats.marked_clause = true;
        }

        std::sort(tmpClause.begin(), tmpClause.end());
        foundCls.add(tmpClause, varsMissing);

        end:;
        //cout << "Not OK" << endl;

        //Clear 'seen2'
        for(const Lit tmp_lit: tmpClause) {
            seen2[tmp_lit.var()] = false;
        }
    }
}

void XorFinder::findXorMatch(
    watch_subarray_const occ
    , const Lit lit
    , FoundXors& foundCls
) {
    xor_find_time_limit -= (int64_t)occ.size();
    for (watch_subarray::const_iterator
        it = occ.begin(), end = occ.end()
        ; it != end
        ; ++it
    ) {
        if (it->isIdx()) {
            continue;
        }

        //Deal with binary
        if (it->isBin()) {
            if (//Only once per binary
                lit < it->lit2()
                //only for correct binary
                && seen[it->lit2().var()]
            ) {
                tmpClause.clear();
                tmpClause.push_back(lit);
                tmpClause.push_back(it->lit2());

                foundCls.add(tmpClause, varsMissing);
                xor_find_time_limit-=5;
                if (foundCls.foundAll())
                    break;
            }

            continue;
        }

        //Deal with tertiary
        if (it->isTri()) {
            if (//Only once per tri
                lit < it->lit2() && it->lit2() < it->lit3()

                //Only for correct tri
                && seen[it->lit2().var()] && seen[it->lit3().var()]
            ) {
                bool rhs = true;
                rhs ^= lit.sign();
                rhs ^= it->lit2().sign();
                rhs ^= it->lit3().sign();

                if (rhs == foundCls.getRHS() || foundCls.getSize() > 3) {
                    tmpClause.clear();
                    tmpClause.push_back(lit);
                    tmpClause.push_back(it->lit2());
                    tmpClause.push_back(it->lit3());

                    foundCls.add(tmpClause, varsMissing);
                    xor_find_time_limit-=5;
                    if (foundCls.foundAll())
                        break;
                }

            }
            continue;
        }

        //Deal with clause
        const ClOffset offset = it->get_offset();
        Clause& cl = *solver->cl_alloc.ptr(offset);
        if (cl.freed() || cl.getRemoved())
            continue;

        //Must not be larger than the original clause
        if (cl.size() > foundCls.getSize())
            continue;

        //Doesn't contain variables not in the original clause
        if ((cl.abst | foundCls.getAbst()) != foundCls.getAbst())
            continue;

        //Check RHS, vars inside
        bool rhs = true;
        for (const Lit cl_lit :cl) {
            //early-abort, contains literals not in original clause
            if (!seen[cl_lit.var()])
                goto end;

            rhs ^= cl_lit.sign();
        }
        //either the invertedness has to match, or the size must be smaller
        if (rhs != foundCls.getRHS() && cl.size() == foundCls.getSize())
            continue;

        //If the size of this clause is the same of the base clause, then
        //there is no point in using this clause as a base for another XOR
        //because exactly the same things will be found.
        if (cl.size() == foundCls.getSize()) {
            cl.stats.marked_clause = true;;
        }

        foundCls.add(cl, varsMissing);
        xor_find_time_limit-=5;
        if (foundCls.foundAll())
            break;

        end:;
    }
}

size_t XorFinder::mem_used() const
{
    size_t mem = 0;
    mem += xors.capacity()*sizeof(Xor);

    //Temporary
    mem += tmpClause.capacity()*sizeof(Lit);
    mem += varsMissing.capacity()*sizeof(uint32_t);

    return mem;
}

void XorFinder::Stats::print_short(const Solver* solver) const
{
    cout
    << "c [occ-xor] found " << std::setw(6) << foundXors
    << " avg sz " << std::setw(4) << std::fixed << std::setprecision(1)
    << float_div(sumSizeXors, foundXors)
    << solver->conf.print_times(findTime, time_outs)
    << endl;
}

void XorFinder::Stats::print() const
{
    cout << "c --------- XOR STATS ----------" << endl;
    print_stats_line("c num XOR found on avg"
        , float_div(foundXors, numCalls)
        , "avg size"
    );

    print_stats_line("c XOR avg size"
        , float_div(sumSizeXors, foundXors)
    );

    print_stats_line("c XOR finding time"
        , findTime
        , float_div(time_outs, numCalls)*100.0
        , "time-out"
    );
    cout << "c --------- XOR STATS END ----------" << endl;
}

XorFinder::Stats& XorFinder::Stats::operator+=(const XorFinder::Stats& other)
{
    //Time
    findTime += other.findTime;

    //XOR
    foundXors += other.foundXors;
    sumSizeXors += other.sumSizeXors;

    //Usefulness
    time_outs += other.time_outs;

    return *this;
}
