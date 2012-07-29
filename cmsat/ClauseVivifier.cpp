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

#include "ClauseVivifier.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "Solver.h"
#include <iomanip>
using std::cout;
using std::endl;

//#define ASSYM_DEBUG

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

ClauseVivifier::ClauseVivifier(Solver* _solver) :
    solver(_solver)
    , numCalls(0)
{}

bool ClauseVivifier::vivify(bool alsoStrengthen)
{
    assert(solver->ok);
    #ifdef VERBOSE_DEBUG
    cout << "c clauseVivifier started" << endl;
    #endif //VERBOSE_DEBUG
    numCalls++;

    solver->clauseCleaner->cleanClauses(solver->longIrredCls);

    if (!vivifyClausesCache(solver->longIrredCls, false, alsoStrengthen))
        goto end;

    if (!vivifyClausesCache(solver->longRedCls, true, alsoStrengthen))
        goto end;

    if (!vivifyClausesLongIrred())
        goto end;

    if (!vivifyClausesTriIrred())
        goto end;

end:
    //Stats
    globalStats += runStats;
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }

    return solver->ok;
}

bool ClauseVivifier::vivifyClausesTriIrred()
{
    uint64_t origShorten = runStats.numClShorten;
    double myTime = cpuTime();
    uint64_t maxNumProps = 10L*1000L*1000L;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    size_t origTrailSize = solver->trail.size();

    size_t upI;
    upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (; numDone < solver->watches.size()
        ; upI = (upI +1) % solver->watches.size(), numDone++

    ) {
        if (solver->propStats.bogoProps-oldBogoProps + extraTime > maxNumProps) {
            break;
        }

        Lit lit = Lit::toLit(upI);
        const vec<Watched>& ws = solver->watches[upI];
        for (size_t i = 0; i < ws.size(); i++) {
            if (solver->propStats.bogoProps-oldBogoProps + extraTime > maxNumProps) {
                break;
            }

            //Only TRI and each TRI only once
            if (ws[i].isTri()
                && lit < ws[i].lit1()
                && ws[i].lit1() < ws[i].lit2()
            ) {
                uselessLits.clear();
                lits.resize(3);
                lits[0] = lit;
                lits[1] = ws[i].lit1();
                lits[2] = ws[i].lit2();
                testVivify(
                    std::numeric_limits<ClOffset>::max()
                    , NULL
                    , ws[i].learnt()
                    , 2
                );

                //We could have modified the watchlist, better exit now
                break;
            }
        }
    }

    if (solver->conf.verbosity >= 2) {
        cout
        << "c TRI vivified: " << runStats.numClShorten - origShorten
        << " 0-detph ass: " << solver->trail.size() - origTrailSize
        << " time: " << cpuTime() - myTime
        << endl;
    }

    runStats.zeroDepthAssigns = solver->trail.size() - origTrailSize;

    return solver->ok;
}

/**
@brief Performs clause vivification (by Hamadi et al.)
*/
bool ClauseVivifier::vivifyClausesLongIrred()
{
    assert(solver->ok);

    double myTime = cpuTime();
    const size_t origTrailSize = solver->trail.size();

    //Time-limiting
    uint64_t maxNumProps = 5L*1000L*1000L;
    if (solver->irredLits + solver->redLits < 500000)
        maxNumProps *=2;

    extraTime = 0;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    bool needToFinish = false;
    runStats.potentialClauses = solver->longIrredCls.size();
    runStats.numCalled = 1;

    cout << "c WARNING!! We didn't sort by clause size here" << endl;

    uint32_t queueByBy = 2;
    if (numCalls > 8
        && (solver->irredLits + solver->redLits < 4000000)
        && (solver->longIrredCls.size() < 50000))
        queueByBy = 1;

    vector<ClOffset>::iterator i, j;
    i = j = solver->longIrredCls.begin();
    for (vector<ClOffset>::iterator end = solver->longIrredCls.end()
        ; i != end
        ; i++
    ) {
        //Check if we are in state where we only copy offsets around
        if (needToFinish || !solver->ok) {
            *j++ = *i;
            continue;
        }

        //if done enough, stop doing it
        if (solver->propStats.bogoProps-oldBogoProps + extraTime > maxNumProps) {
            if (solver->conf.verbosity >= 2) {
                cout
                << "c Need to finish asymm -- ran out of prop (=allocated time)"
                << endl;
            }
            needToFinish = true;
        }

        //Get pointer
        ClOffset offset = *i;
        Clause& cl = *solver->clAllocator->getPointer(offset);
        extraTime += cl.size();
        runStats.checkedClauses++;

        //Sanity check
        assert(cl.size() > 3);
        assert(!cl.learnt());

        //Copy literals
        uselessLits.clear();
        lits.resize(cl.size());
        std::copy(cl.begin(), cl.end(), lits.begin());

        //Try to vivify clause
        ClOffset offset2 = testVivify(
            offset
            , &cl
            , cl.learnt()
            , queueByBy
        );
        if (offset2 != std::numeric_limits<ClOffset>::max()) {
            *j++ = offset2;
        }
    }
    solver->longIrredCls.resize(solver->longIrredCls.size()- (i-j));

    //Update stats
    runStats.timeNorm = cpuTime() - myTime;
    runStats.zeroDepthAssigns = solver->trail.size() - origTrailSize;

    return solver->ok;
}

