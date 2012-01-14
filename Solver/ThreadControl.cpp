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

#include "ThreadControl.h"
#include "VarReplacer.h"
#include "time_mem.h"
#include "CommandControl.h"
#include "SCCFinder.h"
#include "Subsumer.h"
#include "FailedLitSearcher.h"
#include "RestartPrinter.h"
#include "ClauseVivifier.h"
#include "ClauseCleaner.h"
#include "SolutionExtender.h"
#include <omp.h>

ThreadControl::ThreadControl(const SolverConf& _conf) :
    Solver(NULL, AgilityData(_conf.agilityG, _conf.forgetLowAgilityAfter, _conf.countAgilityFromThisConfl))
    , mtrand(_conf.origSeed)
    , nbReduceDB(0)
    , conf(_conf)
    , needToInterrupt(false)
    , sumConflicts(0)
    , numDecisionVars(0)
    , clausesLits(0)
    , learntsLits(0)
    , numBins(0)
{
    failedLitSearcher = new FailedLitSearcher(this);
    subsumer = new Subsumer(this);
    sCCFinder = new SCCFinder(this);
    clauseVivifier = new ClauseVivifier(this);
    clauseCleaner = new ClauseCleaner(this);
    clAllocator = new ClauseAllocator;
    restPrinter = new RestartPrinter(this);
    varReplacer = new VarReplacer(this);

    Solver::clAllocator = clAllocator;
}

ThreadControl::~ThreadControl()
{
    delete failedLitSearcher;
    delete subsumer;
    delete sCCFinder;
    delete clauseVivifier;
    delete clauseCleaner;
    delete clAllocator;
    delete restPrinter;
    delete varReplacer;
}

bool ThreadControl::addXorClauseInt(const vector< Lit >& lits, bool rhs)
{
    assert(ok);
    assert(qhead == trail.size());
    assert(decisionLevel() == 0);

    if (lits.size() > (0x01UL << 18)) {
        std::cout << "Too long clause!" << std::endl;
        exit(-1);
    }

    vector<Lit> ps(lits);
    std::sort(ps.begin(), ps.end());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        assert(!ps[i].sign()); //Every literal has to be unsigned

        if (ps[i].var() == p.var()) {
            //added, but easily removed
            j--;
            p = lit_Undef;
            if (!assigns[ps[i].var()].isUndef())
                rhs ^= assigns[ps[i].var()].getBool();
        } else if (assigns[ps[i].var()].isUndef()) { //just add
            ps[j++] = p = ps[i];
            assert(!subsumer->getVarElimed()[p.var()]);
        } else //modify rhs instead of adding
            rhs ^= (assigns[ps[i].var()].getBool());
    }
    ps.resize(ps.size() - (i - j));

    switch(ps.size()) {
        case 0:
            if (rhs)
                ok = false;
            return ok;

        case 1:
            enqueue(Lit(ps[0].var(), !rhs));
            ok = propagate().isNULL();
            return ok;

        case 2:
            ps[0] ^= !rhs;
            addClauseInt(ps, false);
            if (!ok)
                return false;

            ps[0] ^= true;
            ps[1] ^= true;
            addClauseInt(ps, false);
            break;

        default:
            assert(false && "larger than 2-long XORs are not supported yet");
            break;
    }

    return ok;
}

/**
@brief Adds a clause to the problem. Should ONLY be called internally

This code is very specific in that it must NOT be called with varibles in
"ps" that have been replaced, eliminated, etc. Also, it must not be called
when the solver is in an UNSAT (!ok) state, for example. Use it carefully,
and only internally
*/
template <class T>
Clause* ThreadControl::addClauseInt(const T& lits
                            , const bool learnt
                            , const uint32_t glue
                            , const bool attach)
{
    assert(ok);
    assert(decisionLevel() == 0);
    assert(qhead == trail.size());
    #ifdef VERBOSE_DEBUG
    std::cout << "addClauseInt clause " << lits << std::endl;
    #endif //VERBOSE_DEBUG

    vector<Lit> ps(lits.size());
    std::copy(lits.begin(), lits.end(), ps.begin());

    std::sort(ps.begin(), ps.end());
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i != ps.size(); i++) {
        if (value(ps[i]).getBool() || ps[i] == ~p)
            return NULL;
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];
            assert(varData[p.var()].elimed == ELIMED_NONE
                    || varData[p.var()].elimed == ELIMED_QUEUED_VARREPLACER);
        }
    }
    ps.resize(ps.size() - (i - j));

    //Handle special cases
    switch (ps.size()) {
        case 0:
            ok = false;
            return NULL;
        case 1:
            enqueue(ps[0]);
            ok = (propagate().isNULL());
            return NULL;
        case 2:
            attachBinClause(ps[0], ps[1], learnt);
            return NULL;
        default:
            Clause* c = clAllocator->Clause_new(ps);
            if (learnt) c->makeLearnt(glue);

            if (attach) attachClause(*c);
            return c;
    }
}

