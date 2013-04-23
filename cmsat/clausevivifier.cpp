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

#include "clausevivifier.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solver.h"
#include <iomanip>
using namespace CMSat;
using std::cout;
using std::endl;

//#define ASSYM_DEBUG
//#define DEBUG_STAMPING

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

ClauseVivifier::ClauseVivifier(Solver* _solver) :
    solver(_solver)
    , numCalls(0)
{}

bool ClauseVivifier::vivify(const bool alsoStrengthen)
{
    assert(solver->ok);
    #ifdef VERBOSE_DEBUG
    cout << "c clauseVivifier started" << endl;
    #endif //VERBOSE_DEBUG
    numCalls++;

    solver->clauseCleaner->cleanClauses(solver->longIrredCls);

    if (!vivifyClausesCache(solver->longIrredCls, false, false))
        goto end;

    if (!vivifyClausesCache(solver->longRedCls, true, false))
        goto end;

    if (alsoStrengthen) {
        if (!vivifyClausesCache(solver->longIrredCls, false, true))
            goto end;

        if (!vivifyClausesCache(solver->longRedCls, true, true))
            goto end;
    }

    if (alsoStrengthen
        && !asymmClausesLongIrred()
    ) {
        goto end;
    }

    if (alsoStrengthen
        && !vivifyClausesTriIrred()
    ) {
        goto end;
    }

end:
    //Stats
    globalStats += runStats;
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }
    runStats.clear();

    return solver->ok;
}

bool ClauseVivifier::vivifyClausesTriIrred()
{
    uint64_t origShorten = runStats.numClShorten;
    uint64_t origLitRem = runStats.numLitsRem;
    double myTime = cpuTime();
    uint64_t maxNumProps = 2LL*1000LL*1000LL;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    size_t origTrailSize = solver->trail.size();

    //Randomize start in the watchlist
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

            //Only irred TRI and each TRI only once
            if (ws[i].isTri()
                && !ws[i].learnt()
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
                    , ws[i].learnt()
                    , 2
                );

                //We could have modified the watchlist, better exit now
                break;
            }
        }

        if (!solver->okay()) {
            break;
        }
    }

    if (solver->conf.verbosity >= 3) {
        cout
        << "c [vivif] tri "
        << " tri-shorten: " << runStats.numClShorten - origShorten
        << " lit-rem: " << runStats.numLitsRem - origLitRem
        << " 0-depth ass: " << solver->trail.size() - origTrailSize
        << " T: " << std::setprecision(2) << cpuTime() - myTime
        << endl;
    }

    runStats.zeroDepthAssigns = solver->trail.size() - origTrailSize;

    return solver->ok;
}

struct ClauseSizeSorter
{
    ClauseSizeSorter(const ClauseAllocator* _clAllocator, const bool _invert = false) :
        clAllocator(_clAllocator)
        , invert(_invert)
    {}

    const ClauseAllocator* clAllocator;
    const bool invert;

    bool operator()(const ClOffset off1, const ClOffset off2) const
    {
        const Clause* cl1 = clAllocator->getPointer(off1);
        const Clause* cl2 = clAllocator->getPointer(off2);

        if (!invert)
            return cl1->size() > cl2->size();
        else
            return cl1->size() < cl2->size();
    }
};

