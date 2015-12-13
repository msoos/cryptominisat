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

#include "str_impl_w_impl_stamp.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "sqlstats.h"

using namespace CMSat;

bool StrImplWImplStamp::str_impl_w_impl_stamp()
{
    str_impl_data.clear();

    const size_t origTrailSize = solver->trail_size();
    timeAvailable =
        solver->conf.distill_implicit_with_implicit_time_limitM*1000LL*1000LL
        *solver->conf.global_timeout_multiplier;
    const int64_t orig_time = timeAvailable;
    double myTime = cpuTime();

    //Cannot handle empty
    if (solver->watches.size() == 0)
        return solver->okay();

    //Randomize starting point
    size_t upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (; numDone < solver->watches.size() && timeAvailable > 0
        ; upI = (upI +1) % solver->watches.size(), numDone++

    ) {
        str_impl_data.numWatchesLooked++;
        const Lit lit = Lit::toLit(upI);
        distill_implicit_with_implicit_lit(lit);
    }

    //Enqueue delayed values
    if (!solver->fully_enqueue_these(str_impl_data.toEnqueue))
        goto end;

    //Add delayed binary clauses
    for(const BinaryClause& bin: str_impl_data.binsToAdd) {
        lits.clear();
        lits.push_back(bin.getLit1());
        lits.push_back(bin.getLit2());
        timeAvailable -= 5;
        solver->add_clause_int(lits, bin.isRed());
        if (!solver->okay())
            goto end;
    }

end:

    if (solver->conf.verbosity >= 1) {
        str_impl_data.print(
            solver->trail_size() - origTrailSize
            , cpuTime() - myTime
            , timeAvailable
            , orig_time
            , solver
        );
    }
    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_stats();
    #endif

    return solver->okay();
}

void StrImplWImplStamp::distill_implicit_with_implicit_lit(const Lit lit)
{
    watch_subarray ws = solver->watches[lit];

    Watched* i = ws.begin();
    Watched* j = i;
    for (const Watched* end = ws.end()
        ; i != end
        ; i++
    ) {
        timeAvailable -= 2;
        if (timeAvailable < 0) {
            *j++ = *i;
            continue;
        }

        switch(i->getType()) {
            case CMSat::watch_clause_t:
                *j++ = *i;
                break;

            case CMSat::watch_binary_t:
                timeAvailable -= 20;
                strengthen_bin_with_bin(lit, i, j, end);
                break;

            case CMSat::watch_tertiary_t:
                timeAvailable -= 20;
                strengthen_tri_with_bin_tri_stamp(lit, i, j);
                break;

            default:
                assert(false);
                break;
        }
    }
    ws.shrink(i-j);
}