template Clause* ThreadControl::addClauseInt(const Clause& ps, const bool learnt, const uint32_t glue, const bool attach);
template Clause* ThreadControl::addClauseInt(const vector<Lit>& ps, const bool learnt, const uint32_t glue, const bool attach);

void ThreadControl::attachClause(const Clause& c, const uint16_t point1, const uint16_t point2)
{
    if (c.learnt()) learntsLits += c.size();
    else            clausesLits += c.size();
    Solver::attachClause(c, point1, point2);
}

void ThreadControl::attachBinClause(const Lit lit1, const Lit lit2, const bool learnt, const bool checkUnassignedFirst)
{
    if (learnt) learntsLits += 2;
    else        clausesLits += 2;
    numBins++;
    Solver::attachBinClause(lit1, lit2, learnt, checkUnassignedFirst);
}

void ThreadControl::detachModifiedClause(const Lit lit1, const Lit lit2, const Lit lit3, const uint32_t origSize, const Clause* address)
{
    if (address->learnt()) learntsLits -= origSize;
    else                   clausesLits -= origSize;
    Solver::detachModifiedClause(lit1, lit2, lit3, origSize, address);
}

bool ThreadControl::addClauseHelper(vector<Lit>& ps)
{
    //Sanity checks
    assert(decisionLevel() == 0);
    assert(qhead == trail.size());
    if (ps.size() > (0x01UL << 18)) {
        std::cout << "Too long clause!" << std::endl;
        exit(-1);
    }
    for (vector<Lit>::const_iterator it = ps.begin(), end = ps.end(); it != end; it++) {
        assert(it->var() < nVars() && "Clause inserted, but variable inside has not been declared with Solver::newVar() !");
    }

    if (!ok)
        return false;

    for (uint32_t i = 0; i != ps.size(); i++) {
        //Update to correct var
        ps[i] = varReplacer->getReplaceTable()[ps[i].var()] ^ ps[i].sign();

        //Uneliminate var if need be
        if (subsumer->getVarElimed()[ps[i].var()]
            && !subsumer->unEliminate(ps[i].var(), this)
        ) return false;
    }

    //Randomise
    for (uint32_t i = 0; i < ps.size(); i++) {
        std::swap(ps[i], ps[(mtrand.randInt() % (ps.size()-i)) + i]);
    }

    return true;
}

/**
@brief Adds a clause to the problem. Calls addClauseInt() for heavy-lifting

Checks whether the
variables of the literals in "ps" have been eliminated/replaced etc. If so,
it acts on them such that they are correct, and calls addClauseInt() to do
the heavy-lifting
*/
bool ThreadControl::addClause(const vector<Lit>& lits)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "Adding clause " << lits << std::endl;
    #endif //VERBOSE_DEBUG
    vector<Lit> ps = lits;

    if (!addClauseHelper(ps))
        return false;

    Clause* c = addClauseInt(ps);
    if (c != NULL)
        clauses.push_back(c);

    return ok;
}

bool ThreadControl::addLearntClause(const vector<Lit>& lits, const uint32_t glue)
{
    vector<Lit> ps(lits.size());
    std::copy(lits.begin(), lits.end(), ps.begin());

    if (!addClauseHelper(ps))
        return false;

    Clause* c = addClauseInt(ps, true, glue);
    if (c != NULL)
        learnts.push_back(c);

    return ok;
}

void ThreadControl::reArrangeClause(Clause* clause)
{
    Clause& c = *clause;
    assert(c.size() > 2);
    if (c.size() == 3) return;

    //Change anything, but find the first two and assign them
    //accordingly at the ClauseData
    ClauseData& data = clauseData[c.getNum()];
    const Lit lit1 = c[data[0]];
    const Lit lit2 = c[data[1]];
    assert(lit1 != lit2);

    std::sort(c.begin(), c.end(), PolaritySorter(varData));

    uint8_t foundDatas = 0;
    for (uint16_t i = 0; i < c.size(); i++) {
        if (c[i] == lit1) {
            data[0] = i;
            foundDatas++;
        }
        if (c[i] == lit2) {
            data[1] = i;
            foundDatas++;
        }
    }
    assert(foundDatas == 2);
}

void ThreadControl::reArrangeClauses()
{
    assert(decisionLevel() == 0);
    assert(ok);
    assert(qhead == trail.size());

    double myTime = cpuTime();
    for (uint32_t i = 0; i < clauses.size(); i++) {
        reArrangeClause(clauses[i]);
    }
    for (uint32_t i = 0; i < learnts.size(); i++) {
        reArrangeClause(learnts[i]);
    }

    if (conf.verbosity >= 3) {
        std::cout << "c Rearrange lits in clauses "
        << std::setw(4) << std::setprecision(2) << (cpuTime() - myTime)  << " s"
        << std::endl;
    }
}

Var ThreadControl::newVar(const bool dvar)
{
    Solver::newVar();

    decision_var.push_back(dvar);
    numDecisionVars += dvar;

    implCache.addNew();
    litReachable.push_back(LitReachData());
    litReachable.push_back(LitReachData());

    varReplacer->newVar();
    subsumer->newVar();

    return decision_var.size()-1;
}

