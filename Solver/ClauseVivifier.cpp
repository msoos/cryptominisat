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
#include "ThreadControl.h"
#include <iomanip>
using std::cout;
using std::endl;

//#define ASSYM_DEBUG

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

bool ClauseVivifier::SortBySize::operator()(const Clause* x, const Clause* y)
{
    return (x->size() > y->size());
}

ClauseVivifier::ClauseVivifier(ThreadControl* _control) :
    control(_control)
    , numCalls(0)
{}

bool ClauseVivifier::vivify()
{
    assert(control->ok);
    #ifdef VERBOSE_DEBUG
    cout << "c clauseVivifier started" << endl;
    #endif //VERBOSE_DEBUG
    numCalls++;

    control->clauseCleaner->cleanClauses(control->clauses, ClauseCleaner::clauses);

    if (!vivifyClausesCache(control->clauses, false))
        goto end;

    if (!vivifyClausesCache(control->learnts, true))
        goto end;

    if (!vivifyClausesNormal())
        goto end;

end:
    //Stats
    globalStats += runStats;
    if (control->conf.verbosity >= 1) {
        if (control->conf.verbosity >= 3)
            runStats.print(control->nVars());
        else
            runStats.printShort();
    }

    return control->ok;
}

struct BinSorter2 {
    bool operator()(const Watched& first, const Watched& second)
    {
        if (!first.isBinary() && !second.isBinary()) return false;
        if (first.isBinary() && !second.isBinary()) return true;
        if (second.isBinary() && !first.isBinary()) return false;

        if (!first.getLearnt() && second.getLearnt()) return true;
        if (first.getLearnt() && !second.getLearnt()) return false;
        return false;
    };
};

void ClauseVivifier::makeNonLearntBin(const Lit lit1, const Lit lit2)
{
    findWatchedOfBin(control->watches, lit1 ,lit2, true).setLearnt(false);
    findWatchedOfBin(control->watches, lit2 ,lit1, true).setLearnt(false);
}

/**
@brief Performs clause vivification (by Hamadi et al.)

Using normal propagation-based one and new cache-based one
*/
bool ClauseVivifier::vivifyClausesNormal()
{
    assert(control->ok);

    double myTime = cpuTime();
    const size_t origTrailSize = control->trail.size();

    //Time-limiting
    uint64_t maxNumProps = 5L*1000L*1000L;
    if (control->clausesLits + control->learntsLits < 500000)
        maxNumProps *=2;

    uint64_t extraDiff = 0;
    uint64_t oldBogoProps = control->propStats.bogoProps;
    bool needToFinish = false;
    runStats.potentialClauses = control->clauses.size();
    runStats.numCalled = 1;
    vector<Lit> lits;
    vector<Lit> unused;

    if (control->clauses.size() < 1000000) {
        //if too many clauses, random order will do perfectly well
        std::sort(control->clauses.begin(), control->clauses.end(), SortBySize());
    }
    //cout << "Time now: " << (cpuTime() - myTime) << endl;

    uint32_t queueByBy = 2;
    if (numCalls > 8
        && (control->clausesLits + control->learntsLits < 4000000)
        && (control->clauses.size() < 50000))
        queueByBy = 1;

    vector<Clause*>::iterator i, j;
    i = j = control->clauses.begin();
    for (vector<Clause*>::iterator end = control->clauses.end(); i != end; i++) {
        if (needToFinish) {
            *j++ = *i;
            continue;
        }

        //if done enough, stop doing it
        //cout << "Time now: " << (cpuTime() - myTime) << " todo: " << (control->bogoProps-oldBogoProps + extraDiff)/1000000 << endl;
        if (control->propStats.bogoProps-oldBogoProps + extraDiff > maxNumProps) {
            if (control->conf.verbosity >= 2) {
                cout
                << "c Need to finish asymm -- ran out of prop (=allocated time)"
                << endl;
            }
            needToFinish = true;
        }

        Clause& c = **i;
        extraDiff += c.size();
        runStats.checkedClauses++;

        assert(c.size() > 2);
        assert(!c.learnt());

        unused.clear();
        lits.resize(c.size());
        std::copy(c.begin(), c.end(), lits.begin());

        bool failed = false;
        uint32_t done = 0;
        control->newDecisionLevel();
        for (; done < lits.size();) {
            uint32_t i2 = 0;
            for (; (i2 < queueByBy) && ((done+i2) < lits.size()); i2++) {
                lbool val = control->value(lits[done+i2]);
                if (val == l_Undef) {
                    control->enqueue(~lits[done+i2]);
                } else if (val == l_False) {
                    unused.push_back(lits[done+i2]);
                }
            }
            done += i2;
            extraDiff += 5;
            failed = (!control->propagate().isNULL());
            if (failed) break;
        }
        control->cancelZeroLight();
        assert(control->ok);

        if (unused.size() > 0 || (failed && done < lits.size())) {
            runStats.numClShorten++;
            uint32_t origSize = lits.size();
            #ifdef ASSYM_DEBUG
            cout << "Assym branch effective." << endl;
            cout << "-- Orig clause:"; c.plainPrint();
            #endif
            control->detachClause(c);

            //Make 'lits' the size it should be
            lits.resize(done);
            for (uint32_t i2 = 0; i2 < unused.size(); i2++) {
                remove(lits, unused[i2]);
            }

            Clause *c2 = control->addClauseInt(lits);
            #ifdef ASSYM_DEBUG
            cout << "-- Origsize:" << origSize << " newSize:" << (c2 == NULL ? 0 : c2->size()) << " toRemove:" << c.size() - done << " unused.size():" << unused.size() << endl;
            #endif
            extraDiff += 20;
            //TODO cheating here: we don't detect a NULL return that is in fact a 2-long clause
            runStats.numLitsRem += origSize - (c2 == NULL ? 0 : c2->size());
            control->clAllocator->clauseFree(&c);

            if (c2 != NULL) {
                #ifdef ASSYM_DEBUG
                cout << "-- New clause:"; c2->plainPrint();
                #endif
                *j++ = c2;
            }

            if (!control->ok) needToFinish = true;
        } else {
            *j++ = *i;
        }
    }
    control->clauses.resize(control->clauses.size()- (i-j));
    runStats.timeNorm = cpuTime() - myTime;
    runStats.zeroDepthAssigns = control->trail.size() - origTrailSize;

    return control->ok;
}