ClOffset ClauseVivifier::testVivify(
    ClOffset offset
    , Clause* cl
    , const bool learnt
    , const uint32_t queueByBy
) {
    //Try to enqueue the literals in 'queueByBy' amounts and see if we fail
    bool failed = false;
    uint32_t done = 0;
    solver->newDecisionLevel();
    for (; done < lits.size();) {
        uint32_t i2 = 0;
        for (; (i2 < queueByBy) && ((done+i2) < lits.size()); i2++) {
            lbool val = solver->value(lits[done+i2]);
            if (val == l_Undef) {
                solver->enqueue(~lits[done+i2]);
            } else if (val == l_False) {
                //Record that there is no use for this literal
                uselessLits.push_back(lits[done+i2]);
            }
        }
        done += i2;
        extraTime += 5;
        failed = (!solver->propagate().isNULL());
        if (failed) break;
    }
    solver->cancelZeroLight();
    assert(solver->ok);

    if (uselessLits.size() > 0 || (failed && done < lits.size())) {
        //Stats
        runStats.numClShorten++;
        extraTime += 20;
        const uint32_t origSize = lits.size();

        //Remove useless literals from 'lits'
        lits.resize(done);
        for (uint32_t i2 = 0; i2 < uselessLits.size(); i2++) {
            remove(lits, uselessLits[i2]);
        }


        //Detach and free clause if it's really a clause
        if (cl) {
            solver->detachClause(*cl);
            solver->clAllocator->clauseFree(offset);
        }

        //Make new clause
        Clause *cl2 = solver->addClauseInt(lits, learnt);

        //Print results
        if (solver->conf.verbosity >= 5) {
            cout
            << "c Assym branch effective." << endl;
            if (cl)
                cout
                << "c --> orig clause:" << *cl << endl;
            else
                cout
                << "c --> orig clause: TRI/BIN" << endl;
            cout
            << "c --> orig size:" << origSize << endl
            << "c --> new size:" << (cl2 == NULL ? 0 : cl2->size()) << endl
            << "c --> removing lits from end:" << origSize - done
            << "c --> useless lits in middle:" << uselessLits.size()
            << endl;
        }

        //TODO cheating here: we don't detect a NULL return that is in fact a 2/3-long clause
        runStats.numLitsRem += origSize - (cl2 == NULL ? 0 : cl2->size());

        if (cl2 != NULL) {
            return solver->clAllocator->getOffset(cl2);
        } else {
            return std::numeric_limits<ClOffset>::max();
        }
    } else {
        return offset;
    }
}