/**
@brief Clean leart clause database

Locked clauses are clauses that are reason to some assignment.
Binary&Tertiary clauses are never removed.
*/
bool ThreadControl::reduceDBStruct::operator () (const Clause* x, const Clause* y) {
    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    assert(xsize > 2 && ysize > 2);
    if (x->getGlue() > y->getGlue()) return 1;
    if (x->getGlue() < y->getGlue()) return 0;
    return xsize > ysize;
}

/**
@brief Removes learnt clauses that have been found not to be too good

Either based on glue or MiniSat-style learnt clause activities, the clauses are
sorted and then removed
*/
void ThreadControl::reduceDB()
{
    nbReduceDB++;
    std::sort(learnts.begin(), learnts.end(), reduceDBStruct());

    #ifdef VERBOSE_DEBUG
    std::cout << "Cleaning learnt clauses. Learnt clauses after sort: " << std::endl;
    for (uint32_t i = 0; i != learnts.size(); i++) {
        std::cout << "activity:" << learnts[i]->getGlue()
        << " \tsize:" << learnts[i]->size() << std::endl;
    }
    #endif

    uint32_t removeNum = (double)learnts.size() * conf.ratioRemoveClauses;

    //Statistics about clauses removed
    uint32_t totalNumRemoved = 0;
    uint32_t totalNumNonRemoved = 0;
    uint64_t totalGlueOfRemoved = 0;
    uint64_t totalSizeOfRemoved = 0;
    uint64_t totalGlueOfNonRemoved = 0;
    uint64_t totalSizeOfNonRemoved = 0;
    uint32_t numThreeLongLearnt = 0;

    uint32_t i, j;
    for (i = j = 0; i < std::min(removeNum, (uint32_t)learnts.size()); i++) {
        if (i+1 < learnts.size())
            __builtin_prefetch(learnts[i+1], 0);

        //const uint32_t clNum = learnts[i]->getNum();
        assert(learnts[i]->size() > 2);
        if (//locked[clNum] &&
            learnts[i]->getGlue() > 2
            && learnts[i]->size() > 3 //we cannot update activity of 3-longs because of watchlists
        ) {
            totalGlueOfRemoved += learnts[i]->getGlue();
            totalSizeOfRemoved += learnts[i]->size();
            totalNumRemoved++;
            detachClause(*learnts[i]);
            toDetach.push_back(learnts[i]);
        } else {
            totalGlueOfNonRemoved += learnts[i]->getGlue();
            totalSizeOfNonRemoved += learnts[i]->size();
            totalNumNonRemoved++;
            numThreeLongLearnt += (learnts[i]->size()==3);
            removeNum++;
            learnts[j++] = learnts[i];
        }
    }

    //Count what is left
    for (; i < learnts.size(); i++) {
        totalGlueOfNonRemoved += learnts[i]->getGlue();
        totalSizeOfNonRemoved += learnts[i]->size();
        totalNumNonRemoved++;
        numThreeLongLearnt += (learnts[i]->size()==3);
        learnts[j++] = learnts[i];
    }
    learnts.resize(learnts.size() - (i - j));

    if (conf.verbosity >= 1) {
        std::cout << "c rem " << std::setw(6) << totalNumRemoved
        << "  avgGlue "
        << std::fixed << std::setw(5) << std::setprecision(2)  << ((double)totalGlueOfRemoved/(double)totalNumRemoved)
        << "  avgSize "
        << std::fixed << std::setw(6) << std::setprecision(2) << ((double)totalSizeOfRemoved/(double)totalNumRemoved)
        << "  || remain " << std::setw(6) << totalNumNonRemoved
        << "  avgGlue "
        << std::fixed << std::setw(5) << std::setprecision(2)  << ((double)totalGlueOfNonRemoved/(double)totalNumNonRemoved)
        << "  avgSize "
        << std::fixed << std::setw(6) << std::setprecision(2) << ((double)totalSizeOfNonRemoved/(double)totalNumNonRemoved)
        //<< "  3-long: " << std::setw(6) << numThreeLongLearnt
        << "  sumConflicts:" << sumConflicts
        << std::endl;
    }
}