/**
@brief Performs clause vivification (by Hamadi et al.)
*/
bool ClauseVivifier::asymmClausesLongIrred()
{
    assert(solver->ok);

    double myTime = cpuTime();
    const size_t origTrailSize = solver->trail.size();

    //Time-limiting
    uint64_t maxNumProps = 20LL*1000LL*1000LL;
    if (solver->binTri.irredLits + solver->binTri.redLits < 500000)
        maxNumProps *=2;

    extraTime = 0;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    bool needToFinish = false;
    runStats.potentialClauses = solver->longIrredCls.size();
    runStats.numCalled = 1;

    std::sort(solver->longIrredCls.begin(), solver->longIrredCls.end(), ClauseSizeSorter(solver->clAllocator));
    uint64_t origLitRem = runStats.numLitsRem;
    uint64_t origClShorten = runStats.numClShorten;

    uint32_t queueByBy = 2;
    if (numCalls > 8
        && (solver->binTri.irredLits + solver->binTri.redLits < 4000000)
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
            if (solver->conf.verbosity >= 3) {
                cout
                << "c Need to finish asymm -- ran out of prop (=allocated time)"
                << endl;
            }
            runStats.timeOut++;
            needToFinish = true;
        }

        //Get pointer
        ClOffset offset = *i;
        Clause& cl = *solver->clAllocator->getPointer(offset);
        //Time to dereference
        extraTime += 5;

        //If we already tried this clause, then move to next
        if (cl.getAsymmed()) {
            *j++ = *i;
            continue;
        } else {
            //Otherwise, this clause has been tried for sure
            cl.setAsymmed(true);
        }

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
            , cl.learnt()
            , queueByBy
        );

        if (offset2 != std::numeric_limits<ClOffset>::max()) {
            *j++ = offset2;
        }
    }
    solver->longIrredCls.resize(solver->longIrredCls.size()- (i-j));

    //If went through the whole list, then reset 'asymmed' flag
    //so next time we start from the beginning
    if (!needToFinish) {
        for (vector<ClOffset>::const_iterator
            it = solver->longIrredCls.begin(), end = solver->longIrredCls.end()
            ; it != end
            ; it++
        ) {
            Clause* cl = solver->clAllocator->getPointer(*it);
            cl->setAsymmed(false);
        }
    }

    if (solver->conf.verbosity >= 2) {
        cout << "c [asymm] longirred"
        << " tried: "
        << runStats.checkedClauses << "/" << solver->longIrredCls.size()
        << " cl-rem:"
        << runStats.numClShorten- origClShorten
        << " lits-rem:"
        << runStats.numLitsRem - origLitRem
        << " T: "
        << std::setprecision(2) << cpuTime() - myTime
        << " T-out: " << (needToFinish ? "Y" : "N")
        << endl;
    }

    //Update stats
    runStats.timeNorm = cpuTime() - myTime;
    runStats.zeroDepthAssigns = solver->trail.size() - origTrailSize;

    return solver->ok;
}