bool ClauseVivifier::vivifyClausesCache(vector<Clause*>& clauses, bool learnt)
{
    assert(control->ok);

    //Stats
    uint64_t countTime = 0;
    uint64_t maxCountTime = 700000000;
    if (control->clausesLits + control->learntsLits < 300000)
        maxCountTime *= 2;
    double myTime = cpuTime();

    Stats::CacheBased tmpStats;
    tmpStats.totalCls = clauses.size();
    tmpStats.numCalled = 1;

    //Temps
    vector<Lit> lits;
    vector<char> seen(control->nVars()*2); //For strengthening
    vector<char> seen_subs(control->nVars()*2); //For subsumption
    bool needToFinish = false;

    vector<Clause*>::iterator i = clauses.begin();
    vector<Clause*>::iterator j = i;
    for (vector<Clause*>::iterator end = clauses.end(); i != end; i++) {
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
        Clause& cl = **i;
        assert(cl.size() > 2);
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
            const vec<Watched>& thisW = control->watches[(~lit).toInt()];
            countTime += thisW.size();
            for(vec<Watched>::const_iterator
                wit = thisW.begin(), wend = thisW.end()
                ; wit != wend
                ; wit++
            ) {

                //Strengthening w/ bin
                if (wit->isBinary()
                    && seen[lit.toInt()] //We haven't yet removed it
                ) {
                    seen[(~wit->getOtherLit()).toInt()] = 0;
                }

                //Strengthening w/ tri
                if (wit->isTriClause()
                    && seen[lit.toInt()] //We haven't yet removed it
                ) {
                    if (seen[(wit->getOtherLit()).toInt()])
                        seen[(~wit->getOtherLit2()).toInt()] = 0;
                    else if (seen[wit->getOtherLit2().toInt()])
                        seen[(~wit->getOtherLit()).toInt()] = 0;
                }

                //Subsumption w/ bin
                if (wit->isBinary() &&
                    seen_subs[wit->getOtherLit().toInt()]
                ) {
                    isSubsumed = true;
                    //If subsuming non-learnt with learnt, make the learnt into non-learnt
                    if (wit->getLearnt() && !cl.learnt()) {
                        makeNonLearntBin(lit, wit->getOtherLit());
                        control->numBinsLearnt--;
                        control->numBinsNonLearnt++;
                        control->learntsLits -= 2;
                        control->clausesLits += 2;
                    }
                    break;
                }

                //Subsumption w/ tri
                if (wit->isTriClause()
                    && cl.size() > 3 //Don't subsume clause with itself
                    && cl.learnt() //We cannot distinguish between learnt and non-learnt, so we have to do with only learnt here
                    && seen_subs[wit->getOtherLit().toInt()]
                    && seen_subs[wit->getOtherLit2().toInt()]
                ) {
                    isSubsumed = true;
                    break;
                }
            }

            //Go through the cache
            if (seen[lit.toInt()]) { //We haven't yet removed it
                countTime += control->implCache[lit.toInt()].lits.size();
                for (vector<LitExtra>::const_iterator it2 = control->implCache[lit.toInt()].lits.begin()
                    , end2 = control->implCache[lit.toInt()].lits.end(); it2 != end2; it2++
                ) {
                    seen[(~(it2->getLit())).toInt()] = 0;
                }
            }
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

        //Else either remove or shrink
        countTime += cl.size()*10;
        control->detachClause(cl);
        if (isSubsumed) {
            tmpStats.numClSubsumed++;
            control->clAllocator->clauseFree(&cl);
        } else {
            tmpStats.shrinked++;
            Clause* c2 = control->addClauseInt(lits, cl.learnt(), cl.stats);
            control->clAllocator->clauseFree(&cl);

            if (c2 != NULL)
                *j++ = c2;

            if (!control->ok)
                needToFinish = true;
        }
    }
    clauses.resize(clauses.size() - (i-j));

    //Set stats
    tmpStats.cpu_time = cpuTime() - myTime;
    if (learnt) {
        runStats.redCacheBased = tmpStats;
    } else {
        runStats.irredCacheBased = tmpStats;
    }

    return control->ok;
}