void ThreadControl::moveClausesHere()
{
    assert(ok);
    assert(decisionLevel() == 0);
    vector<Lit> lits;

    lits.resize(1);
    for(vector<Lit>::const_iterator it = unitLearntsToAdd.begin(), end = unitLearntsToAdd.end(); it != end; it++) {
        lits[0] = *it;;
        addClauseInt(lits, true);
        assert(ok);
    }
    unitLearntsToAdd.clear();

    lits.resize(2);
    for(vector<BinaryClause>::const_iterator it = binLearntsToAdd.begin(), end = binLearntsToAdd.end(); it != end; it++) {
        lits[0] = it->getLit1();
        lits[1] = it->getLit2();
        addClauseInt(lits, true);
        assert(ok);
    }
    binLearntsToAdd.clear();

    //We might attach it, then need to detach it, because it becomes satisfied
    vector<char> attached(longLearntsToAdd.size(), 0);
    size_t at = 0;
    for(vector<Clause*>::const_iterator
        it = longLearntsToAdd.begin()
        , end = longLearntsToAdd.end()
        ; it != end
        ; it++, at++
    ) {
        if (clauseCleaner->satisfied(**it)) {
            continue;
        } else {
            uint32_t points[2];
            uint32_t numPoints = 0;
            Clause& cl = **it;
            for(uint32_t i = 0; i < cl.size(); i++) {
                if (value(cl[i]) == l_Undef) {
                    points[numPoints] = i;
                    numPoints++;
                }
                //Moving 2 lits to front is enough
                if (numPoints == 2)
                    break;
            }
            switch(numPoints) {
                case 0:
                    assert(false && "Not satisfied, but no l_Undef --> UNSAT, but that should have been reported!");
                    break;
                case 1:
                    lits.resize(1);
                    lits[0] = cl[points[0]];
                    addClauseInt(lits, true);
                    assert(ok);
                    break;
                default:
                    attached[at] = 1;
                    attachClause(cl, points[0], points[1]);
                    break;
            };
        }
    }

    at = 0;
    for(vector<Clause*>::const_iterator
        it = longLearntsToAdd.begin()
        , end = longLearntsToAdd.end()
        ; it != end
        ; it++, at++
    ) {
        if (clauseCleaner->satisfied(**it)) {
            toDetach.push_back(*it);
            if (attached[at]) {
                //We had to attach it so we would get the right 'trail' size
                //But it is no longer needed, so detach
                detachClause(**it);
            }
        } else {
            learnts.push_back(*it);
        }
    }
    std::cout << "c CommandContr trail size: " << trail.size() << std::endl;
    longLearntsToAdd.clear();
}

void ThreadControl::toDetachFree()
{
    for(vector<Clause*>::const_iterator it = toDetach.begin(), end = toDetach.end(); it != end; it++) {
        clAllocator->clauseFree(*it);
    }
    toDetach.clear();
    //findAllAttach();
}

lbool ThreadControl::solve(const int numThreads)
{
    restPrinter->printStatHeader();
    restPrinter->printRestartStat("B");

    //Initialise stuff
    vector<lbool> solution;
    uint32_t numConfls = 40000;
    nextCleanLimit = 20000;
    nextCleanLimitInc = 20000;

    //Solve in infinite loop
    lbool status = ok ? l_Undef : l_False;
    if (status == l_Undef)
        status = simplifyProblem(conf.simpBurstSConf);

    omp_set_num_threads(numThreads);

    while (status == l_Undef) {
        restPrinter->printRestartStat("N");
        calcClauseDistrib();

        //This is crucial, since we need to attach() clauses to threads
        clauseCleaner->removeAndCleanAll();

        //Solve using threads
        vector<lbool> statuses;
        #pragma omp parallel
        {
            CommandControl *cc = new CommandControl(conf, this);
            size_t at;
            #pragma omp critical (threadadd)
            {
                at = threads.size();
                threads.push_back(cc);
                statuses.push_back(l_Undef);
            }
            #pragma omp barrier

            statuses[at] = threads[at]->solve(numConfls);
        }

        for (size_t i = 0; i < statuses.size(); i++) {
            if (statuses[i] == l_True) {
                solution = threads[0]->solution;
                status = l_True;
                continue;
            }

            if (statuses[i] == l_False)
                status = l_False;
        }

        for (size_t i = 0; i < varData.size(); i++) {
            varData[i].polarity = threads[0]->getSavedPolarity(i);
        }

        #pragma omp parallel for
        for(size_t i = 0; i < threads.size(); i++) {
            delete threads[i];
        }
        threads.clear();

        if (status != l_Undef)
            break;

        //Move data, print data
        moveClausesHere();
        toDetachFree();
        restPrinter->printRestartStat("N");

        //Simplify
        status = simplifyProblem(conf.simpBurstSConf);
        numConfls *= 2;
    }

    //Handle found solution
    if (status == l_False) {
        return l_False;
    } else if (status == l_True) {
        SolutionExtender extender(this, solution);
        extender.extend();
    }

    //Free threads
    for(vector<CommandControl*>::iterator it = threads.begin(), end = threads.end(); it != end; it++) {
        delete *it;
    }
    threads.clear();

    restPrinter->printEndSearchStat();

    return status;
}