ClOffset ClauseVivifier::testVivify(
    ClOffset offset
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

        //Make new clause
        Clause *cl2 = solver->addClauseInt(lits, learnt);

        //Print results
        if (solver->conf.verbosity >= 5) {
            cout
            << "c Assym branch effective." << endl;
            if (offset != std::numeric_limits<ClOffset>::max()) {
                cout
                << "c --> orig clause:" <<
                 *solver->clAllocator->getPointer(offset)
                 << endl;
            } else {
                cout
                << "c --> orig clause: TRI/BIN" << endl;
            }
            cout
            << "c --> orig size:" << origSize << endl
            << "c --> new size:" << (cl2 == NULL ? 0 : cl2->size()) << endl
            << "c --> removing lits from end:" << origSize - done << endl
            << "c --> useless lits in middle:" << uselessLits.size()
            << endl;
        }

        //Detach and free old clause
        if (offset != std::numeric_limits<ClOffset>::max()) {
            solver->detachClause(offset);
            solver->clAllocator->clauseFree(offset);
        }

        runStats.numLitsRem += origSize - lits.size();

        if (cl2 != NULL) {
            //The new clause has been asymm-tried
            cl2->setAsymmed(true);

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
    uint64_t maxCountTime = 700ULL*1000ULL*1000ULL;
    if (!alsoStrengthen) {
        maxCountTime *= 4;
    }

    double myTime = cpuTime();

    //If it hasn't been to successful until now, don't do it so much
    Stats::CacheBased* tmp = NULL;
    if (learnt) {
        tmp = &(globalStats.redCacheBased);
    } else {
        tmp = &(globalStats.irredCacheBased);
    }
    if (tmp->numCalled > 2
        && (double)tmp->numClSubsumed/(double)tmp->triedCls < 0.05
        && (double)tmp->numLitsRem/(double)tmp->totalLits < 0.05
    ) {
        maxCountTime /= 2;
    }

    Stats::CacheBased tmpStats;
    tmpStats.totalCls = clauses.size();
    tmpStats.numCalled = 1;
    size_t remLitTimeStampTotal = 0;
    size_t remLitTimeStampTotalInv = 0;
    size_t subsumedStamp = 0;
    size_t remLitCache = 0;
    size_t remLitBinTri = 0;
    size_t subBinTri = 0;
    size_t subCache = 0;
    const bool doStamp = solver->conf.doStamp;

    //Temps
    vector<Lit> lits;
    vector<Lit> lits2;
    vector<char> seen(solver->nVars()*2); //For strengthening
    vector<char> seen_subs(solver->nVars()*2); //For subsumption
    bool needToFinish = false;

    //Randomise order of clauses
    if (!clauses.empty()) {
        countTime += clauses.size()*2;
        for(size_t i = 0; i < clauses.size()-1; i++) {
            std::swap(
                clauses[i]
                , clauses[i + solver->mtrand.randInt(clauses.size()-i-1)]
            );
        }
    }

    size_t i = 0; //solver->mtrand.randInt(clauses.size()-1);
    size_t j = i;
    const size_t end = clauses.size();
    for (
        ; i < end
        ; i++
    ) {
        //Check status
        if (needToFinish) {
            clauses[j++] = clauses[i];
            continue;
        }
        if (countTime > maxCountTime) {
            needToFinish = true;
            tmpStats.ranOutOfTime++;
        }

        //Setup
        ClOffset offset = clauses[i];
        Clause& cl = *solver->clAllocator->getPointer(offset);
        assert(cl.size() > 3);
        countTime += cl.size()*2;
        tmpStats.totalLits += cl.size();
        tmpStats.triedCls++;
        bool isSubsumed = false;
        size_t thisRemLitCache = 0;
        size_t thisRemLitBinTri = 0;

        //Fill 'seen'
        lits2.clear();
        for (uint32_t i2 = 0; i2 < cl.size(); i2++) {
            seen[cl[i2].toInt()] = 1;
            seen_subs[cl[i2].toInt()] = 1;
            lits2.push_back(cl[i2]);
        }

        //Go through each literal and subsume/strengthen with it
        for (const Lit
            *l = cl.begin(), *end = cl.end()
            ; l != end && !isSubsumed
            ; l++
        ) {
            const Lit lit = *l;

            //Use cache
            if (alsoStrengthen
                && solver->conf.doCache
                && seen[lit.toInt()] //We haven't yet removed it
             ) {
                 countTime += 2*solver->implCache[lit.toInt()].lits.size();
                 for (vector<LitExtra>::const_iterator it2 = solver->implCache[lit.toInt()].lits.begin()
                     , end2 = solver->implCache[lit.toInt()].lits.end()
                     ; it2 != end2
                     ; it2++
                 ) {
                     if (seen[(~(it2->getLit())).toInt()]) {
                        seen[(~(it2->getLit())).toInt()] = 0;
                        thisRemLitCache++;
                     }

                     if (seen_subs[it2->getLit().toInt()]
                         && it2->getOnlyNLBin()
                     ) {
                         isSubsumed = true;
                         subCache++;
                         break;
                     }
                 }
             }

             if (isSubsumed)
                 break;

            //Go through the watchlist
            vec<Watched>& thisW = solver->watches[lit.toInt()];
            countTime += thisW.size()*2 + 5;
            for(vec<Watched>::iterator
                wit = thisW.begin(), wend = thisW.end()
                ; wit != wend
                ; wit++
            ) {
                //Can't do anything with a clause
                if (wit->isClause())
                    continue;

                countTime += 5;

                if (alsoStrengthen) {
                    //Strengthening w/ bin
                    if (wit->isBinary()
                        && seen[lit.toInt()] //We haven't yet removed it
                    ) {
                        if (seen[(~wit->lit1()).toInt()]) {
                            thisRemLitBinTri++;
                            seen[(~wit->lit1()).toInt()] = 0;
                        }
                    }

                    //Strengthening w/ tri
                    if (wit->isTri()
                        && seen[lit.toInt()] //We haven't yet removed it
                    ) {
                        if (seen[(wit->lit1()).toInt()]) {
                            if (seen[(~wit->lit2()).toInt()]) {
                                thisRemLitBinTri++;
                                seen[(~wit->lit2()).toInt()] = 0;
                            }
                        } else if (seen[wit->lit2().toInt()]) {
                            if (seen[(~wit->lit1()).toInt()]) {
                                thisRemLitBinTri++;
                                seen[(~wit->lit1()).toInt()] = 0;
                            }
                        }
                    }
                }

                //Subsumption w/ bin
                if (wit->isBinary() &&
                    seen_subs[wit->lit1().toInt()]
                ) {
                    //If subsuming non-learnt with learnt, make the learnt into non-learnt
                    if (wit->learnt() && !cl.learnt()) {
                        wit->setLearnt(false);
                        countTime += solver->watches[wit->lit1().toInt()].size()*3;
                        findWatchedOfBin(solver->watches, wit->lit1(), lit, true).setLearnt(false);
                        solver->binTri.redBins--;
                        solver->binTri.irredBins++;
                        solver->binTri.redLits -= 2;
                        solver->binTri.irredLits += 2;
                    }
                    subBinTri++;
                    isSubsumed = true;
                    break;
                }

                //Extension w/ bin
                if (wit->isBinary()
                    && !wit->learnt()
                    && !seen_subs[(~(wit->lit1())).toInt()]
                ) {
                    seen_subs[(~(wit->lit1())).toInt()] = 1;
                    lits2.push_back(~(wit->lit1()));
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
                        countTime += solver->watches[wit->lit1().toInt()].size()*3;
                        countTime += solver->watches[wit->lit2().toInt()].size()*3;
                        findWatchedOfTri(solver->watches, wit->lit1(), lit, wit->lit2(), true).setLearnt(false);
                        findWatchedOfTri(solver->watches, wit->lit2(), lit, wit->lit1(), true).setLearnt(false);
                        solver->binTri.redTris--;
                        solver->binTri.irredTris++;
                        solver->binTri.redLits -= 3;
                        solver->binTri.irredLits += 3;
                    }
                    subBinTri++;
                    isSubsumed = true;
                    break;
                }

                //Extension w/ tri (1)
                if (wit->isTri()
                    && lit < wit->lit1() //Check only one instance of the TRI clause
                    && !wit->learnt()
                    && seen_subs[wit->lit1().toInt()]
                    && !seen_subs[(~(wit->lit2())).toInt()]
                ) {
                    seen_subs[(~(wit->lit2())).toInt()] = 1;
                    lits2.push_back(~(wit->lit2()));
                }

                //Extension w/ tri (2)
                if (wit->isTri()
                    && lit < wit->lit1() //Check only one instance of the TRI clause
                    && !wit->learnt()
                    && !seen_subs[(~(wit->lit1())).toInt()]
                    && seen_subs[wit->lit2().toInt()]
                ) {
                    seen_subs[(~(wit->lit1())).toInt()] = 1;
                    lits2.push_back(~(wit->lit1()));
                }

            }
        }

        //Remove through stamp
        assert(lits2.size() > 1);
        if (doStamp
            && !isSubsumed
            && !learnt
        ) {
            countTime += lits2.size()*3 + 10;
            if (solver->stamp.stampBasedClRem(lits2)) {
                isSubsumed = true;
                subsumedStamp++;
            }
        }

        //Clear 'seen2'
        countTime += lits2.size()*3;
        for (vector<Lit>::const_iterator
            it2 = lits2.begin(), end2 = lits2.end()
            ; it2 != end2
            ; it2++
        ) {
            seen_subs[it2->toInt()] = 0;
        }

        //Clear 'seen' and fill new clause data
        lits.clear();
        countTime += cl.size()*3;
        for (Clause::const_iterator
            it2 = cl.begin(), end2 = cl.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only fill new clause data if clause hasn't been subsumed
            if (!isSubsumed
                && seen[it2->toInt()]
            ) {
                lits.push_back(*it2);
            }

            //Clear 'seen' and 'seen_subs'
            seen[it2->toInt()] = 0;
        }

        //Remove lits through stamping
        if (doStamp
            && alsoStrengthen
            && lits.size() > 1
            && !isSubsumed
        ) {
            countTime += lits.size()*3 + 10;
            std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
            remLitTimeStampTotal += tmp.first;
            remLitTimeStampTotalInv += tmp.second;
        }

        //Remove lits through stamping
        if (doStamp
            && alsoStrengthen
            && lits.size() > 1
            && !isSubsumed
        ) {
            countTime += lits.size()*3 + 10;
            std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
            remLitTimeStampTotal += tmp.first;
            remLitTimeStampTotalInv += tmp.second;
        }

        //If nothing to do, then move along
        if (lits.size() == cl.size() && !isSubsumed) {
            clauses[j++] = clauses[i];
            continue;
        }

        //Else either remove or shrink clause
        countTime += cl.size()*10;
        if (!isSubsumed) {
            remLitCache += thisRemLitCache;
            remLitBinTri += thisRemLitBinTri;
            tmpStats.shrinked++;
            countTime += lits.size()*2 + 50;
            Clause* c2 = solver->addClauseInt(lits, cl.learnt(), cl.stats);

            if (c2 != NULL) {
                clauses[j++] = solver->clAllocator->getOffset(c2);
            }

            if (!solver->ok) {
                needToFinish = true;
            }
        }
        solver->detachClause(offset);
        solver->clAllocator->clauseFree(offset);
    }
    clauses.resize(clauses.size() - (i-j));
    #ifdef DEBUG_IMPLICIT_STATS
    solver->checkImplicitStats();
    #endif

    //Set stats
    tmpStats.numClSubsumed += subBinTri + subsumedStamp + subCache;
    tmpStats.numLitsRem += remLitBinTri + remLitCache + remLitTimeStampTotal + remLitTimeStampTotalInv;
    tmpStats.cpu_time = cpuTime() - myTime;
    if (learnt) {
        runStats.redCacheBased = tmpStats;
    } else {
        runStats.irredCacheBased = tmpStats;
    }

    if (solver->conf.verbosity >= 2) {
        cout
        << "c [cl-str] stamp-based"
        << " lit-rem: " << remLitTimeStampTotal
        << " inv-lit-rem: " << remLitTimeStampTotalInv
        << " stamp-cl-rem: " << subsumedStamp
        << endl;

        cout
        << "c [cl-str] bintri-based"
        << " lit-rem: " << remLitBinTri
        << " cl-sub: " << subBinTri
        << endl;

        cout
        << "c [cl-str] cache-based"
        << " lit-rem: " << remLitCache
        << " cl-sub: " << subCache
        << endl;
    }

    return solver->ok;
}

