/**************************************************************************
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
*****************************************************************************/

#include "ClauseVivifier.h"
#include "ClauseCleaner.h"
#include "time_mem.h"

//#define ASSYM_DEBUG

ClauseVivifier::ClauseVivifier(Solver& _solver) :
    lastTimeWentUntil(0)
    , solver(_solver)
{}


/**
@brief Performs clause vivification (by Hamadi et al.)

This is the only thing that does not fit under the aegis of tryBoth(), since
it is not part of failed literal probing, really. However, it is here because
it seems to be a function that fits into the idology of failed literal probing.
Maybe I am off-course and it should be in another class, or a class of its own.
*/
const bool ClauseVivifier::vivifyClauses()
{
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    bool failed;

    uint32_t effective = 0;
    uint32_t effectiveLit = 0;
    double myTime = cpuTime();
    uint64_t maxNumProps = 30000*1000;
    uint64_t extraDiff = 0;
    uint64_t oldProps = solver.propagations;
    bool needToFinish = false;
    uint32_t checkedClauses = 0;
    uint32_t potentialClauses = solver.clauses.size();
    if (lastTimeWentUntil + 500 > solver.clauses.size())
        lastTimeWentUntil = 0;
    uint32_t thisTimeWentUntil = 0;
    vec<Lit> lits;
    vec<Lit> unused;

    if (solver.clauses.size() < 1000000) {
        //if too many clauses, random order will do perfectly well
        std::sort(solver.clauses.getData(), solver.clauses.getDataEnd(), sortBySize());
    }

    Clause **i, **j;
    i = j = solver.clauses.getData();
    for (Clause **end = solver.clauses.getDataEnd(); i != end; i++) {
        if (needToFinish || lastTimeWentUntil > 0) {
            if (!needToFinish) {
                lastTimeWentUntil--;
                thisTimeWentUntil++;
            }
            *j++ = *i;
            continue;
        }

        //if done enough, stop doing it
        if (solver.propagations-oldProps + extraDiff > maxNumProps) {
            //std::cout << "Need to finish -- ran out of prop" << std::endl;
            needToFinish = true;
        }

        //if bad performance, stop doing it
        /*if ((i-solver.clauses.getData() > 5000 && effectiveLit < 300)) {
            std::cout << "Need to finish -- not effective" << std::endl;
            needToFinish = true;
        }*/

        Clause& c = **i;
        extraDiff += c.size();
        checkedClauses++;
        thisTimeWentUntil++;

        assert(c.size() > 2);
        assert(!c.learnt());

        unused.clear();
        lits.clear();
        lits.growTo(c.size());
        memcpy(lits.getData(), c.getData(), c.size() * sizeof(Lit));

        failed = false;
        uint32_t done = 0;
        solver.newDecisionLevel();
        for (; done < lits.size();) {
            uint32_t i2 = 0;
            for (; i2 < 2 && done+i2 < lits.size(); i2++) {
                lbool val = solver.value(lits[done+i2]);
                if (val == l_Undef) {
                    solver.uncheckedEnqueueLight(~lits[done+i2]);
                } else if (val == l_False) {
                    unused.push(lits[done+i2]);
                }
            }
            done += i2;
            failed = (!solver.propagate(false).isNULL());
            if (failed) {
                break;
            }
        }
        solver.cancelUntil(0);
        assert(solver.ok);

        if (unused.size() > 0 || (failed && done < lits.size())) {
            effective++;
            uint32_t origSize = lits.size();
            #ifdef ASSYM_DEBUG
            std::cout << "Assym branch effective." << std::endl;
            std::cout << "-- Orig clause:"; c.plainPrint();
            #endif
            solver.detachClause(c);

            lits.shrink(lits.size() - done);
            for (uint32_t i2 = 0; i2 < unused.size(); i2++) {
                remove(lits, unused[i2]);
            }

            Clause *c2 = solver.addClauseInt(lits, c.getGroup());
            #ifdef ASSYM_DEBUG
            std::cout << "-- Origsize:" << origSize << " newSize:" << (c2 == NULL ? 0 : c2->size()) << " toRemove:" << c.size() - done << " unused.size():" << unused.size() << std::endl;
            #endif
            extraDiff += 20;
            //TODO cheating here: we don't detect a NULL return that is in fact a 2-long clause
            effectiveLit += origSize - (c2 == NULL ? 0 : c2->size());
            solver.clauseAllocator.clauseFree(&c);

            if (c2 != NULL) {
                #ifdef ASSYM_DEBUG
                std::cout << "-- New clause:"; c2->plainPrint();
                #endif
                *j++ = c2;
            }

            if (!solver.ok) needToFinish = true;
        } else {
            *j++ = *i;
        }
    }
    solver.clauses.shrink(i-j);

    lastTimeWentUntil = thisTimeWentUntil;

    if (solver.conf.verbosity  >= 1) {
        std::cout << "c asymm "
        << " cl-useful: " << effective << "/" << checkedClauses << "/" << potentialClauses
        << " lits-rem:" << effectiveLit
        << " time: " << cpuTime() - myTime
        << std::endl;
    }

    return solver.ok;
}