/**
@brief The function that brings together almost all CNF-simplifications

It burst-searches for given number of conflicts, then it tries all sorts of
things like variable elimination, subsumption, failed literal probing, etc.
to try to simplifcy the problem at hand.
*/
lbool ThreadControl::simplifyProblem(const uint64_t numConfls)
{
    assert(ok);
    testAllClauseAttach();

    reArrangeClauses();

    if (conf.verbosity >= 3)
        std::cout << "c Simplifying problem for "
        << std::setw(8) << numConfls << " confls"
        << std::endl;

    lbool status = l_Undef;

    /*
    conf.random_var_freq = 1;
    simplifying = true;
    uint64_t origConflicts = conflicts;
    restPrinter->printRestartStat("S");
    while(status == l_Undef && conflicts-origConflicts < numConfls && needToInterrupt == false) {
        status = search(SearchFuncParams(100, std::numeric_limits<uint64_t>::max(), false));
    }
    if (needToInterrupt) return l_Undef;*/

    //restPrinter->printRestartStat("S");
    if (status != l_Undef) goto end;

    clAllocator->consolidate(this, threads, true);
    if (conf.doFindEqLits && !sCCFinder->find2LongXors())
        goto end;

    if (conf.doReplace && !varReplacer->performReplace())
        goto end;

    if (conf.doFailedLit && !failedLitSearcher->search())
        goto end;

    if (conf.doFindEqLits && !sCCFinder->find2LongXors())
        goto end;

    if (conf.doReplace && !varReplacer->performReplace())
        goto end;

    if (needToInterrupt) return l_Undef;

    //Vivify clauses
    if (conf.doClausVivif && !clauseVivifier->vivify())
        goto end;

    //Var-elim, gates, subsumption, strengthening
    if (conf.doSatELite && !subsumer->simplifyBySubsumption())
        goto end;

    //Search & replace 2-long XORs
    if (conf.doFindEqLits && !sCCFinder->find2LongXors())
        goto end;

    if (conf.doReplace && !varReplacer->performReplace())
        goto end;

    //Cleaning, stat counting, etc.
    if (conf.doCache && conf.doCalcReach)
        calcReachability();
    if (conf.doCache)
        implCache.clean(this);
    if (conf.doSortWatched)
        sortWatched();

    //addSymmBreakClauses();

end:
    if (conf.verbosity >= 3)
        std::cout << "c Simplifying finished" << std::endl;

    testAllClauseAttach();
    checkNoWrongAttach();

    if (!ok) return l_False;
    return status;
}

void ThreadControl::calcReachability()
{
    double myTime = cpuTime();

    for (uint32_t i = 0; i < nVars()*2; i++) {
        litReachable[i] = LitReachData();
    }

    for (size_t var = 0; var < decision_var.size(); var++) {
        if (value(var) != l_Undef
            || varData[var].elimed != ELIMED_NONE
            || !decision_var[var]
        ) continue;

        for (uint32_t sig1 = 0; sig1 < 2; sig1++)  {
            const Lit lit = Lit(var, sig1);

            vector<LitExtra>& cache = implCache[(~lit).toInt()].lits;
            uint32_t cacheSize = cache.size();
            for (vector<LitExtra>::const_iterator it = cache.begin(), end = cache.end(); it != end; it++) {
                if (it->getLit() == lit || it->getLit() == ~lit) continue;

                if (litReachable[it->getLit().toInt()].lit == lit_Undef
                    || litReachable[it->getLit().toInt()].numInCache </*=*/ cacheSize
                ) {
                    //if (litReachable[it->toInt()].numInCache == cacheSize && mtrand.randInt(1) == 0) continue;
                    litReachable[it->getLit().toInt()].lit = lit;
                    litReachable[it->getLit().toInt()].numInCache = cacheSize;
                }
            }
        }
    }

    if (conf.verbosity >= 1) {
        std::cout << "c calculated reachability. Time: " << (cpuTime() - myTime) << std::endl;
    }
}

Clause* ThreadControl::newClauseByThread(const vector<Lit>& lits, const uint32_t glue, uint64_t& thisSumConflicts)
{
    Clause* cl = NULL;
    switch (lits.size()) {
        case 1:
            unitLearntsToAdd.push_back(lits[0]);
            break;
        case 2:
            binLearntsToAdd.push_back(BinaryClause(lits[0], lits[1], false));
            break;
        default:
            cl = clAllocator->Clause_new(lits);
            cl->makeLearnt(glue);
            longLearntsToAdd.push_back(cl);
            break;
    }
    sumConflicts++;
    thisSumConflicts = sumConflicts;

    return cl;
}

void ThreadControl::moveReduce()
{
    assert(toDetach.empty());
    moveClausesHere();
    reduceDB();
    nextCleanLimit += nextCleanLimitInc;
    nextCleanLimitInc *= 1.2;
}

void ThreadControl::consolidateMem()
{
    clAllocator->consolidate(this, threads, true);
}

void ThreadControl::printStats()
{
    double cpu_time = cpuTime();
    printStatsLine("c filedLit time"
                    , getTotalTimeFailedLitSearcher()
                    , getTotalTimeFailedLitSearcher()/cpu_time*100.0
                    , "% time");

    //Subsumer stats
    printStatsLine("c v-elimed"
                    , getNumElimSubsume()
                    , (double)getNumElimSubsume()/(double)nVars()*100.0
                    , "% vars");

    printStatsLine("c SatELite time"
                    , getTotalTimeSubsumer()
                    , getTotalTimeSubsumer()/cpu_time*100.0
                    , "% time");

    //VarReplacer stats
    printStatsLine("c binxor tree roots", getNumXorTrees());
    printStatsLine("c binxor trees' crown"
                    , getNumXorTreesCrownSize()
                    , (double)getNumXorTreesCrownSize()/(double)getNumXorTrees()
                    , "leafs/tree");

    printStatsLine("c SCC time", getTotalTimeSCC());
    printStatsLine("c Conflicts", sumConflicts, (double)sumConflicts/cpu_time, "conf/sec");
    printStatsLine("c Total time", cpu_time);
}