void StrImplWImplStamp::strengthen_tri_with_bin_tri_stamp(
    const Lit lit
    , Watched*& i
    , Watched*& j
) {
    const Lit lit1 = i->lit2();
    const Lit lit2 = i->lit3();
    bool rem = false;

    timeAvailable -= (long)solver->watches[~lit].size();
    for(watch_subarray::const_iterator
        it2 = solver->watches[~lit].begin(), end2 = solver->watches[~lit].end()
        ; it2 != end2 && timeAvailable > 0
        ; it2++
    ) {
        if (it2->isBin()
            && (it2->lit2() == lit1 || it2->lit2() == lit2)
        ) {
            rem = true;
            str_impl_data.remLitFromTriByBin++;
            break;
        }

        if (it2->isTri()
            && (
                (it2->lit2() == lit1 && it2->lit3() == lit2)
                ||
                (it2->lit2() == lit2 && it2->lit3() == lit1)
            )

        ) {
            rem = true;
            str_impl_data.remLitFromTriByTri++;
            break;
        }

        //watches are sorted, so early-abort
        if (it2->isClause())
            break;
    }

    if (rem) {
        solver->remove_tri_but_lit1(lit, i->lit2(), i->lit3(), i->red(), timeAvailable);
        str_impl_data.remLitFromTri++;
        str_impl_data.binsToAdd.push_back(BinaryClause(i->lit2(), i->lit3(), i->red()));

        (*solver->drat)
        << i->lit2()  << i->lit3() << fin
        << del << lit << i->lit2() << i->lit3() << fin;
        return;
    }

    if (solver->conf.doStamp) {
        //Strengthen TRI using stamps
        lits.clear();
        lits.push_back(lit);
        lits.push_back(i->lit2());
        lits.push_back(i->lit3());

        //Try both stamp types to reduce size
        timeAvailable -= 15;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
        str_impl_data.stampRem += tmp.first;
        str_impl_data.stampRem += tmp.second;
        if (lits.size() > 1) {
            timeAvailable -= 15;
            std::pair<size_t, size_t> tmp2 = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
            str_impl_data.stampRem += tmp2.first;
            str_impl_data.stampRem += tmp2.second;
        }

        if (lits.size() == 2) {
            solver->remove_tri_but_lit1(lit, i->lit2(), i->lit3(), i->red(), timeAvailable);
            str_impl_data.remLitFromTri++;
            str_impl_data.binsToAdd.push_back(BinaryClause(lits[0], lits[1], i->red()));

            //Drat
            (*solver->drat)
            << lits[0] << lits[1] << fin
            << del << lit << i->lit2() << i->lit3() << fin;

            return;
        } else if (lits.size() == 1) {
            solver->remove_tri_but_lit1(lit, i->lit2(), i->lit3(), i->red(), timeAvailable);
            str_impl_data.remLitFromTri+=2;
            str_impl_data.toEnqueue.push_back(lits[0]);
            (*solver->drat)
            << lits[0] << fin
            << del << lit << i->lit2() << i->lit3() << fin;

            return;
        }
    }


    //Nothing to do, copy
    *j++ = *i;
}

void StrImplWImplStamp::strengthen_bin_with_bin(
    const Lit lit
    , Watched*& i
    , Watched*& j
    , const Watched* end
) {
    lits.clear();
    lits.push_back(lit);
    lits.push_back(i->lit2());
    if (solver->conf.doStamp) {
        timeAvailable -= 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
        str_impl_data.stampRem += tmp.first;
        str_impl_data.stampRem += tmp.second;
        assert(!lits.empty());
        if (lits.size() == 1) {
            str_impl_data.toEnqueue.push_back(lits[0]);
            (*solver->drat) << lits[0] << fin;

            str_impl_data.remLitFromBin++;
            str_impl_data.stampRem++;
            *j++ = *i;
            return;
        }
    }

    //If inverted, then the inverse will never be found, because
    //watches are sorted
    if (i->lit2().sign()) {
        *j++ = *i;
        return;
    }

    //Try to look for a binary in this same watchlist
    //that has ~i->lit2() inside. Everything is sorted, so we are
    //lucky, this is speedy
    bool rem = false;
    watch_subarray::const_iterator i2 = i;
    while(i2 != end
        && (i2->isBin() || i2->isTri())
        && i->lit2().var() == i2->lit2().var()
    ) {
        timeAvailable -= 2;
        //Yay, we have found what we needed!
        if (i2->isBin() && i2->lit2() == ~i->lit2()) {
            rem = true;
            break;
        }

        i2++;
    }

    //Enqeue literal
    if (rem) {
        str_impl_data.remLitFromBin++;
        str_impl_data.toEnqueue.push_back(lit);
        (*solver->drat) << lit << fin;
    }
    *j++ = *i;
}

void StrImplWImplStamp::StrImplicitData::print(
    const size_t trail_diff
    , const double time_used
    , const int64_t timeAvailable
    , const int64_t orig_time
    , Solver* solver
) const {
    bool time_out = timeAvailable <= 0;
    const double time_remain = float_div(timeAvailable, orig_time);

    cout
    << "c [impl str]"
    << " lit bin: " << remLitFromBin
    << " lit tri: " << remLitFromTri
    << " (by tri: " << remLitFromTriByTri << ")"
    << " (by stamp: " << stampRem << ")"
    << " set-var: " << trail_diff
    << solver->conf.print_times(time_used, time_out, time_remain)
    << " w-visit: " << numWatchesLooked
    << endl;

    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "implicit str"
            , time_used
            , time_out
            , time_remain
        );
    }
}