void ClauseVivifier::subsumeImplicit()
{
    assert(solver->okay());
    const double myTime = cpuTime();
    uint64_t remBins = 0;
    uint64_t remTris = 0;
    uint64_t stampTriRem = 0;
    uint64_t cacheTriRem = 0;
    timeAvailable = 1900LL*1000LL*1000LL;
    const bool doStamp = solver->conf.doStamp;
    uint64_t numWatchesLooked = 0;

    //Randomize starting point
    size_t upI;
    upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (; numDone < solver->watches.size() && timeAvailable > 0
        ; upI = (upI +1) % solver->watches.size(), numDone++

    ) {
        numWatchesLooked++;
        Lit lit = Lit::toLit(upI);
        vec<Watched>& ws = solver->watches[upI];

        //We can't do much when there is nothing, or only one
        if (ws.size() < 2)
            continue;

        timeAvailable -= ws.size()*std::ceil(std::log((double)ws.size())) + 20;
        std::sort(ws.begin(), ws.end(), WatchSorter());
        /*cout << "---> Before" << endl;
        printWatchlist(ws, lit);*/

        Watched* i = ws.begin();
        Watched* j = i;
        Watched* lastBin = NULL;

        Lit lastLit = lit_Undef;
        Lit lastLit2 = lit_Undef;
        bool lastLearnt = false;
        for (vec<Watched>::iterator end = ws.end(); i != end; i++) {

            //Don't care about long clauses
            if (i->isClause() || timeAvailable < 0) {
                *j++ = *i;
                continue;
            }

            if (i->isTri()) {

                //Only treat one of the TRI's instances
                if (lit > i->lit1()) {
                    *j++ = *i;
                    continue;
                }

                bool remove = false;

                //Subsumed by bin
                if (lastLit == i->lit1()
                    && lastLit2 == lit_Undef
                    && lastLit == i->lit1()
                ) {
                    if (lastLearnt && !i->learnt()) {
                        assert(lastBin->isBinary());
                        assert(lastBin->learnt());
                        assert(lastBin->lit1() == lastLit);

                        lastBin->setLearnt(false);
                        timeAvailable -= 20;
                        timeAvailable -= solver->watches[lastLit.toInt()].size();
                        findWatchedOfBin(solver->watches, lastLit, lit, true).setLearnt(false);
                        solver->binTri.redLits -= 2;
                        solver->binTri.irredLits += 2;
                        solver->binTri.redBins--;
                        solver->binTri.irredBins++;
                        lastLearnt = false;
                    }

                    remove = true;
                }

                //Subsumed by Tri
                if (!remove
                    && lastLit == i->lit1()
                    && lastLit2 == i->lit2()
                ) {
                    //The sorting algorithm prefers non-learnt to learnt, so it is
                    //impossible to have non-learnt before learnt
                    assert(!(i->learnt() == false && lastLearnt == true));

                    remove = true;
                }

                lits.clear();
                lits.push_back(lit);
                lits.push_back(i->lit1());
                lits.push_back(i->lit2());

                //Subsumed by stamp
                if (doStamp && !remove) {
                    timeAvailable -= 15;
                    remove = solver->stamp.stampBasedClRem(lits);
                    stampTriRem += remove;
                }

                //Subsumed by cache
                if (!remove
                    && solver->conf.doCache
                ) {
                    for(size_t i = 0; i < lits.size() && !remove; i++) {
                        timeAvailable -= solver->implCache[lit.toInt()].lits.size();
                        for (vector<LitExtra>::const_iterator
                            it2 = solver->implCache[lits[i].toInt()].lits.begin()
                            , end2 = solver->implCache[lits[i].toInt()].lits.end()
                            ; it2 != end2
                            ; it2++
                        ) {
                            if ((   it2->getLit() == lits[0]
                                    || it2->getLit() == lits[1]
                                    || it2->getLit() == lits[2]
                                )
                                && it2->getOnlyNLBin()
                            ) {
                                remove = true;
                                cacheTriRem++;
                                break;
                             }
                        }
                    }
                }

                if (remove) {
                    //Remove Tri
                    timeAvailable -= 30;
                    timeAvailable -= solver->watches[lit.toInt()].size();
                    timeAvailable -= solver->watches[i->lit1().toInt()].size();
                    timeAvailable -= solver->watches[i->lit2().toInt()].size();
                    removeTri(lit, i->lit1(), i->lit2(), i->learnt());
                    remTris++;

                    #ifdef DRUP
                    if (solver->drup) {
                        (*solver->drup)
                        << "d "
                        << lit << " "
                        << i->lit1() << " "
                        << i->lit2() << " 0"
                        << endl;
                    }
                    #endif
                    continue;
                }

                //Don't remove
                lastLit = i->lit1();
                lastLit2 = i->lit2();
                lastLearnt = i->learnt();
                *j++ = *i;
                continue;
            }

            //Binary from here on
            assert(i->isBinary());

            //Subsume bin with bin
            if (i->lit1() == lastLit && lastLit2 == lit_Undef) {
                //The sorting algorithm prefers non-learnt to learnt, so it is
                //impossible to have non-learnt before learnt
                assert(!(i->learnt() == false && lastLearnt == true));

                remBins++;
                assert(i->lit1().var() != lit.var());
                timeAvailable -= 30;
                timeAvailable -= solver->watches[i->lit1().toInt()].size();
                removeWBin(solver->watches, i->lit1(), lit, i->learnt());
                if (i->learnt()) {
                    solver->binTri.redLits -= 2;
                    solver->binTri.redBins--;
                } else {
                    solver->binTri.irredLits -= 2;
                    solver->binTri.irredBins--;
                }

                #ifdef DRUP
                if (solver->drup) {
                    (*solver->drup)
                    << "d "
                    << lit << " "
                    << i->lit1() << " 0"
                    << endl;
                }
                #endif

                continue;
            } else {
                lastBin = j;
                lastLit = i->lit1();
                lastLit2 = lit_Undef;
                lastLearnt = i->learnt();
                *j++ = *i;
            }
        }
        ws.shrink(i-j);
    }

    if (solver->conf.verbosity >= 1) {
        cout
        << "c [implicit] sub"
        << " bin: " << remBins
        << " tri: " << remTris << " (stamp: " << stampTriRem << ", cache: " << cacheTriRem << ")"

        << " T: " << std::fixed << std::setprecision(2)
        << (cpuTime() - myTime)
        << " T-out: " << (timeAvailable < 0 ? "Y" : "N")
        << " w-visit: " << numWatchesLooked
        << endl;
    }
    solver->checkStats();

    //Update stats
    solver->solveStats.subsBinWithBinTime += cpuTime() - myTime;
    solver->solveStats.subsBinWithBin += remBins;
}