template<class T, class T2>
void ThreadControl::printStatsLine(std::string left, T value, T2 value2, std::string extra)
{
    std::cout << std::fixed << std::left << std::setw(27) << left << ": " << std::setw(11) << std::setprecision(2) << value << " (" << std::left << std::setw(9) << std::setprecision(2) << value2 << " " << extra << ")" << std::endl;
}

template<class T>
void ThreadControl::printStatsLine(std::string left, T value, std::string extra)
{
    std::cout << std::fixed << std::left << std::setw(27) << left << ": " << std::setw(11) << std::setprecision(2) << value << extra << std::endl;
}

void ThreadControl::dumpBinClauses(const bool alsoLearnt, const bool alsoNonLearnt, std::ostream& outfile) const
{
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator it = watches.begin(), end = watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary() && lit < it2->getOtherLit()) {
                bool toDump = false;
                if (it2->getLearnt() && alsoLearnt) toDump = true;
                if (!it2->getLearnt() && alsoNonLearnt) toDump = true;

                if (toDump)
                    outfile << it2->getOtherLit() << " " << lit << " 0" << std::endl;
            }
        }
    }
}

void ThreadControl::calcClauseDistrib()
{
    size_t size3 = 0;
    size_t size4 = 0;
    size_t size5 = 0;
    size_t sizeLarge = 0;
    for(vector<Clause*>::const_iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        switch((*it)->size()) {
            case 0:
            case 1:
            case 2:
                assert(false);
                break;
            case 3:
                size3++;
                break;
            case 4:
                size4++;
                break;
            case 5:
                size5++;
                break;
            default:
                sizeLarge++;
                break;
        }
    }

    for(vector<Clause*>::const_iterator it = learnts.begin(), end = learnts.end(); it != end; it++) {
        switch((*it)->size()) {
            case 0:
            case 1:
            case 2:
                assert(false);
                break;
            case 3:
                size3++;
                break;
            case 4:
                size4++;
                break;
            case 5:
                size5++;
                break;
            default:
                sizeLarge++;
                break;
        }
    }

    std::cout << "c size3: " << size3
    << " size4: " << size4
    << " size5: " << size5
    << " larger: " << sizeLarge << std::endl;
}

void ThreadControl::dumpSortedLearnts(std::ostream& os, const uint32_t maxSize)
{
    os
    << "c " << std::endl
    << "c ---------" << std::endl
    << "c unitaries" << std::endl
    << "c ---------" << std::endl;
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++) {
        os << trail[i] << " 0" << std::endl;    }


    os
    << "c " << std::endl
    << "c ---------------------------------" << std::endl
    << "c learnt binary clauses (extracted from watchlists)" << std::endl
    << "c ---------------------------------" << std::endl;
    if (maxSize >= 2) dumpBinClauses(true, false, os);

    os
    << "c " << std::endl
    << "c ---------------------------------------" << std::endl
    << "c clauses representing 2-long XOR clauses" << std::endl
    << "c ---------------------------------------" << std::endl;
    if (maxSize >= 2) {
        const vector<Lit>& table = varReplacer->getReplaceTable();
        for (Var var = 0; var != table.size(); var++) {
            Lit lit = table[var];
            if (lit.var() == var)
                continue;

            os << (~lit) << " " << Lit(var, false) << " 0" << std::endl;
            os << lit << " " << Lit(var, true) << " 0" << std::endl;
        }
    }

    os
    << "c " << std::endl
    << "c --------------------" << std::endl
    << "c clauses from learnts" << std::endl
    << "c --------------------" << std::endl;
    std::sort(learnts.begin(), learnts.begin()+learnts.size(), reduceDBStruct());
    for (int i = learnts.size()-1; i >= 0 ; i--) {
        Clause& cl = *learnts[i];
        if (cl.size() <= maxSize) {
            os << cl << " 0" << std::endl;
            os << "c clause learnt " << (cl.learnt() ? "yes" : "no") << " glue "  << cl.getGlue() << std::endl;
        }
    }
}