bool ClauseVivifier::vivifyClausesCache(
    vector<ClOffset>& clauses
    , bool learnt
    , bool alsoStrengthen
) {
    assert(solver->ok);

    //Stats
    uint64_t countTime = 0;
    uint64_t maxCountTime = 700000000;
    if (solver->irredLits + solver->redLits < 300000)
        maxCountTime *= 2;
    double myTime = cpuTime();

    Stats::CacheBased tmpStats;
    tmpStats.totalCls = clauses.size();
    tmpStats.numCalled = 1;

    //Temps
    vector<Lit> lits;
    vector<char> seen(solver->nVars()*2); //For strengthening
    vector<char> seen_subs(solver->nVars()*2); //For subsumption
    bool needToFinish = false;

    vector<ClOffset>::iterator i = clauses.begin();
    vector<ClOffset>::iterator j = i;
    for (vector<ClOffset>::iterator
        end = clauses.end()
        ; i != end
        ; i++
    ) {
        //Check status
        if (needToFinish) {
            *j++ = *i;
            continue;
        }
        if (countTime > maxCountTime) {
            needToFinish = true;
            tmpStats.ranOutOfTime++;
        }

        //Setup
        ClOffset offset = *i;
        Clause& cl = *solver->clAllocator->getPointer(offset);
        assert(cl.size() > 3);
        countTime += cl.size()*2;
        tmpStats.tried++;
        bool isSubsumed = false;

        //Fill 'seen'
        for (uint32_t i2 = 0; i2 < cl.size(); i2++) {
            seen[cl[i2].toInt()] = 1;
            seen_subs[cl[i2].toInt()] = 1;
        }

        //Go through each literal and subsume/strengthen with it
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end && !isSubsumed; l++) {
            const Lit lit = *l;

            //Go through the watchlist
            vec<Watched>& thisW = solver->watches[lit.toInt()];
            countTime += thisW.size();
            for(vec<Watched>::iterator
                wit = thisW.begin(), wend = thisW.end()
                ; wit != wend
                ; wit++
            ) {

                if (alsoStrengthen) {
                    //Strengthening w/ bin
                    if (wit->isBinary()
                        && seen[lit.toInt()] //We haven't yet removed it
                    ) {
                        seen[(~wit->lit1()).toInt()] = 0;
                    }

                    //Strengthening w/ tri
                    if (wit->isTri()
                        && seen[lit.toInt()] //We haven't yet removed it
                    ) {
                        if (seen[(wit->lit1()).toInt()])
                            seen[(~wit->lit2()).toInt()] = 0;
                        else if (seen[wit->lit2().toInt()])
                            seen[(~wit->lit1()).toInt()] = 0;
                    }
                }

                //Subsumption w/ bin
                if (wit->isBinary() &&
                    seen_subs[wit->lit1().toInt()]
                ) {
                    //If subsuming non-learnt with learnt, make the learnt into non-learnt
                    if (wit->learnt() && !cl.learnt()) {
                        wit->setLearnt(false);
                        findWatchedOfBin(solver->watches, wit->lit1(), lit, true).setLearnt(false);
                        solver->redBins--;
                        solver->irredBins++;
                        solver->redLits -= 2;
                        solver->irredLits += 2;
                    }
                    isSubsumed = true;
                    break;
                }

                if (wit->isTri()) {
                    assert(wit->lit1() < wit->lit2());
                }

                //Subsumption w/ tri
                if (wit->isTri()
                    && lit < wit->lit1() //Check only one instance of the TRI clause
                    && seen_subs[wit->lit1().toInt()]
                    && seen_subs[wit->lit2().toInt()]
                ) {
                    //If subsuming non-learnt with learnt, make the learnt into non-learnt
                    if (!cl.learnt() && wit->learnt()) {
                        wit->setLearnt(false);
                        findWatchedOfTri(solver->watches, wit->lit1(), lit, wit->lit2(), true).setLearnt(false);
                        findWatchedOfTri(solver->watches, wit->lit2(), lit, wit->lit1(), true).setLearnt(false);
                        solver->redTris--;
                        solver->irredTris++;
                        solver->redLits -= 3;
                        solver->irredLits += 3;
                    }
                    isSubsumed = true;
                    break;
                }
            }

            //TODO stamping -- cache was here
            /*
            if (alsoStrengthen //we need to strengthen
                && seen[lit.toInt()] //We haven't yet removed it
            ) {
                countTime += solver->implCache[lit.toInt()].lits.size();
                for (vector<LitExtra>::const_iterator it2 = solver->implCache[lit.toInt()].lits.begin()
                    , end2 = solver->implCache[lit.toInt()].lits.end(); it2 != end2; it2++
                ) {
                    seen[(~(it2->getLit())).toInt()] = 0;
                }
            }
            */

        }

        //Clear 'seen' and fill new clause data
        lits.clear();
        for (const Lit *it2 = cl.begin(), *end2 = cl.end(); it2 != end2; it2++) {
            //Only fill new clause data if clause hasn't been subsumed
            if (!isSubsumed) {
                if (seen[it2->toInt()])
                    lits.push_back(*it2);
                else
                    tmpStats.numLitsRem++;
            }

            //Clear 'seen' and 'seen_subs'
            seen[it2->toInt()] = 0;
            seen_subs[it2->toInt()] = 0;
        }

        //If nothing to do, then move along
        if (lits.size() == cl.size() && !isSubsumed) {
            *j++ = *i;
            continue;
        }

        //Else either remove or shrink clause
        countTime += cl.size()*10;
        solver->detachClause(cl);
        if (isSubsumed) {
            tmpStats.numClSubsumed++;
            solver->clAllocator->clauseFree(offset);
        } else {
            tmpStats.shrinked++;
            Clause* c2 = solver->addClauseInt(lits, cl.learnt(), cl.stats);
            solver->clAllocator->clauseFree(offset);

            if (c2 != NULL)
                *j++ = solver->clAllocator->getOffset(c2);

            if (!solver->ok)
                needToFinish = true;
        }
    }
    clauses.resize(clauses.size() - (i-j));
    solver->checkImplicitStats();

    //Set stats
    tmpStats.cpu_time = cpuTime() - myTime;
    if (learnt) {
        runStats.redCacheBased = tmpStats;
    } else {
        runStats.irredCacheBased = tmpStats;
    }

    return solver->ok;
}