bool ClauseVivifier::strengthenImplicit()
{
    uint64_t remLitFromBin = 0;
    uint64_t remLitFromTri = 0;
    uint64_t remLitFromTriByBin = 0;
    uint64_t remLitFromTriByTri = 0;
    uint64_t stampRem = 0;
    const size_t origTrailSize = solver->trail.size();
    timeAvailable = 1000LL*1000LL*1000LL;
    double myTime = cpuTime();
    const bool doStamp = solver->conf.doStamp;
    uint64_t numWatchesLooked = 0;

    //For delayed enqueue and binary adding
    //Used for strengthening
    vector<BinaryClause> binsToAdd;
    vector<Lit> toEnqueue;

    //Randomize starting point
    size_t upI;
    upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (; numDone < solver->watches.size() && timeAvailable > 0
        ; upI = (upI +1) % solver->watches.size(), numDone++

    ) {
        numWatchesLooked++;
        Lit lit = Lit::toLit(upI);
        vec<Watched>& ws = solver->watches[upI];

        Watched* i = ws.begin();
        Watched* j = i;
        for (vec<Watched>::iterator
            end = ws.end()
            ; i != end
            ; i++
        ) {
            timeAvailable -= 2;
            //Can't do much with clause, will treat them during vivification
            //Or timeout
            if (i->isClause() || timeAvailable < 0) {
                *j++ = *i;
                continue;
            }

            timeAvailable -= 20;

            //Strengthen bin with bin -- effectively setting literal
            if (i->isBinary()) {
                lits.clear();
                lits.push_back(lit);
                lits.push_back(i->lit1());
                if (doStamp) {
                    timeAvailable -= 10;
                    std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
                    stampRem += tmp.first;
                    stampRem += tmp.second;
                    assert(!lits.empty());
                    if (lits.size() == 1) {
                        toEnqueue.push_back(lits[0]);

                        #ifdef DRUP
                        if (solver->drup) {
                            (*solver->drup)
                            << lits[0] << " 0"
                            << endl;
                        }
                        #endif
                        remLitFromBin++;
                        stampRem++;
                        *j++ = *i;
                        continue;
                    }
                }

                //If inverted, then the inverse will never be found, because
                //watches are sorted
                if (i->lit1().sign()) {
                    *j++ = *i;
                    continue;
                }

                //Try to look for a binary in this same watchlist
                //that has ~i->lit1() inside. Everything is sorted, so we are
                //lucky, this is speedy
                bool rem = false;
                vec<Watched>::const_iterator i2 = i;
                while(i2 != end
                    && (i2->isBinary() || i2->isTri())
                    && i2->lit1().var() == i2->lit1().var()
                ) {
                    timeAvailable -= 2;
                    //Yay, we have found what we needed!
                    if (i2->isBinary() && i2->lit1() == ~i->lit1()) {
                        rem = true;
                        break;
                    }

                    i2++;
                }

                //Enqeue literal
                if (rem) {
                    remLitFromBin++;
                    toEnqueue.push_back(lit);
                    #ifdef DRUP
                    if (solver->drup) {
                        (*solver->drup)
                        << lit << " 0"
                        << endl;
                    }
                    #endif

                }
                *j++ = *i;
                continue;
            }

            //Strengthen tri with bin/tri/stamp
            if (i->isTri()) {
                const Lit lit1 = i->lit1();
                const Lit lit2 = i->lit2();
                bool rem = false;

                timeAvailable -= solver->watches[(~lit).toInt()].size();
                for(vec<Watched>::const_iterator
                    it2 = solver->watches[(~lit).toInt()].begin(), end2 = solver->watches[(~lit).toInt()].end()
                    ; it2 != end2 && timeAvailable > 0
                    ; it2++
                ) {
                    if (it2->isBinary()
                        && (it2->lit1() == lit1 || it2->lit1() == lit2)
                    ) {
                        rem = true;
                        remLitFromTriByBin++;
                        break;
                    }

                    if (it2->isTri()
                        && (
                            (it2->lit1() == lit1 && it2->lit2() == lit2)
                            ||
                            (it2->lit1() == lit2 && it2->lit2() == lit1)
                        )

                    ) {
                        rem = true;
                        remLitFromTriByTri++;
                        break;
                    }

                    //watches are sorted, so early-abort
                    if (it2->isClause())
                        break;
                }

                if (rem) {
                    removeTri(lit, i->lit1(), i->lit2(), i->learnt());
                    remLitFromTri++;
                    binsToAdd.push_back(BinaryClause(i->lit1(), i->lit2(), i->learnt()));
                    #ifdef DRUP
                    if (solver->drup) {
                        (*solver->drup)
                        //Add shortened
                        << i->lit1() << " "
                        << i->lit2()
                        << " 0"
                        << endl

                        //Delete old
                        << "d "
                        << lit << " "
                        << i->lit1() << " "
                        << i->lit2()
                        << " 0"
                        << endl;
                    }
                    #endif


                    continue;
                }

                if (doStamp) {
                    //Strengthen TRI using stamps
                    lits.clear();
                    lits.push_back(lit);
                    lits.push_back(i->lit1());
                    lits.push_back(i->lit2());

                    //Try both stamp types to reduce size
                    timeAvailable -= 15;
                    std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
                    stampRem += tmp.first;
                    stampRem += tmp.second;
                    if (lits.size() > 1) {
                        timeAvailable -= 15;
                        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
                        stampRem += tmp.first;
                        stampRem += tmp.second;
                    }

                    if (lits.size() == 2) {
                        removeTri(lit, i->lit1(), i->lit2(), i->learnt());
                        remLitFromTri++;
                        binsToAdd.push_back(BinaryClause(lits[0], lits[1], i->learnt()));
                        #ifdef DRUP
                        if (solver->drup) {
                            (*solver->drup)
                            //Add shortened
                            << lits[0] << " "
                            << lits[1]
                            << " 0"
                            << endl

                            //Delete old
                            << "d "
                            << lit << " "
                            << i->lit1() << " "
                            << i->lit2()
                            << " 0"
                            << endl;
                        }
                        #endif

                        continue;
                    } else if (lits.size() == 1) {
                        removeTri(lit, i->lit1(), i->lit2(), i->learnt());
                        remLitFromTri+=2;
                        toEnqueue.push_back(lits[0]);
                        #ifdef DRUP
                        if (solver->drup) {
                            (*solver->drup)
                            //Add shortened
                            << lits[0] << " 0"
                            << endl

                            //Delete old
                            << "d "
                            << lit << " "
                            << i->lit1() << " "
                            << i->lit2()
                            << " 0"
                            << endl;
                        }
                        #endif

                        continue;
                    }
                }


                //Nothing to do, copy
                *j++ = *i;
                continue;
            }

            //Only bin, tri and clause in watchlist
            assert(false);
        }
        ws.shrink(i-j);
    }

    //Enqueue delayed values
    if (!solver->enqueueThese(toEnqueue))
        goto end;

    //Add delayed binary clauses
    for(vector<BinaryClause>::const_iterator
        it = binsToAdd.begin(), end = binsToAdd.end()
        ; it != end
        ; it++
    ) {
        lits.clear();
        lits.push_back(it->getLit1());
        lits.push_back(it->getLit2());
        timeAvailable -= 5;
        solver->addClauseInt(lits, it->getLearnt());
        if (!solver->okay())
            goto end;
    }

end:

    if (solver->conf.verbosity >= 1) {
        cout
        << "c [implicit] str"
        << " lit bin: " << remLitFromBin
        << " lit tri: " << remLitFromTri << " (by tri: " << remLitFromTriByTri << ")"
        << " (by stamp: " << stampRem << ")"
        << " set-var: " << solver->trail.size() - origTrailSize

        << " T: " << std::fixed << std::setprecision(2)
        << (cpuTime() - myTime)
        << " T-out: " << (timeAvailable < 0 ? "Y" : "N")
        << " w-visit: " << numWatchesLooked
        << endl;
    }
    solver->checkStats();

    //Update stats
    solver->solveStats.subsBinWithBinTime += cpuTime() - myTime;

    return solver->okay();
}

void ClauseVivifier::removeTri(
    const Lit lit1
    ,const Lit lit2
    ,const Lit lit3
    ,const bool learnt
) {
    //Remove tri
    Lit lits[3];
    lits[0] = lit1;
    lits[1] = lit2;
    lits[2] = lit3;
    std::sort(lits, lits+3);
    timeAvailable -= solver->watches[lits[0].toInt()].size();
    timeAvailable -= solver->watches[lits[1].toInt()].size();
    timeAvailable -= solver->watches[lits[2].toInt()].size();
    removeTriAllButOne(solver->watches, lit1, lits, learnt);

    //Update stats for tri
    if (learnt) {
        solver->binTri.redLits -= 3;
        solver->binTri.redTris--;
    } else {
        solver->binTri.irredLits -= 3;
        solver->binTri.irredTris--;
    }
}