void ThreadControl::dumpOrigClauses(std::ostream& os) const
{
    uint32_t numClauses = 0;
    //unitary clauses
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++)
        numClauses++;

    //binary XOR clauses
    const vector<Lit>& table = varReplacer->getReplaceTable();
    for (Var var = 0; var != table.size(); var++) {
        Lit lit = table[var];
        if (lit.var() == var)
            continue;
        numClauses += 2;
    }

    //binary normal clauses
    numClauses += countNumBinClauses(false, true);

    //normal clauses
    numClauses += clauses.size();

    //previously eliminated clauses
    const vector<BlockedClause>& blockedClauses = subsumer->getBlockedClauses();
    numClauses += blockedClauses.size();

    os << "p cnf " << nVars() << " " << numClauses << std::endl;

    ////////////////////////////////////////////////////////////////////

    os
    << "c " << std::endl
    << "c ---------" << std::endl
    << "c unitaries" << std::endl
    << "c ---------" << std::endl;
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++) {
        os << trail[i] << " 0" << std::endl;
    }

    os
    << "c " << std::endl
    << "c ---------------------------------------" << std::endl
    << "c clauses representing 2-long XOR clauses" << std::endl
    << "c ---------------------------------------" << std::endl;
    for (Var var = 0; var != table.size(); var++) {
        Lit lit = table[var];
        if (lit.var() == var)
            continue;

        Lit litP1 = ~lit;
        Lit litP2 = Lit(var, false);
        os << litP1 << " " << litP2 << std::endl;
        os << ~litP1 << " " << ~litP2 << std::endl;
    }

    os
    << "c " << std::endl
    << "c ---------------" << std::endl
    << "c binary clauses" << std::endl
    << "c ---------------" << std::endl;
    dumpBinClauses(false, true, os);

    os
    << "c " << std::endl
    << "c ---------------" << std::endl
    << "c normal clauses" << std::endl
    << "c ---------------" << std::endl;
    for (vector<Clause*>::const_iterator i = clauses.begin(); i != clauses.end(); i++) {
        assert(!(*i)->learnt());
        os << (**i) << " 0" << std::endl;
    }

    os
    << "c " << std::endl
    << "c -------------------------------" << std::endl
    << "c previously eliminated variables" << std::endl
    << "c -------------------------------" << std::endl;
    for (vector<BlockedClause>::const_iterator it = blockedClauses.begin(); it != blockedClauses.end(); it++) {
        os << "c next clause is eliminated/blocked on lit " << it->blockedOn << std::endl;
        os << it->lits << " 0" << std::endl;
    }
}

void ThreadControl::printAllClauses() const
{
    for (uint32_t i = 0; i < clauses.size(); i++) {
        std::cout
                << "Normal clause num " << clAllocator->getOffset(clauses[i])
                << " cl: " << *clauses[i] << std::endl;
    }

    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator it = watches.begin(), end = watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        std::cout << "watches[" << lit << "]" << std::endl;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary()) {
                std::cout << "Binary clause part: " << lit << " , " << it2->getOtherLit() << std::endl;
            } else if (it2->isClause()) {
                std::cout << "Normal clause num " << it2->getNormOffset() << std::endl;
            } else if (it2->isTriClause()) {
                std::cout << "Tri clause:"
                << lit << " , "
                << it2->getOtherLit() << " , "
                << it2->getOtherLit2() << std::endl;
            }
        }
    }
}

bool ThreadControl::verifyBinClauses() const
{
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator it = watches.begin(), end = watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;

        for (vec<Watched>::const_iterator i = ws.begin(), end = ws.end() ; i != end; i++) {
            if (i->isBinary()
                && modelValue(lit) != l_True
                && modelValue(i->getOtherLit()) != l_True
            ) {
                std::cout << "bin clause: " << lit << " , " << i->getOtherLit() << " not satisfied!" << std::endl;
                std::cout << "value of unsat bin clause: " << value(lit) << " , " << value(i->getOtherLit()) << std::endl;
                return false;
            }
        }
    }

    return true;
}

bool ThreadControl::verifyClauses(const vector<Clause*>& cs) const
{
    #ifdef VERBOSE_DEBUG
    std::cout << "Checking clauses whether they have been properly satisfied." << std::endl;;
    #endif

    bool verificationOK = true;

    for (uint32_t i = 0; i != cs.size(); i++) {
        Clause& c = *cs[i];
        for (uint32_t j = 0; j < c.size(); j++)
            if (modelValue(c[j]) == l_True)
                goto next;

            std::cout << "unsatisfied clause: " << *cs[i] << std::endl;
        verificationOK = false;
        next:
                ;
    }

    return verificationOK;
}

bool ThreadControl::verifyModel() const
{
    bool verificationOK = true;
    verificationOK &= verifyClauses(clauses);
    verificationOK &= verifyClauses(learnts);
    verificationOK &= verifyBinClauses();

    if (conf.verbosity >= 1 && verificationOK) {
        std::cout
        << "c Verified " <<  clauses.size() << " clauses."
        << std::endl;
    }

    return verificationOK;
}


void ThreadControl::checkLiteralCount() const
{
    // Check that sizes are calculated correctly:
    uint64_t cnt = 0;
    for (uint32_t i = 0; i != clauses.size(); i++)
        cnt += clauses[i]->size();

    if (clausesLits != cnt) {
        std::cout << "c ERROR! literal count: " << clausesLits << " , real value = " <<  cnt << std::endl;
        assert(clausesLits == cnt);
    }
}

uint32_t ThreadControl::getNumElimSubsume() const
{
    return subsumer->getNumElimed();
}

uint32_t ThreadControl::getNumXorTrees() const
{
    return varReplacer->getNumTrees();
}

uint32_t ThreadControl::getNumXorTreesCrownSize() const
{
    return varReplacer->getNumReplacedVars();
}

double ThreadControl::getTotalTimeSubsumer() const
{
    return subsumer->getTotalTime();
}

double ThreadControl::getTotalTimeFailedLitSearcher() const
{
    return failedLitSearcher->getTotalTime();
}

double ThreadControl::getTotalTimeSCC() const
{
    return  sCCFinder->getTotalTime();
}

uint32_t ThreadControl::getNumUnsetVars() const
{
    assert(decisionLevel() == 0);
    return (decision_var.size() - trail.size());
}

uint32_t ThreadControl::getNumDecisionVars() const
{
    return numDecisionVars;
}

uint64_t ThreadControl::getNumTotalConflicts() const
{
    return sumConflicts;
}

void ThreadControl::setNeedToInterrupt()
{
    for(vector<CommandControl*>::iterator it = threads.begin(), end = threads.end(); it != end; it++) {
        (*it)->setNeedToInterrupt();
    }

    needToInterrupt = true;
}

lbool ThreadControl::modelValue (const Lit p) const
{
    return model[p.var()] ^ p.sign();
}

void ThreadControl::testAllClauseAttach() const
{
#ifndef DEBUG_ATTACH_MORE
    return;
#endif

    for (vector<Clause*>::const_iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        assert(normClauseIsAttached(**it));
    }
}

bool ThreadControl::normClauseIsAttached(const Clause& c) const
{
    bool attached = true;
    assert(c.size() > 2);

    ClauseOffset offset = clAllocator->getOffset(&c);
    if (c.size() == 3) {
        //The clause might have been longer, and has only recently
        //became 3-long. Check, and detach accordingly
        if (findWCl(watches[(~c[0]).toInt()], offset)) goto fullClause;

        Lit lit1 = c[0];
        Lit lit2 = c[1];
        Lit lit3 = c[2];
        attached &= findWTri(watches[(~lit1).toInt()], lit2, lit3);
        attached &= findWTri(watches[(~lit2).toInt()], lit1, lit3);
        attached &= findWTri(watches[(~lit3).toInt()], lit1, lit2);
    } else {
        fullClause:
        const ClauseData& data = clauseData[c.getNum()];
        attached &= findWCl(watches[(~c[data[0]]).toInt()], offset);
        attached &= findWCl(watches[(~c[data[1]]).toInt()], offset);
    }

    return attached;
}

void ThreadControl::findAllAttach() const
{
    for (uint32_t i = 0; i < watches.size(); i++) {
        const Lit lit = ~Lit::toLit(i);
        for (uint32_t i2 = 0; i2 < watches[i].size(); i2++) {
            const Watched& w = watches[i][i2];
            if (!w.isClause())
                continue;

            //Get clause
            Clause* cl = clAllocator->getPointer(w.getNormOffset());
            assert(!cl->getFreed());
            std::cout << (*cl) << std::endl;

            //Assert clauseData correctness
            const ClauseData& clData = clauseData[cl->getNum()];
            const bool watchNum = w.getWatchNum();
            std::cout << "Watchnum: " << watchNum << " clData watchnum: " << (clData[watchNum]) << " value: " << ((*cl)[clData[watchNum]]) << std::endl;
            if (((*cl)[clData[watchNum]]) != lit) {
                std::cout << "ERROR! ((*cl)[clData[watchNum]]) != lit !!" << std::endl;
            }

            //Clause in one of the lists
            if (!findClause(cl)) {
                std::cout << "ERROR! did not find clause!" << std::endl;
            }
        }
    }
}


bool ThreadControl::findClause(const Clause* c) const
{
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] == c) return true;
    }
    for (uint32_t i = 0; i < learnts.size(); i++) {
        if (learnts[i] == c) return true;
    }

    return false;
}

void ThreadControl::checkNoWrongAttach() const
{
    #ifndef VERBOSE_DEBUG
    return;
    #endif //VERBOSE_DEBUG

    for (vector<Clause*>::const_iterator i = learnts.begin(), end = learnts.end(); i != end; i++) {

        const Clause& cl = **i;
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (i > 0) assert(cl[i-1].var() != cl[i].var());
        }
    }
}

uint32_t ThreadControl::getNumFreeVars() const
{
    uint32_t freeVars = nVars();

    //Variables set
    freeVars -= trail.size();

    //Variables elimed
    Var var = 0;
    for(vector<VarData>::const_iterator it = varData.begin(), end = varData.end(); it != end; it++, var++) {
        if (value(var) == l_Undef
            && it->elimed != ELIMED_NONE)
        {
            freeVars--;
        }
    }

    return freeVars;
}

uint32_t ThreadControl::getNewToReplaceVars() const
{
    return varReplacer->getNewToReplaceVars();
}
