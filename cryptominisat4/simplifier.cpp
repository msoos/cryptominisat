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

#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <set>
#include <algorithm>
#include <fstream>
#include <set>
#include <iostream>
#include <limits>
#include <cmath>
#include <functional>


#include "simplifier.h"
#include "clause.h"
#include "solver.h"
#include "clausecleaner.h"
#include "constants.h"
#include "solutionextender.h"
#include "gatefinder.h"
#include "varreplacer.h"
#include "varupdatehelper.h"
#include "completedetachreattacher.h"
#include "subsumestrengthen.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "xorfinderabst.h"
#include "subsumeimplicit.h"
#include "sqlstats.h"
#include "datasync.h"

#ifdef USE_M4RI
#include "xorfinder.h"
#endif

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#define VERBOSE_ORGATE_REPLACE
#define VERBOSE_ASYMTE
#define VERBOSE_GATE_REMOVAL
#define VERBOSE_XORGATE_MIX
#define VERBOSE_DEBUG_XOR_FINDER
#define VERBOSE_DEBUG_VARELIM
#endif

using namespace CMSat;
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG_VARELIM
//#define VERBOSE_DEBUG_XOR_FINDER
//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS
//#define VERBOSE_ORGATE_REPLACE
//#define VERBOSE_DEBUG_ASYMTE
//#define VERBOSE_GATE_REMOVAL
//#define VERBOSE_XORGATE_MIX

Simplifier::Simplifier(Solver* _solver):
    solver(_solver)
    , seen(solver->seen)
    , seen2(solver->seen2)
    , toClear(solver->toClear)
    , varElimOrder(VarOrderLt(varElimComplexity))
    , var_bva_order(VarBVAOrder(watch_irred_sizes))
    , xorFinder(NULL)
    , gateFinder(NULL)
    , anythingHasBeenBlocked(false)
    , blockedMapBuilt(false)
{
    xorFinder = new XorFinderAbst();
    #ifdef USE_M4RI
    if (solver->conf.doFindXors) {
        delete xorFinder;
        xorFinder = new XorFinder(this, solver);
    }
    #endif
    subsumeStrengthen = new SubsumeStrengthen(this, solver);

    if (solver->conf.doGateFind) {
        gateFinder = new GateFinder(this, solver);
    }
}

Simplifier::~Simplifier()
{
    delete xorFinder;
    delete subsumeStrengthen;
    delete gateFinder;
}

void Simplifier::check_delete_gatefinder()
{
    if (solver->conf.doGateFind
        && solver->nVars() > 10ULL*1000ULL*1000ULL
    ) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [simp] gate finder switched off due to"
            << " excessive number of variables (we may run out of memory)"
            << endl;
        }
        delete gateFinder;
        gateFinder = NULL;
        solver->conf.doGateFind = false;
    }
}

void Simplifier::new_var(const Var orig_outer)
{
    check_delete_gatefinder();

    if (solver->conf.doGateFind) {
        gateFinder->new_var(orig_outer);
    }
}

void Simplifier::new_vars(size_t n)
{
    check_delete_gatefinder();

    if (solver->conf.doGateFind) {
        gateFinder->new_vars(n);
    }
}

void Simplifier::saveVarMem()
{
    if (gateFinder)
        gateFinder->saveVarMem();
}

void Simplifier::print_blocked_clauses_reverse() const
{
    for(vector<BlockedClause>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; it++
    ) {
        if (it->dummy) {
            cout
            << "dummy blocked clause for literal " << it->blockedOn
            << endl;
        } else {
            cout
            << "blocked clause " << it->lits
            << " blocked on var "
            << solver->map_outer_to_inter(it->blockedOn.var()) + 1
            << endl;
        }
    }
}

void Simplifier::extendModel(SolutionExtender* extender)
{
    //Either a variable is not eliminated, or its value is undef
    for(size_t i = 0; i < solver->nVarsOuter(); i++) {
        const Var outer = solver->map_inter_to_outer(i);
        assert(solver->varData[i].removed != Removed::elimed
            || (solver->value(i) == l_Undef && solver->model[outer] == l_Undef)
        );
    }

    cleanBlockedClauses();
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "Number of blocked clauses:" << blockedClauses.size() << endl;
    print_blocked_clauses_reverse();
    #endif

    //go through in reverse order
    for (vector<BlockedClause>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; it++
    ) {
        if (it->dummy) {
            extender->dummyBlocked(it->blockedOn);
        } else {
            extender->addClause(it->lits, it->blockedOn);
        }
    }
}

void Simplifier::unlinkClause(
    const ClOffset offset
    , bool doDrup
    , bool allow_empty_watch
    , bool only_set_is_removed
) {
    Clause& cl = *solver->clAllocator.getPointer(offset);
    if (solver->drup->enabled() && doDrup) {
       (*solver->drup) << del << cl << fin;
    }

    if (!only_set_is_removed) {
        for (const Lit lit: cl) {
            if (!(allow_empty_watch && solver->watches[lit.toInt()].empty())) {
                *limit_to_decrease -= 2*(long)solver->watches[lit.toInt()].size();
                removeWCl(solver->watches[lit.toInt()], offset);
            }
        }
    } else {
        cl.setRemoved();
    }

    if (!cl.red()) {
        for (const Lit lit: cl) {
            touched.touch(lit);
        }
    }

    if (cl.red()) {
        solver->litStats.redLits -= cl.size();
    } else {
        solver->litStats.irredLits -= cl.size();
    }

    if (!only_set_is_removed) {
        solver->clAllocator.clauseFree(&cl);
    } else {
        cl_to_free_later.push_back(offset);
    }
}

lbool Simplifier::cleanClause(ClOffset offset)
{
    assert(!solver->drup->something_delayed());
    assert(solver->ok);

    bool satisfied = false;
    Clause& cl = *solver->clAllocator.getPointer(offset);
    (*solver->drup) << deldelay << cl << fin;
    #ifdef VERBOSE_DEBUG
    cout << "Clause to clean: " << cl << endl;
    for(size_t i = 0; i < cl.size(); i++) {
        cout << cl[i] << " : "  << solver->value(cl[i]) << " , ";
    }
    cout << endl;
    #endif

    Lit* i = cl.begin();
    Lit* j = cl.begin();
    const Lit* end = cl.end();
    *limit_to_decrease -= (long)cl.size();
    for(; i != end; i++) {
        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (solver->value(*i) == l_True)
            satisfied = true;

        if (solver->value(*i) == l_True
            || solver->value(*i) == l_False
        ) {
            removeWCl(solver->watches[i->toInt()], offset);
        }
    }
    cl.shrink(i-j);

    if (satisfied) {
        #ifdef VERBOSE_DEBUG
        cout << "Clause cleaning -- satisfied, removing" << endl;
        #endif
        (*solver->drup) << findelay;
        unlinkClause(offset, false);
        return l_True;
    }

    //Update lits stat
    if (cl.red())
        solver->litStats.redLits -= i-j;
    else
        solver->litStats.irredLits -= i-j;

    if (solver->conf.verbosity >= 6 || bva_verbosity) {
        cout << "-> Clause became after cleaning:" << cl << endl;
    }

    if (i-j > 0) {
        (*solver->drup) << cl << fin << findelay;
    } else {
        solver->drup->forget_delay();
    }

    switch(cl.size()) {
        case 0:
            unlinkClause(offset, false);
            solver->ok = false;
            return l_False;

        case 1:
            solver->enqueue(cl[0]);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
            unlinkClause(offset, false);
            return l_True;

        case 2:
            solver->attachBinClause(cl[0], cl[1], cl.red());
            unlinkClause(offset, false);
            return l_True;

        case 3:
            solver->attachTriClause(cl[0], cl[1], cl[2], cl.red());
            unlinkClause(offset, false);
            return l_True;

        default:
            cl.setStrenghtened();
            return l_Undef;
    }
}

uint64_t Simplifier::calc_mem_usage_of_occur(const vector<ClOffset>& toAdd) const
{
     uint64_t memUsage = 0;
    for (vector<ClOffset>::const_iterator
        it = toAdd.begin(), end = toAdd.end()
        ; it !=  end
        ; it++
    ) {
        Clause* cl = solver->clAllocator.getPointer(*it);
        //*2 because of the overhead of allocation
        memUsage += cl->size()*sizeof(Watched)*2;
    }

    //Estimate malloc overhead
    memUsage += solver->numActiveVars()*2*40;

    return memUsage;
}

void Simplifier::print_mem_usage_of_occur(bool irred, uint64_t memUsage) const
{
    if (solver->conf.verbosity >= 2) {
        cout
        << "c [simp] mem usage for occur of "
        << (irred ?  "irred" : "red  ")
        << " " << std::setw(6) << memUsage/(1024ULL*1024ULL) << " MB"
        << endl;
    }
}

void Simplifier::print_linkin_data(const LinkInData link_in_data) const
{
    if (solver->conf.verbosity < 2)
        return;

    double val;
    if (link_in_data.cl_linked + link_in_data.cl_not_linked == 0) {
        val = 0;
    } else {
        val = (double)link_in_data.cl_not_linked/(double)(link_in_data.cl_linked+link_in_data.cl_not_linked)*100.0;
    }

    cout
    << "c [simp] Not linked in red "
    << link_in_data.cl_not_linked << "/"
    << (link_in_data.cl_linked + link_in_data.cl_not_linked)
    << " ("
    << std::setprecision(2) << std::fixed
    << val
    << " %)"
    << endl;
}


Simplifier::LinkInData Simplifier::link_in_clauses(
    const vector<ClOffset>& toAdd
    , bool irred
    , bool alsoOccur
) {
    LinkInData link_in_data;
    uint64_t linkedInLits = 0;
    for (vector<ClOffset>::const_iterator
        it = toAdd.begin(), end = toAdd.end()
        ; it !=  end
        ; it++
    ) {
        Clause* cl = solver->clAllocator.getPointer(*it);

        //Sanity check that the value given as irred is correct
        assert(
            (irred && !cl->red())
            || (!irred && cl->red())
        );

        if (alsoOccur
            //If irreduntant or (small enough AND link in limit not reached)
            && (irred
                || (cl->size() < solver->conf.maxRedLinkInSize
                    && linkedInLits < (solver->conf.maxOccurRedLitLinkedM*1000ULL*1000ULL))
            )
        ) {
            linkInClause(*cl);
            link_in_data.cl_linked++;
            linkedInLits += cl->size();
        } else {
            assert(cl->red());
            cl->setOccurLinked(false);
            link_in_data.cl_not_linked++;
        }

        clauses.push_back(*it);
    }
    clause_lits_added += linkedInLits;

    return link_in_data;
}

bool Simplifier::decide_occur_limit(bool irred, uint64_t memUsage)
{
    //over + irred -> exit
    if (irred
        && memUsage/(1024ULL*1024ULL) >= solver->conf.maxOccurIrredMB
    ) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [simp] Not linking in irred due to excessive expected memory usage"
            << endl;
        }
        return false;
    }

    //over + red -> don't link
    if (!irred
        && memUsage/(1024ULL*1024ULL) >= solver->conf.maxOccurRedMB
    ) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [simp] Not linking in red due to excessive expected memory usage"
            << endl;
        }

        return false;
    }

    return true;
}

bool Simplifier::addFromSolver(
    vector<ClOffset>& toAdd
    , bool alsoOccur
    , bool irred
) {
    //solver->printWatchMemUsed();

    if (alsoOccur) {
        uint64_t memUsage = calc_mem_usage_of_occur(toAdd);
        print_mem_usage_of_occur(irred, memUsage);
        alsoOccur = decide_occur_limit(irred, memUsage);
        if (irred && !alsoOccur)
            return false;
    }

    if (!irred && alsoOccur) {
        std::sort(toAdd.begin(), toAdd.end(), ClauseSizeSorter(solver->clAllocator));
    }

    LinkInData link_in_data = link_in_clauses(toAdd, irred, alsoOccur);
    toAdd.clear();
    if (!irred)
        print_linkin_data(link_in_data);

    return true;
}

bool Simplifier::check_varelim_when_adding_back_cl(const Clause* cl) const
{
    bool notLinkedNeedFree = false;
    for (Clause::const_iterator
        it2 = cl->begin(), end2 = cl->end()
        ; it2 != end2
        ; it2++
    ) {
        //The clause was too long, and wasn't linked in
        //but has been var-elimed, so remove it
        if (!cl->getOccurLinked()
            && solver->varData[it2->var()].removed == Removed::elimed
        ) {
            notLinkedNeedFree = true;
        }

        if (cl->getOccurLinked()
            && solver->varData[it2->var()].removed != Removed::none
            && solver->varData[it2->var()].removed != Removed::queued_replacer
        ) {
            cout
            << "ERROR! Clause " << *cl
            << " red: " << cl->red()
            << " contains lit " << *it2
            << " which has removed status"
            << removed_type_to_string(solver->varData[it2->var()].removed)
            << endl;

            assert(false);
            std::exit(-1);
        }
    }

    return notLinkedNeedFree;
}

void Simplifier::addBackToSolver()
{
    for (vector<ClOffset>::const_iterator
        it = clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = solver->clAllocator.getPointer(*it);
        if (cl->getFreed())
            continue;

        assert(!cl->getRemoved());

        //All clauses are larger than 2-long
        assert(cl->size() > 3);

        bool notLinkedNeedFree = check_varelim_when_adding_back_cl(cl);
        if (notLinkedNeedFree) {
            //The clause wasn't linked in but needs removal now
            if (cl->red()) {
                solver->litStats.redLits -= cl->size();
            } else {
                solver->litStats.irredLits -= cl->size();
            }
            solver->clAllocator.clauseFree(cl);
            continue;
        }

        if (completeCleanClause(*cl)) {
            solver->attachClause(*cl);
            if (cl->red()) {
                solver->longRedCls.push_back(*it);
            } else {
                solver->longIrredCls.push_back(*it);
            }
        } else {
            solver->clAllocator.clauseFree(cl);
        }
    }
}

bool Simplifier::completeCleanClause(Clause& cl)
{
    assert(!solver->drup->something_delayed());
    assert(cl.size() > 3);
    (*solver->drup) << deldelay << cl << fin;

    //Remove all lits from stats
    //we will re-attach the clause either way
    if (cl.red()) {
        solver->litStats.redLits -= cl.size();
    } else {
        solver->litStats.irredLits -= cl.size();
    }

    Lit *i = cl.begin();
    Lit *j = i;
    for (Lit *end = cl.end(); i != end; i++) {
        if (solver->value(*i) == l_True) {

            (*solver->drup) << findelay;
            return false;
        }

        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    cl.shrink(i-j);

    //Drup
    if (i - j > 0) {
        (*solver->drup) << cl << fin << findelay;
    } else {
        solver->drup->forget_delay();
    }

    switch (cl.size()) {
        case 0:
            solver->ok = false;
            return false;

        case 1:
            solver->enqueue(cl[0]);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
            return false;

        case 2:
            solver->attachBinClause(cl[0], cl[1], cl.red());
            return false;

        case 3:
            solver->attachTriClause(cl[0], cl[1], cl[2], cl.red());
            return false;

        default:
            return true;
    }

    return true;
}

void Simplifier::removeAllLongsFromWatches()
{
    for (watch_array::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; ++it
    ) {
        watch_subarray ws = *it;

        watch_subarray::iterator i = ws.begin();
        watch_subarray::iterator j = i;
        for (watch_subarray::iterator end2 = ws.end(); i != end2; i++) {
            if (i->isClause()) {
                continue;
            } else {
                assert(i->isBinary() || i->isTri());
                *j++ = *i;
            }
        }
        ws.shrink(i - j);
    }
}

void Simplifier::eliminate_empty_resolvent_vars()
{
    uint32_t var_elimed = 0;
    double myTime = cpuTime();
    const int64_t orig_empty_varelim_time_limit = empty_varelim_time_limit;
    limit_to_decrease = &empty_varelim_time_limit;
    cl_to_free_later.clear();

    size_t num = 0;
    for(size_t var = solver->mtrand.randInt(solver->nVars())
        ; num < solver->nVars()
        && var < solver->nVars()
        && *limit_to_decrease > 0
        ; var = (var + 1) % solver->nVars(), num++
    ) {
        if (!can_eliminate_var(var))
            continue;

        const Lit lit = Lit(var, false);
        if (!checkEmptyResolvent(lit))
            continue;

        create_dummy_blocked_clause(lit);
        rem_cls_from_watch_due_to_varelim(solver->watches[lit.toInt()], lit);
        rem_cls_from_watch_due_to_varelim(solver->watches[(~lit).toInt()], ~lit);
        set_var_as_eliminated(var, lit);
        var_elimed++;
    }

    if (!cl_to_free_later.empty()) {
        clean_occur_from_removed_clauses();
        free_clauses_to_free();
    }
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain = (double)*limit_to_decrease/(double)orig_empty_varelim_time_limit;
    if (solver->conf.verbosity >= 2) {
        cout
        << "c Empty resolvent elimed: " << var_elimed
        << " T:" << time_used
        << " T-out: " << (time_out ? "Y" : "N")
        << endl;
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "empty resolvent"
            , time_used
            , time_out
            , time_remain
        );
    }
}

bool Simplifier::can_eliminate_var(const Var var) const
{
    if (solver->value(var) != l_Undef
        || solver->varData[var].removed != Removed::none
        ||  solver->var_inside_assumptions(var)
    ) {
        return false;
    }

    return true;
}

bool Simplifier::eliminateVars()
{
    //Set-up
    double myTime = cpuTime();
    size_t vars_elimed = 0;
    size_t wenThrough = 0;
    time_spent_on_calc_otf_update = 0;
    num_otf_update_until_now = 0;
    limit_to_decrease = &norm_varelim_time_limit;
    cl_to_free_later.clear();

    order_vars_for_elim();
    if (solver->conf.verbosity >= 5) {
        cout << "c #order size:" << varElimOrder.size() << endl;
    }

    //Go through the ordered list of variables to eliminate
    while(!varElimOrder.empty()
        && *limit_to_decrease > 0
        && varelim_num_limit > 0
        && !solver->must_interrupt_asap()
    ) {
        assert(limit_to_decrease == &norm_varelim_time_limit);
        Var var = varElimOrder.removeMin();

        //Stats
        *limit_to_decrease -= 20;
        wenThrough++;

        //Print status
        if (solver->conf.verbosity >= 5
            && wenThrough % 200 == 0
        ) {
            cout << "toDecrease: " << *limit_to_decrease << endl;
        }

        if (!can_eliminate_var(var))
            continue;

        //Try to eliminate
        if (maybeEliminate(var)) {
            vars_elimed++;
            varelim_num_limit--;
        }
        if (!solver->ok)
            goto end;
    }

end:
    clean_occur_from_removed_clauses();
    free_clauses_to_free();
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain = (double)*limit_to_decrease/(double)norm_varelim_time_limit;

    if (solver->conf.verbosity >= 2) {
        cout
        << "c  #try to eliminate: " << wenThrough << endl
        << "c  #var-elim: " << vars_elimed << endl
        << "c  #T-o: " << (time_out ? "Y" : "N") << endl
        << "c  #T-r: " << std::fixed << std::setprecision(2) << (time_remain*100.0) << "%" << endl
        << "c  #T: " << time_used << endl;
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "bve"
            , time_used
            , time_out
            , time_remain
        );
    }

    assert(limit_to_decrease == &norm_varelim_time_limit);

    runStats.varElimTimeOut += (*limit_to_decrease <= 0);
    runStats.varElimTime += cpuTime() - myTime;

    return solver->ok;
}

void Simplifier::free_clauses_to_free()
{
    for(ClOffset off: cl_to_free_later) {
        Clause* cl = solver->clAllocator.getPointer(off);
        solver->clAllocator.clauseFree(cl);
    }
    cl_to_free_later.clear();
}

void Simplifier::clean_occur_from_removed_clauses()
{
    for(watch_subarray w: solver->watches) {
        size_t i = 0;
        size_t j = 0;
        size_t end = w.size();
        for(; i < end; i++) {
            const Watched ws = w[i];
            if (ws.isBinary() || ws.isTri()) {
                w[j++] = w[i];
                continue;
            }

            assert(ws.isClause());
            Clause* cl = solver->clAllocator.getPointer(ws.getOffset());
            if (!cl->getRemoved()) {
                w[j++] = w[i];
            }
        }
        w.shrink(i-j);
    }
}

void Simplifier::subsumeReds()
{
    double myTime = cpuTime();

    //Test & debug
    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    assert(solver->varReplacer->getNewToReplaceVars() == 0
            && "Cannot work in an environment when elimnated vars could be replaced by other vars");

    //If too many clauses, don't do it
    if (solver->getNumLongClauses() > 10000000UL
        || solver->litStats.irredLits > 50000000UL
    )  return;

    //Setup
    clause_lits_added = 0;
    runStats.clear();
    clauses.clear();
    limit_to_decrease = &strengthening_time_limit;
    size_t origTrailSize = solver->trail_size();

    //Remove all long clauses from watches
    removeAllLongsFromWatches();

    //Add red to occur
    runStats.origNumRedLongClauses = solver->longRedCls.size();
    addFromSolver(
        solver->longRedCls
        , true //try to add to occur
        , false //irreduntant?
    );
    solver->longRedCls.clear();
    runStats.origNumFreeVars = solver->getNumFreeVars();
    setLimits();

    //Print link-in and startup time
    const double linkInTime = cpuTime() - myTime;
    runStats.linkInTime += linkInTime;
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed_min(
            solver
            , "occur build"
            , linkInTime
        );
    }


    subsumeStrengthen->backward_subsumption_with_all_clauses();

    //Add irred to occur, but only temporarily
    runStats.origNumIrredLongClauses = solver->longIrredCls.size();
    addFromSolver(solver->longIrredCls
        , false //try to add to occur
        , true //irreduntant?
    );
    solver->longIrredCls.clear();

    //Add back clauses to solver etc
    finishUp(origTrailSize);

    if (solver->conf.verbosity >= 1) {
        subsumeStrengthen->getRunStats().printShort();
    }
}

void Simplifier::checkAllLinkedIn()
{
    for(vector<ClOffset>::const_iterator
        it = clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        Clause& cl = *solver->clAllocator.getPointer(*it);

        assert(cl.red() || cl.getOccurLinked());
        if (cl.freed() || cl.red())
            continue;

        for(size_t i = 0; i < cl.size(); i++) {
            Lit lit = cl[i];
            bool found = findWCl(solver->watches[lit.toInt()], *it);
            assert(found);
        }
    }
}

bool Simplifier::fill_occur_and_print_stats()
{
    double myTime = cpuTime();
    removeAllLongsFromWatches();
    if (!fill_occur()) {
        return false;
    }
    sanityCheckElimedVars();
    const double linkInTime = cpuTime() - myTime;
    runStats.linkInTime += linkInTime;
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed_min(
            solver
            , "occur build"
            , linkInTime
        );
    }

    //Print memory usage after occur link-in
    if (solver->conf.verbosity >= 2) {
        solver->printWatchMemUsed(memUsedTotal());
    }

    return true;
}


bool Simplifier::simplify()
{
    assert(solver->okay());
    assert(toClear.empty());

    //Test & debug
    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    assert(solver->varReplacer->getNewToReplaceVars() == 0
            && "Cannot work in an environment when elimnated vars could be replaced by other vars");

    //Clean the clauses before playing with them
    solver->clauseCleaner->removeAndCleanAll();

    //If too many clauses, don't do it
    if (solver->getNumLongClauses() > 10ULL*1000ULL*1000ULL
        || solver->litStats.irredLits > 50ULL*1000ULL*1000ULL
    ) {
        return solver->okay();
    }

    //Setup
    clause_lits_added = 0;
    runStats.clear();
    runStats.numCalls++;
    clauses.clear();
    limit_to_decrease = &strengthening_time_limit;
    if (!fill_occur_and_print_stats()) {
        return solver->okay();
    }

    setLimits();
    runStats.origNumFreeVars = solver->getNumFreeVars();
    const size_t origBlockedSize = blockedClauses.size();
    const size_t origTrailSize = solver->trail_size();

    //subsumeStrengthen->subsumeWithTris();
    subsumeStrengthen->backward_subsumption_with_all_clauses();
    if (!subsumeStrengthen->performStrengthening()
        || solver->must_interrupt_asap()
    ) {
        goto end;
    }

    #ifdef USE_M4RI
    if (solver->conf.doFindXors
        && xorFinder != NULL
        && !xorFinder->findXors()
    ) {
        goto end;
    }
    #endif

    if (!solver->propagate_occur()
        || solver->must_interrupt_asap()
    ) {
        goto end;
    }

    solver->clauseCleaner->clean_implicit_clauses();
    if (solver->conf.doVarElim && solver->conf.do_empty_varelim) {
        eliminate_empty_resolvent_vars();
        if (!eliminateVars()
            || solver->must_interrupt_asap()
        )
            goto end;
    }

    if (!solver->propagate_occur()
        || solver->must_interrupt_asap()
    ) {
        goto end;
    }

    if (!bounded_var_addition()
        || solver->must_interrupt_asap()
    ) {
        goto end;
    }

    if (solver->conf.doCache && solver->conf.doGateFind) {
        if (!gateFinder->doAll()
            || solver->must_interrupt_asap()
        ) {
            goto end;
        }
    }

end:

    remove_by_drup_recently_blocked_clauses(origBlockedSize);
    finishUp(origTrailSize);

    //Print stats
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort(solver->conf.doVarElim);
    }

    return solver->ok;
}

bool Simplifier::fill_occur()
{
    //Try to add irreducible to occur
    runStats.origNumIrredLongClauses = solver->longIrredCls.size();
    bool ret = addFromSolver(solver->longIrredCls
        , true //try to add to occur list
        , true //it is irred
    );

    //Memory limit reached, irreduntant clauses cannot
    //be added to occur --> exit
    if (!ret) {
        CompleteDetachReatacher detRet(solver);
        detRet.reattachLongs(true);
        return false;
    }

    //Add redundant to occur
    runStats.origNumRedLongClauses = solver->longRedCls.size();
    addFromSolver(solver->longRedCls
        , true //try to add to occur list
        , false //irreduntant?
    );

    return true;
}

bool Simplifier::unEliminate(Var var)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->okay());

    //Check that it was really eliminated
    assert(solver->varData[var].removed == Removed::elimed);
    assert(!solver->varData[var].is_decision);
    assert(solver->value(var) == l_Undef);

    if (!blockedMapBuilt) {
        cleanBlockedClauses();
        buildBlockedMap();
    }

    //Uneliminate it in theory
    globalStats.numVarsElimed--;
    solver->varData[var].removed = Removed::none;
    solver->setDecisionVar(var);
    if (solver->conf.doStamp) {
        solver->stamp.remove_from_stamps(var);
    }

    //Find if variable is really needed to be eliminated
    var = solver->map_inter_to_outer(var);
    map<Var, vector<size_t> >::iterator it = blk_var_to_cl.find(var);
    if (it == blk_var_to_cl.end())
        return solver->okay();

    //Eliminate it in practice
    //NOTE: Need to eliminate in theory first to avoid infinite loops
    for(size_t i = 0; i < it->second.size(); i++) {
        size_t at = it->second[i];

        //Mark for removal from blocked list
        blockedClauses[at].toRemove = true;
        assert(blockedClauses[at].blockedOn.var() == var);

        if (blockedClauses[at].dummy)
            continue;

        //Re-insert into Solver
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        cout
        << "Uneliminating cl " << blockedClauses[at].lits
        << " on var " << var+1
        << endl;
        #endif
        solver->addClause(blockedClauses[at].lits);
        if (!solver->okay())
            return false;
    }

    return solver->okay();
}

void Simplifier::remove_by_drup_recently_blocked_clauses(size_t origBlockedSize)
{
    if (!(*solver->drup).enabled())
        return;

    if (solver->conf.verbosity >= 6) {
        cout << "c Deleting blocked clauses for DRUP" << endl;
    }

    for(size_t i = origBlockedSize; i < blockedClauses.size(); i++) {
        if (blockedClauses[i].dummy)
            continue;

        //If doing stamping or caching, we cannot delete binary redundant
        //clauses, because they are stored in the stamp/cache and so
        //will be used -- and DRUP will complain when used
        if (blockedClauses[i].lits.size() <= 2
            && (solver->conf.doCache
                || solver->conf.doStamp)
        ) {
            continue;
        }

        (*solver->drup) << del;
        for(vector<Lit>::const_iterator
            it = blockedClauses[i].lits.begin(), end = blockedClauses[i].lits.end()
            ; it != end
            ; it++
        ) {
            (*solver->drup) << *it;
        }
        (*solver->drup) << fin;
    }
}

void Simplifier::buildBlockedMap()
{
    blk_var_to_cl.clear();
    for(size_t i = 0; i < blockedClauses.size(); i++) {
        const BlockedClause& blocked = blockedClauses[i];
        map<Var, vector<size_t> >::iterator it
            = blk_var_to_cl.find(blocked.blockedOn.var());

        if (it == blk_var_to_cl.end()) {
            vector<size_t> tmp;
            tmp.push_back(i);
            blk_var_to_cl[blocked.blockedOn.var()] = tmp;
        } else {
            it->second.push_back(i);
        }
    }
    blockedMapBuilt = true;
}

void Simplifier::finishUp(
    size_t origTrailSize
) {
    bool somethingSet = (solver->trail_size() - origTrailSize) > 0;
    runStats.zeroDepthAssings = solver->trail_size() - origTrailSize;
    const double myTime = cpuTime();

    //Add back clauses to solver
    solver->propagate_occur();
    removeAllLongsFromWatches();
    addBackToSolver();
    solver->propagate_occur();
    if (solver->ok) {
        solver->clauseCleaner->removeAndCleanAll();
    }

    //Update global stats
    const double time_used = cpuTime() - myTime;
    runStats.finalCleanupTime += time_used;
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed_min(
            solver
            , "occur cleanup"
            , time_used
        );
    }
    globalStats += runStats;
    subsumeStrengthen->finishedRun();

    //Sanity checks
    if (solver->ok && somethingSet) {
        solver->testAllClauseAttach();
        solver->checkNoWrongAttach();
        solver->check_stats();
        solver->checkImplicitPropagated();
    }

    if (solver->ok) {
        checkElimedUnassignedAndStats();
    }
}

void Simplifier::sanityCheckElimedVars()
{
    //First, sanity-check the long clauses
    for (vector<ClOffset>::const_iterator
        it =  clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        const Clause* cl = solver->clAllocator.getPointer(*it);

        //Already removed
        if (cl->getFreed())
            continue;

        for (const Lit lit: *cl) {
            if (solver->varData[lit.var()].removed == Removed::elimed) {
                cout
                << "Error: elimed var -- Lit " << lit << " in clause"
                << endl
                << "wrongly left in clause: " << *cl
                << endl;
                std::exit(-1);
            }
        }
    }

    //Then, sanity-check the binary clauses
    size_t wsLit = 0;
    for (watch_array::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBinary()) {
                if (solver->varData[lit.var()].removed == Removed::elimed
                        || solver->varData[it2->lit2().var()].removed == Removed::elimed
                ) {
                    cout
                    << "Error: A var is elimed in a binary clause: "
                    << lit << " , " << it2->lit2()
                    << endl;
                    std::exit(-1);
                }
            }
        }
    }
}

void Simplifier::setLimits()
{
    subsumption_time_limit     = 850LL*1000LL*1000LL;
    strengthening_time_limit   = 400LL*1000LL*1000LL;
//     numMaxTriSub      = 600LL*1000LL*1000LL;
    norm_varelim_time_limit    = 4ULL*1000LL*1000LL*1000LL;
    empty_varelim_time_limit   = 200LL*1000LL*1000LL;
    aggressive_elim_time_limit = 300LL *1000LL*1000LL;
    bounded_var_elim_time_limit= 400LL *1000LL*1000LL;

    //numMaxElim = 0;
    //numMaxElim = std::numeric_limits<int64_t>::max();

    //If variable elimination isn't going so well
    if (globalStats.testedToElimVars > 0
        && (double)globalStats.numVarsElimed/(double)globalStats.testedToElimVars < 0.1
    ) {
        norm_varelim_time_limit /= 2;
    }

    #ifdef BIT_MORE_VERBOSITY
    cout << "c addedClauseLits: " << clause_lits_added_limit << endl;
    #endif
    if (clause_lits_added < 10ULL*1000ULL*1000ULL) {
        norm_varelim_time_limit *= 2;
        empty_varelim_time_limit *= 2;
        subsumption_time_limit *= 2;
        strengthening_time_limit *= 2;
        bounded_var_elim_time_limit *= 2;
    }

    if (clause_lits_added < 3ULL*1000ULL*1000ULL) {
        norm_varelim_time_limit *= 2;
        empty_varelim_time_limit *= 2;
        subsumption_time_limit *= 2;
        strengthening_time_limit *= 2;
    }

    varelim_num_limit = ((double)solver->getNumFreeVars() * solver->conf.varElimRatioPerIter);
    if (globalStats.numCalls > 0) {
        varelim_num_limit = (double)varelim_num_limit * (globalStats.numCalls+0.5);
    }
    runStats.origNumMaxElimVars = varelim_num_limit;

    if (!solver->conf.doStrengthen) {
        strengthening_time_limit = 0;
    }

    //For debugging

    //numMaxSubsume0 = 0;
    //numMaxSubsume1 = 0;
    //numMaxElimVars = 0;
    //numMaxElim = 0;
    //numMaxSubsume0 = std::numeric_limits<int64_t>::max();
    //numMaxSubsume1 = std::numeric_limits<int64_t>::max();
    //numMaxElimVars = std::numeric_limits<int32_t>::max();
    //numMaxElim     = std::numeric_limits<int64_t>::max();
}

void Simplifier::cleanBlockedClauses()
{
    assert(solver->decisionLevel() == 0);
    vector<BlockedClause>::iterator i = blockedClauses.begin();
    vector<BlockedClause>::iterator j = blockedClauses.begin();
    size_t at = 0;

    for (vector<BlockedClause>::iterator
        end = blockedClauses.end()
        ; i != end
        ; i++, at++
    ) {
        const Var blockedOn = solver->map_outer_to_inter(i->blockedOn.var());
        if (solver->varData[blockedOn].removed == Removed::elimed
            && solver->value(blockedOn) != l_Undef
        ) {
            cout
            << "ERROR: lit " << *i << " elimed,"
            << " value: " << solver->value(blockedOn)
            << endl;
            assert(false);
            std::exit(-1);
        }

        if (blockedClauses[at].toRemove) {
            blockedMapBuilt = false;
        } else {
            assert(solver->varData[blockedOn].removed == Removed::elimed);
            *j++ = *i;
        }
    }
    blockedClauses.resize(blockedClauses.size()-(i-j));
}

size_t Simplifier::rem_cls_from_watch_due_to_varelim(
    watch_subarray_const todo
    , const Lit lit
) {
    blockedMapBuilt = false;
    vector<Lit> lits;
    const size_t orig_blocked_cls_size = blockedClauses.size();

    //Copy todo --> it will be manipulated below
    vector<Watched> todo_copy;
    for(Watched tmp: todo) {
        todo_copy.push_back(tmp);
    }

    solver->watches[lit.toInt()].clear();
    for (const Watched watch :todo_copy) {
        lits.clear();
        bool red = false;

        if (watch.isClause()) {
            ClOffset offset = watch.getOffset();
            Clause& cl = *solver->clAllocator.getPointer(offset);
            if (cl.getRemoved()) {
                continue;
            }

            //Update stats
            if (!cl.red()) {
                runStats.clauses_elimed_long++;
                runStats.clauses_elimed_sumsize += cl.size();

                lits.resize(cl.size());
                std::copy(cl.begin(), cl.end(), lits.begin());
                add_clause_to_blck(lit, lits);
            } else {
                red = true;
                runStats.longRedClRemThroughElim++;
            }

            //Remove -- only DRUP the ones that are redundant
            //The irred will be removed thanks to 'blocked' system
            unlinkClause(offset, cl.red(), true, true);
        }

        if (watch.isBinary()) {

            //Update stats
            if (!watch.red()) {
                runStats.clauses_elimed_bin++;
                runStats.clauses_elimed_sumsize += 2;
            } else {
                red = true;
                runStats.binRedClRemThroughElim++;
            }

            //Put clause into blocked status
            lits.resize(2);
            lits[0] = lit;
            lits[1] = watch.lit2();
            if (!watch.red()) {
                add_clause_to_blck(lit, lits);
                touched.touch(watch.lit2());
            } else {
                //If redundant, delayed blocked-based DRUP deletion will not work
                //so delete explicitly

                //Drup
                if (!solver->conf.doStamp && !solver->conf.doCache) {
                   (*solver->drup) << del << lits[0] << lits[1] << fin;
                }
            }

            //Remove
            *limit_to_decrease -= (long)solver->watches[lits[0].toInt()].size();
            *limit_to_decrease -= (long)solver->watches[lits[1].toInt()].size();
            solver->detachBinClause(lits[0], lits[1], red, true);
        }

        if (watch.isTri()) {

            //Update stats
            if (!watch.red()) {
                runStats.clauses_elimed_tri++;
                runStats.clauses_elimed_sumsize += 3;
            } else {
                red = true;
                runStats.triRedClRemThroughElim++;
            }

            //Put clause into blocked status
            lits.resize(3);
            lits[0] = lit;
            lits[1] = watch.lit2();
            lits[2] = watch.lit3();
            if (!watch.red()) {
                add_clause_to_blck(lit, lits);
                touched.touch(watch.lit2());
                touched.touch(watch.lit3());
            } else {
                //If redundant, delayed blocked-based DRUP deletion will not work
                //so delete explicitly
                (*solver->drup) << del << lits[0] << lits[1] << lits[2] << fin;
            }

            //Remove
            *limit_to_decrease -= (long)solver->watches[lits[0].toInt()].size();
            *limit_to_decrease -= (long)solver->watches[lits[1].toInt()].size();
            *limit_to_decrease -= (long)solver->watches[lits[2].toInt()].size();
            solver->detachTriClause(lits[0], lits[1], lits[2], watch.red(), true);
        }

        if (solver->conf.verbosity >= 3 && !lits.empty()) {
            cout
            << "Eliminated clause " << lits << " (red: " << red << ")"
            << " on var " << lit.var()+1
            << endl;
        }
    }

    return blockedClauses.size() - orig_blocked_cls_size;
}

void Simplifier::add_clause_to_blck(Lit lit, const vector<Lit>& lits)
{
    lit = solver->map_inter_to_outer(lit);
    vector<Lit> lits_outer = lits;
    solver->map_inter_to_outer(lits_outer);
    blockedClauses.push_back(BlockedClause(lit, lits_outer));
}

uint32_t Simplifier::numIrredBins(const Lit lit) const
{
    uint32_t num = 0;
    watch_subarray_const ws = solver->watches[lit.toInt()];
    for (watch_subarray::const_iterator it = ws.begin(), end = ws.end(); it != end; it++) {
        if (it->isBinary() && !it->red()) num++;
    }

    return num;
}

bool Simplifier::find_gate(
    Lit elim_lit
    , watch_subarray_const a
    , watch_subarray_const b
) {
    bool found_better = false;
    assert(toClear.empty());
    for(const Watched w: a) {
        if (w.isBinary()
            && !w.red()
        ) {
            seen[(~w.lit2()).toInt()] = 1;
            toClear.push_back(~w.lit2());
        }
    }

    for(const Watched w: b) {
        if (w.isBinary()
            || (w.isTri() && w.red())
        ) {
            continue;
        }

        if (w.isTri() && gate_lits_of_elim_cls.size() < 2) {
            gate_lits_of_elim_cls.clear();
            assert(!w.red());
            if (seen[w.lit2().toInt()] && seen[w.lit3().toInt()]) {
                gate_lits_of_elim_cls.push_back(w.lit2());
                gate_lits_of_elim_cls.push_back(w.lit3());
                found_better = true;
            }
        }

        if (w.isClause()) {
            const Clause* cl = solver->clAllocator.getPointer(w.getOffset());
            assert(cl->size() > 3);
            if (!cl->red() && cl->size()-1 > gate_lits_of_elim_cls.size()) {
                bool OK = true;
                for(const Lit lit: *cl) {
                    if (lit != ~elim_lit) {
                        if (!seen[lit.toInt()]) {
                            OK = false;
                            break;
                        }
                    }
                }

                //Found all lits inside
                if (OK) {
                    gate_lits_of_elim_cls.clear();
                    for(const Lit lit: *cl) {
                        if (lit != ~elim_lit) {
                            gate_lits_of_elim_cls.push_back(lit);
                        }
                    }
                    found_better = true;
                }
            }
        }
    }

    for(Lit l: toClear) {
        seen[l.toInt()] = 0;
    }
    toClear.clear();

    return found_better;
}

void Simplifier::mark_gate_in_poss_negs(
    Lit elim_lit
    , watch_subarray_const poss
    , watch_subarray_const negs
)
{
    gate_found_elim = false;
    gate_lits_of_elim_cls.clear();
    find_gate(elim_lit, poss, negs);
    gate_found_elim_pos = find_gate(~elim_lit, negs, poss);
    if (!gate_lits_of_elim_cls.empty()
        && solver->conf.verbosity >= 10
    ) {
        cout
        << "Lit: " << elim_lit
        << " gate_lits_of_elim_cls.size():" << gate_lits_of_elim_cls << endl
        << " gate_found_elim_pos:" << gate_found_elim_pos
        << endl;
    }
    if (!gate_lits_of_elim_cls.empty()) {
        gate_found_elim = true;
        if (!gate_found_elim_pos) {
            mark_gate_parts(elim_lit, poss, negs, poss_gate_parts, negs_gate_parts);
        } else {
            mark_gate_parts(~elim_lit, negs, poss, negs_gate_parts, poss_gate_parts);
        }
    }
}

void Simplifier::mark_gate_parts(
    Lit elim_lit
    , watch_subarray_const a
    , watch_subarray_const b
    , vector<char>& a_mark
    , vector<char>& b_mark
) {
    a_mark.clear();
    a_mark.resize(a.size(), 0);
    b_mark.clear();
    b_mark.resize(b.size(), 0);

    for(Lit lit: gate_lits_of_elim_cls) {
        seen[lit.toInt()] = 1;
    }

    size_t num_found = 0;
    size_t at = 0;
    for(Watched w: a) {
        if (w.isBinary()
            && !w.red()
            && seen[(~w.lit2()).toInt()]
        ) {
            num_found++;
            a_mark[at] = 1;
        }
        at++;
    }
    assert(num_found >= gate_lits_of_elim_cls.size()
        && "We have to find all, but there could be multiple that are the same"
        //NOTE: this is not a precise check but it's better than nothing
    );

    at = 0;
    num_found = 0;
    for(Watched w: b) {
        if (gate_lits_of_elim_cls.size() == 2
            && w.isTri()
            && !w.red()
            && seen[w.lit2().toInt()]
            && seen[w.lit3().toInt()]
        ) {
            b_mark[at] = 1;
            num_found++;
        }

        if (gate_lits_of_elim_cls.size() >= 3
            && w.isClause()
        ) {
            const Clause* cl = solver->clAllocator.getPointer(w.getOffset());
            if (!cl->red() && cl->size()-1 == gate_lits_of_elim_cls.size()) {
                bool found_it = true;
                for(const Lit lit: *cl) {
                    if (lit != ~elim_lit) {
                        if (!seen[lit.toInt()]) {
                            found_it = false;
                            break;
                        }
                    }
                }
                if (found_it) {
                    b_mark[at] = 1;
                    num_found++;
                }
            }
        }
        at++;
    }
    assert(num_found >= 1
        && "We have to find the matching gate clause. But there could be multiple matching"
    );

    for(Lit lit: gate_lits_of_elim_cls) {
        seen[lit.toInt()] = 0;
    }
}

bool Simplifier::skip_resolution_thanks_to_gate(
    const size_t at_poss
    , const size_t at_negs
) const {
    if (!gate_found_elim)
        return false;

    return poss_gate_parts[at_poss] == negs_gate_parts[at_negs];
}

int Simplifier::test_elim_and_fill_resolvents(const Var var)
{
    assert(solver->ok);
    assert(solver->varData[var].removed == Removed::none);
    assert(solver->value(var) == l_Undef);

    //Gather data
    HeuristicData pos = calc_data_for_heuristic(Lit(var, false));
    HeuristicData neg = calc_data_for_heuristic(Lit(var, true));

    //Heuristic calculation took too much time
    if (*limit_to_decrease < 0) {
        return 1000;
    }

    //Check if we should do aggressive check or not
    bool aggressive = (aggressive_elim_time_limit > 0);
    runStats.usedAggressiveCheckToELim += aggressive;

    //set-up
    const Lit lit = Lit(var, false);
    watch_subarray poss = solver->watches[lit.toInt()];
    watch_subarray negs = solver->watches[(~lit).toInt()];
    std::sort(poss.begin(), poss.end(), watch_sort_smallest_first());
    std::sort(negs.begin(), negs.end(), watch_sort_smallest_first());
    resolvents.clear();

    //Pure literal, no resolvents
    //we look at "pos" and "neg" (and not poss&negs) because we don't care about redundant clauses
    if (pos.totalCls() == 0 || neg.totalCls() == 0) {
        return -100;
    }

    //Too expensive to check, it's futile
    if (pos.totalCls() >= solver->conf.varelim_cutoff_too_many_clauses
        && neg.totalCls() >= solver->conf.varelim_cutoff_too_many_clauses
    ) {
        return 1000;
    }

    if (solver->conf.skip_some_bve_resolvents) {
        mark_gate_in_poss_negs(lit, poss, negs);
    }

    // Count clauses/literals after elimination
    uint32_t before_clauses = pos.bin + pos.tri + pos.longer + neg.bin + neg.tri + neg.longer;
    uint32_t after_clauses = 0;
    uint32_t after_long = 0;
    uint32_t after_bin = 0;
    uint32_t after_tri = 0;
    uint32_t after_literals = 0;

    size_t at_poss = 0;
    for (watch_subarray::const_iterator
        it = poss.begin(), end = poss.end()
        ; it != end
        ; it++, at_poss++
    ) {
        *limit_to_decrease -= 3;
        if (solver->redundant_or_removed(*it))
            continue;

        size_t at_negs = 0;
        for (watch_subarray::const_iterator
            it2 = negs.begin(), end2 = negs.end()
            ; it2 != end2
            ; it2++, at_negs++
        ) {
            *limit_to_decrease -= 3;
            if (solver->redundant_or_removed(*it2))
                continue;

            if (solver->conf.skip_some_bve_resolvents
                && solver->conf.otfHyperbin
                //Below: Always resolve binaries so that cache&stamps stay OK
                && !(it->isBinary() && it2->isBinary())
                //Real check
                && skip_resolution_thanks_to_gate(at_poss, at_negs)
            ) {
                continue;
            }

            //Resolve the two clauses
            bool tautological = resolve_clauses(*it, *it2, lit, aggressive);
            if (tautological)
                continue;

            #ifdef VERBOSE_DEBUG_VARELIM
            cout << "Adding new clause due to varelim: " << dummy << endl;
            #endif

            //Update after-stats
            after_clauses++;
            after_literals += dummy.size();
            if (dummy.size() > 3)
                after_long++;
            if (dummy.size() == 3)
                after_tri++;
            if (dummy.size() == 2)
                after_bin++;

            //Early-abort or over time
            if (after_clauses > before_clauses
                //Over-time
                || *limit_to_decrease < -10LL*1000LL
            ) {
                return 1000;
            }

            //Calculate new clause stats
            ClauseStats stats;
            if ((it->isBinary() || it->isTri()) && it2->isClause())
                stats = solver->clAllocator.getPointer(it2->getOffset())->stats;
            else if ((it2->isBinary() || it2->isTri()) && it->isClause())
                stats = solver->clAllocator.getPointer(it->getOffset())->stats;
            else if (it->isClause() && it2->isClause())
                stats = ClauseStats::combineStats(
                    solver->clAllocator.getPointer(it->getOffset())->stats
                    , solver->clAllocator.getPointer(it2->getOffset())->stats
            );

            resolvents.push_back(Resolvent(dummy, stats));
        }
    }

    //Smaller value returned, the better
    int cost = (int)after_long + (int)after_tri + (int)after_bin*(int)3
        - (int)pos.longer - (int)neg.longer
        - (int)pos.tri - (int)neg.tri
        - (int)pos.bin*3 - (int)neg.bin*(int)3;

    return cost;
}

void Simplifier::printOccur(const Lit lit) const
{
    for(size_t i = 0; i < solver->watches[lit.toInt()].size(); i++) {
        const Watched& w = solver->watches[lit.toInt()][i];
        if (w.isBinary()) {
            cout
            << "Bin   --> "
            << lit << ", "
            << w.lit2()
            << "(red: " << w.red()
            << ")"
            << endl;
        }

        if (w.isTri()) {
            cout
            << "Tri   --> "
            << lit << ", "
            << w.lit2() << " , " << w.lit3()
            << "(red: " << w.red()
            << ")"
            << endl;
        }

        if (w.isClause()) {
            cout
            << "Clause--> "
            << *solver->clAllocator.getPointer(w.getOffset())
            << "(red: " << solver->clAllocator.getPointer(w.getOffset())->red()
            << ")"
            << endl;
        }
    }
}

void Simplifier::print_var_eliminate_stat(const Lit lit) const
{
    //Eliminate:
    if (solver->conf.verbosity < 5)
        return;

    cout
    << "Eliminating var " << lit
    << " with occur sizes "
    << solver->watches[lit.toInt()].size() << " , "
    << solver->watches[(~lit).toInt()].size()
    << endl;

    cout << "POS: " << endl;
    printOccur(lit);
    cout << "NEG: " << endl;
    printOccur(~lit);
}

bool Simplifier::check_if_new_2_long_subsumes_3_long_return_already_inside(const vector<Lit>& lits_orig)
{
    assert(lits_orig.size() == 2);
    Lit lits[2];
    lits[0] = lits_orig[0];
    lits[1] = lits_orig[1];
    if (solver->watches[lits[0].toInt()].size() > solver->watches[lits[1].toInt()].size()) {
        std::swap(lits[0], lits[1]);
    }

    bool already_inside = false;
    *limit_to_decrease -= solver->watches[lits[0].toInt()].size()/2;
    Watched* i = solver->watches[lits[0].toInt()].begin();
    Watched* j = i;
    for(Watched* end = solver->watches[lits[0].toInt()].end(); i != end; i++) {
        const Watched& w = *i;

        if (w.isBinary()
            && !w.red()
            && w.lit2() == lits[1]
        ) {
            if (solver->conf.verbosity >= 6) {
                cout
                << "Not adding resolvd BIN, it's aready inside"
                << " irred bin: "
                << lits[0]
                << ", " << w.lit2()
                << endl;
            }
            already_inside = true;
        }

        if (w.isTri()
            //&& w.red()
            && (   w.lit2() == lits[1]
                || w.lit3() == lits[1]
            )
        ) {
            if (solver->conf.verbosity >= 6) {
                cout
                << "Removing tri-clause due to addition of"
                << " irred bin: "
                << lits[0]
                << ", " << w.lit2()
                << ", " << w.lit3()
                << endl;
            }
            runStats.subsumedByVE++;
            solver->remove_tri_but_lit1(lits[0],w.lit2(), w.lit3(), w.red(), *limit_to_decrease);
        } else {
            *j++ = *i;
        }
    }
    solver->watches[lits[0].toInt()].shrink(i-j);

    return already_inside;
}

bool Simplifier::add_varelim_resolvent(
    vector<Lit>& finalLits
    , const ClauseStats& stats
) {
    runStats.newClauses++;
    Clause* newCl = NULL;

    //Check if a new 2-long would subsume a 3-long if we have time
    if (finalLits.size() == 2 && *limit_to_decrease > 10LL*1000LL) {
        bool already_inside = check_if_new_2_long_subsumes_3_long_return_already_inside(finalLits);
        if (already_inside) {
            goto subsume;
        }
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "adding v-elim resolvent: "
        << finalLits
        << endl;
    }

    newCl = solver->addClauseInt(
        finalLits //Literals in new clause
        , false //Is the new clause redundant?
        , stats //Statistics for this new clause (usage, etc.)
        , false //Should clause be attached?
        , &finalLits //Return final set of literals here
    );

    if (!solver->ok)
        return false;

    if (newCl != NULL) {
        linkInClause(*newCl);
        ClOffset offset = solver->clAllocator.getOffset(newCl);
        clauses.push_back(offset);
        runStats.subsumedByVE += subsumeStrengthen->subsume_and_unlink_and_markirred(offset);
    } else if (finalLits.size() == 3 || finalLits.size() == 2) {
        if (*limit_to_decrease > 10LL*1000LL) {
            subsume:
            try_to_subsume_with_new_bin_or_tri(finalLits);
        }
    }

    //Touch every var of the new clause, so we re-estimate
    //elimination complexity for this var
    for(Lit lit: finalLits)
        touched.touch(lit);

    return true;
}

void Simplifier::try_to_subsume_with_new_bin_or_tri(const vector<Lit>& lits)
{
    SubsumeStrengthen::Sub0Ret ret = subsumeStrengthen->subsume_and_unlink(
        std::numeric_limits<uint32_t>::max() //Index of this implicit clause (non-existent)
        , lits //Literals in this binary clause
        , calcAbstraction(lits) //Abstraction of literals
        , true //subsume implicit ones
    );
    runStats.subsumedByVE += ret.numSubsumed;
    if (ret.numSubsumed > 0) {
        if (solver->conf.verbosity >= 5) {
            cout << "Subsumed: " << ret.numSubsumed << endl;
        }
    }
}

void Simplifier::update_varelim_complexity_heap(const Var var)
{
    //Update var elim complexity heap
    if (!solver->conf.updateVarElimComplexityOTF)
        return;

    if (num_otf_update_until_now > solver->conf.updateVarElimComplexityOTF_limitvars) {
        const double avg = (double)time_spent_on_calc_otf_update/(double)num_otf_update_until_now;
        //cout << "num_otf_update_until_now: " << num_otf_update_until_now << endl;
        //cout << "avg: " << avg << endl;

        if (avg > solver->conf.updateVarElimComplexityOTF_limitavg) {
            solver->conf.updateVarElimComplexityOTF = false;
            if (solver->conf.verbosity >= 2) {
                cout
                << "c [v-elim] Turning off OTF complexity update, it's too expensive"
                << endl;
            }
            return;
        }
    }


    int64_t limit_before = *limit_to_decrease;
    num_otf_update_until_now++;
    for(Var touchVar: touched.getTouchedList()) {
        //No point in updating the score of this var
        //it's eliminated already, or not to be eliminated at all
        if (touchVar == var
            || !varElimOrder.inHeap(touchVar)
            || solver->value(touchVar) != l_Undef
            || solver->varData[touchVar].removed != Removed::none
        ) {
            continue;
        }

        varElimComplexity[touchVar] = strategyCalcVarElimScore(touchVar);
        varElimOrder.update(touchVar);
    }
    time_spent_on_calc_otf_update += limit_before - *limit_to_decrease;
}

void Simplifier::print_var_elim_complexity_stats(const Var var) const
{
    if (solver->conf.verbosity < 5)
        return;

    cout << "trying complexity: "
    << varElimComplexity[var].first
    << ", " << varElimComplexity[var].second
    << endl;
}

void Simplifier::set_var_as_eliminated(const Var var, const Lit lit)
{
    if (solver->conf.verbosity >= 5) {
        cout << "Elimination of var "
        <<  solver->map_inter_to_outer(lit)
        << " finished " << endl;
    }
    solver->varData[var].removed = Removed::elimed;
    runStats.numVarsElimed++;
    solver->unsetDecisionVar(var);
}

void Simplifier::create_dummy_blocked_clause(const Lit lit)
{
    blockedClauses.push_back(
        BlockedClause(solver->map_inter_to_outer(lit))
    );
}

bool Simplifier::maybeEliminate(const Var var)
{
    assert(solver->ok);
    print_var_elim_complexity_stats(var);
    runStats.testedToElimVars++;

    //Heuristic says no, or we ran out of time
    if (test_elim_and_fill_resolvents(var) == 1000
        || *limit_to_decrease < 0
    ) {
        return false;
    }
    runStats.triedToElimVars++;

    const Lit lit = Lit(var, false);
    print_var_eliminate_stat(lit);

    //Remove clauses
    touched.clear();
    create_dummy_blocked_clause(lit);
    rem_cls_from_watch_due_to_varelim(solver->watches[lit.toInt()], lit);
    rem_cls_from_watch_due_to_varelim(solver->watches[(~lit).toInt()], ~lit);

    //It's best to add resolvents with largest first. Then later, the smaller ones
    //can subsume the larger ones. While adding, we do subsumption check.
    std::sort(resolvents.begin(), resolvents.end());

    //Add resolvents
    for(Resolvent& resolvent: resolvents) {
        bool ok = add_varelim_resolvent(resolvent.lits, resolvent.stats);
        if (!ok)
            goto end;
    }

    if (*limit_to_decrease > 0) {
        update_varelim_complexity_heap(var);
    }

end:
    set_var_as_eliminated(var, lit);

    return true;
}

/*void Simplifier::addRedBinaries(const Var var)
{
    vector<Lit> tmp(2);
    Lit lit = Lit(var, false);
    watch_subarray_const ws = solver->watches[(~lit).toInt()];
    watch_subarray_const ws2 = solver->watches[lit.toInt()];

    for (watch_subarray::const_iterator w1 = ws.begin(), end1 = ws.end(); w1 != end1; w1++) {
        if (!w1->isBinary()) continue;
        const bool numOneIsRed = w1->red();
        const Lit lit1 = w1->lit2();
        if (solver->value(lit1) != l_Undef || var_elimed[lit1.var()]) continue;

        for (watch_subarray::const_iterator w2 = ws2.begin(), end2 = ws2.end(); w2 != end2; w2++) {
            if (!w2->isBinary()) continue;
            const bool numTwoIsRed = w2->red();
            if (!numOneIsRed && !numTwoIsRed) {
                //At least one must be redundant
                continue;
            }

            const Lit lit2 = w2->lit2();
            if (solver->value(lit2) != l_Undef || var_elimed[lit2.var()]) continue;

            tmp[0] = lit1;
            tmp[1] = lit2;
            Clause* tmpOK = solver->addClauseInt(tmp, true);
            runStats.numRedBinVarRemAdded++;
            release_assert(tmpOK == NULL);
            release_assert(solver->ok);
        }
    }
    assert(solver->value(lit) == l_Undef);
}*/

void Simplifier::add_pos_lits_to_dummy_and_seen(
    const Watched ps
    , const Lit posLit
) {
    if (ps.isBinary() || ps.isTri()) {
        *limit_to_decrease -= 1;
        assert(ps.lit2() != posLit);

        seen[ps.lit2().toInt()] = 1;
        dummy.push_back(ps.lit2());
    }

    if (ps.isTri()) {
        assert(ps.lit2() < ps.lit3());

        seen[ps.lit3().toInt()] = 1;
        dummy.push_back(ps.lit3());
    }

    if (ps.isClause()) {
        Clause& cl = *solver->clAllocator.getPointer(ps.getOffset());
        *limit_to_decrease -= (long)cl.size();
        for (const Lit lit : cl){
            if (lit != posLit) {
                seen[lit.toInt()] = 1;
                dummy.push_back(lit);
            }
        }
    }
}

bool Simplifier::add_neg_lits_to_dummy_and_seen(
    const Watched qs
    , const Lit posLit
) {
    if (qs.isBinary() || qs.isTri()) {
        *limit_to_decrease -= 2;
        assert(qs.lit2() != ~posLit);

        if (seen[(~qs.lit2()).toInt()]) {
            return true;
        }
        if (!seen[qs.lit2().toInt()]) {
            dummy.push_back(qs.lit2());
            seen[qs.lit2().toInt()] = 1;
        }
    }

    if (qs.isTri()) {
        assert(qs.lit2() < qs.lit3());

        if (seen[(~qs.lit3()).toInt()]) {
            return true;
        }
        if (!seen[qs.lit3().toInt()]) {
            dummy.push_back(qs.lit3());
            seen[qs.lit3().toInt()] = 1;
        }
    }

    if (qs.isClause()) {
        Clause& cl = *solver->clAllocator.getPointer(qs.getOffset());
        *limit_to_decrease -= (long)cl.size();
        for (const Lit lit: cl) {
            if (lit == ~posLit)
                continue;

            if (seen[(~lit).toInt()]) {
                return true;
            }

            if (!seen[lit.toInt()]) {
                dummy.push_back(lit);
                seen[lit.toInt()] = 1;
            }
        }
    }

    return false;
}

bool Simplifier::reverse_vivification_of_dummy(
    const Watched ps
    , const Watched qs
    , const Lit posLit
) {
    /*
    //TODO
    //Use watchlists
    if (numMaxVarElimAggressiveCheck > 0) {
        if (aggressiveCheck(lit, noPosLit, retval))
            goto end;
    }*/

    //Cache can only be used if none are binary
    if (ps.isBinary()
        || qs.isBinary()
        || !solver->conf.doCache
        || (!solver->conf.otfHyperbin && solver->drup->enabled())
    ) {
        return false;
    }

    for (size_t i = 0
        ; i < toClear.size() && aggressive_elim_time_limit > 0
        ; i++
    ) {
        aggressive_elim_time_limit -= 3;
        const Lit lit = toClear[i];
        assert(lit.var() != posLit.var());

        //Use cache
        const vector<LitExtra>& cache = solver->implCache[lit.toInt()].lits;
        aggressive_elim_time_limit -= cache.size()/3;
        for(const LitExtra litextra: cache) {
            //If redundant, that doesn't help
            if (!litextra.getOnlyIrredBin())
                continue;

            const Lit otherLit = litextra.getLit();
            if (otherLit.var() == posLit.var())
                continue;

            //If (a) was in original clause
            //then (a V b) means -b can be put inside
            if (!seen[(~otherLit).toInt()]) {
                toClear.push_back(~otherLit);
                seen[(~otherLit).toInt()] = 1;
            }

            //If (a V b) is irred in the clause, then done
            if (seen[otherLit.toInt()]) {
                return true;
            }
        }
    }

    return false;
}

bool Simplifier::subsume_dummy_through_stamping(
    const Watched ps
    , const Watched qs
) {
    //only if none of the clauses were binary
    //Otherwise we cannot tell if the value in the cache is dependent
    //on the binary clause itself, so that would cause a circular de-
    //pendency

    if (!ps.isBinary() && !qs.isBinary()) {
        aggressive_elim_time_limit -= toClear.size()*5;
        if (solver->stamp.stampBasedClRem(toClear)) {
            return true;
        }
    }

    return false;
}

bool Simplifier::resolve_clauses(
    const Watched ps
    , const Watched qs
    , const Lit posLit
    , const bool aggressive
) {
    //If clause has already been freed, skip
    if (ps.isClause()
        && solver->clAllocator.getPointer(ps.getOffset())->freed()
    ) {
        return false;
    }
    if (qs.isClause()
        && solver->clAllocator.getPointer(qs.getOffset())->freed()
    ) {
        return false;
    }

    dummy.clear();
    assert(toClear.empty());
    add_pos_lits_to_dummy_and_seen(ps, posLit);
    bool tautological = add_neg_lits_to_dummy_and_seen(qs, posLit);
    toClear = dummy;

    if (!tautological && aggressive) {
        tautological = reverse_vivification_of_dummy(ps, qs, posLit);
    }

    if (!tautological && aggressive
        && solver->conf.doStamp
        && solver->conf.otfHyperbin
    ) {
        tautological = subsume_dummy_through_stamping(ps, qs);
    }

    *limit_to_decrease -= (long)toClear.size()/2 + 1;
    for (const Lit lit: toClear) {
        seen[lit.toInt()] = 0;
    }
    toClear.clear();

    return tautological;
}

bool Simplifier::aggressiveCheck(
    const Lit lit
    , const Lit noPosLit
    , bool& retval
) {
    watch_subarray_const ws = solver->watches[lit.toInt()];
    aggressive_elim_time_limit -= ws.size()/3 + 2;
    for(watch_subarray::const_iterator it =
        ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        //Can't do much with clauses, too expensive
        if (it->isClause())
            continue;

        //handle tri
        if (it->isTri() && !it->red()) {

            //See if any of the literals is in
            Lit otherLit = lit_Undef;
            unsigned inside = 0;
            if (seen[it->lit2().toInt()]) {
                otherLit = it->lit3();
                inside++;
            }

            if (seen[it->lit3().toInt()]) {
                otherLit = it->lit2();
                inside++;
            }

            //Could subsume
            if (inside == 2) {
                retval = false;
                return true;
            }

            //None is in, skip
            if (inside == 0)
                continue;

            if (otherLit.var() == noPosLit.var())
                continue;

            //Extend clause
            if (!seen[(~otherLit).toInt()]) {
                toClear.push_back(~otherLit);
                seen[(~otherLit).toInt()] = 1;
            }

            continue;
        }

        //Handle binary
        if (it->isBinary() && !it->red()) {
            const Lit otherLit = it->lit2();
            if (otherLit.var() == noPosLit.var())
                continue;

            //If (a V b) is irred, and in the clause, then we can remove
            if (seen[otherLit.toInt()]) {
                retval = false;
                return true;
            }

            //If (a) is in clause
            //then (a V b) means -b can be put inside
            if (!seen[(~otherLit).toInt()]) {
                toClear.push_back(~otherLit);
                seen[(~otherLit).toInt()] = 1;
            }
        }
    }

    return false;
}

Simplifier::HeuristicData Simplifier::calc_data_for_heuristic(const Lit lit)
{
    HeuristicData ret;

    watch_subarray_const ws_list = solver->watches[lit.toInt()];
    *limit_to_decrease -= (long)ws_list.size()*3 + 100;
    for (const Watched ws: ws_list) {
        //Skip redundant clauses
        if (solver->redundant(ws))
            continue;

        switch(ws.getType()) {
            case watch_binary_t:
                ret.bin++;
                ret.lit += 2;
                break;

            case CMSat::watch_tertiary_t:
                ret.tri++;
                ret.lit += 3;
                break;

            case watch_clause_t: {
                const Clause* cl = solver->clAllocator.getPointer(ws.getOffset());
                if (!cl->getRemoved()) {
                    assert(!cl->freed() && "Inside occur, so cannot be freed");
                    ret.longer++;
                    ret.lit += cl->size();
                }
                break;
            }

            default:
                assert(false);
                break;
        }
    }
    return ret;
}

bool Simplifier::checkEmptyResolvent(Lit lit)
{
    //Take the smaller of the two
    if (solver->watches[(~lit).toInt()].size() < solver->watches[lit.toInt()].size())
        lit = ~lit;

    int num_bits_set = checkEmptyResolventHelper(
        lit
        , ResolvCount::set
        , 0
    );

    int num_resolvents = std::numeric_limits<int>::max();

    //Can only count if the POS was small enough
    //otherwise 'seen' cannot properly store the data
    if (num_bits_set < 16) {
        num_resolvents = checkEmptyResolventHelper(
            ~lit
            , ResolvCount::count
            , num_bits_set
        );
    }

    //Clear the 'seen' array
    checkEmptyResolventHelper(
        lit
        , ResolvCount::unset
        , 0
    );

    //Okay, this would be great
    return (num_resolvents == 0);
}


int Simplifier::checkEmptyResolventHelper(
    const Lit lit
    , const ResolvCount action
    , const int otherSize
) {
    uint16_t at = 1;
    int count = 0;
    size_t numCls = 0;

    watch_subarray_const watch_list = solver->watches[lit.toInt()];
    *limit_to_decrease -= (long)watch_list.size()*2;
    for (const Watched& ws: watch_list) {
        if (numCls >= 16
            && (action == ResolvCount::set
                || action == ResolvCount::unset)
        ) {
            break;
        }

        if (count > 0
            && action == ResolvCount::count
        ) {
            break;
        }

        //Handle binary
        if (ws.isBinary()){
            //Only count irred
            if (!ws.red()) {
                *limit_to_decrease -= 4;
                switch(action) {
                    case ResolvCount::set:
                        seen[ws.lit2().toInt()] |= at;
                        break;

                    case ResolvCount::unset:
                        seen[ws.lit2().toInt()] = 0;
                        break;

                    case ResolvCount::count:
                        int num = __builtin_popcount(seen[(~ws.lit2()).toInt()]);
                        assert(num <= otherSize);
                        count += otherSize - num;
                        break;
                }
                at <<= 1;
                numCls++;
            }
            continue;
        }

        //Handle tertiary
        if (ws.isTri()) {
            //Only count irred
            if (!ws.red()) {
                *limit_to_decrease -= 4;
                switch(action) {
                    case ResolvCount::set:
                        seen[ws.lit2().toInt()] |= at;
                        seen[ws.lit3().toInt()] |= at;
                        break;

                    case ResolvCount::unset:
                        seen[ws.lit2().toInt()] = 0;
                        seen[ws.lit3().toInt()] = 0;
                        break;

                    case ResolvCount::count:
                        uint16_t tmp = seen[(~ws.lit2()).toInt()] | seen[(~ws.lit3()).toInt()];
                        int num = __builtin_popcount(tmp);
                        assert(num <= otherSize);
                        count += otherSize - num;
                        break;
                }
                at <<= 1;
                numCls++;
            }

            continue;
        }

        if (ws.isClause()) {
            const Clause* cl = solver->clAllocator.getPointer(ws.getOffset());
            if (cl->getRemoved()) {
                continue;
            }

            //If in occur then it cannot be freed
            assert(!cl->freed());

            //Only irred is of relevance
            if (!cl->red()) {
                *limit_to_decrease -= (long)cl->size()*2;
                uint16_t tmp = 0;
                for(const Lit l: *cl) {

                    //Ignore orig
                    if (l == lit)
                        continue;

                    switch (action) {
                        case ResolvCount::set:
                            seen[l.toInt()] |= at;
                            break;

                        case ResolvCount::unset:
                            seen[l.toInt()] = 0;
                            break;

                        case ResolvCount::count:
                            tmp |= seen[(~l).toInt()];
                            break;
                    }
                }
                at <<= 1;
                numCls++;

                //Count using tmp
                if (action == ResolvCount::count) {
                    int num = __builtin_popcount(tmp);
                    assert(num <= otherSize);
                    count += otherSize - num;
                }
            }

            continue;
        }

        //Only these types are possible
        assert(false);
    }

    switch(action) {
        case ResolvCount::count:
            return count;

        case ResolvCount::set:
            return numCls;

        case ResolvCount::unset:
            return 0;
    }

    assert(false);
    return std::numeric_limits<int>::max();
}



pair<int, int> Simplifier::heuristicCalcVarElimScore(const Var var)
{
    const Lit lit(var, false);
    const HeuristicData pos = calc_data_for_heuristic(lit);
    const HeuristicData neg = calc_data_for_heuristic(~lit);

    //Estimate cost
    int posTotalLonger = pos.longer + pos.tri;
    int negTotalLonger = neg.longer + neg.tri;
    int normCost;
    switch(solver->conf.varElimCostEstimateStrategy) {
        case 0:
            normCost =  posTotalLonger*negTotalLonger
                + pos.bin*negTotalLonger*2
                + neg.bin*posTotalLonger*2
                + pos.bin*neg.bin*3;
            break;

        case 1:
            normCost =  posTotalLonger*negTotalLonger
                + pos.bin*negTotalLonger*2
                + neg.bin*posTotalLonger*2
                + pos.bin*neg.bin*4;
            break;

        default:
            cout
            << "ERROR: Invalid var-elim cost estimation strategy"
            << endl;
            std::exit(-1);
            break;
    }


    /*if ((pos.longer + pos.tri + pos.bin) <= 2
        && (neg.longer + neg.tri + neg.bin) <= 2
    ) {
        normCost /= 2;
    }*/

    if ((pos.longer + pos.tri + pos.bin) == 0
        || (neg.longer + neg.tri + neg.bin) == 0
    ) {
        normCost = 0;
    }

    int litCost = pos.lit * neg.lit;

    return std::make_pair(normCost, litCost);
}

void Simplifier::order_vars_for_elim()
{
    varElimOrder.clear();
    varElimComplexity.clear();
    varElimComplexity.resize(
        solver->nVars()
        , std::make_pair<int, int>(1000, 1000)
    );

    //Go through all vars
    for (
        size_t var = 0
        ; var < solver->nVars() && *limit_to_decrease > 0
        ; var++
    ) {
        if (!can_eliminate_var(var))
            continue;

        *limit_to_decrease -= 50;
        assert(!varElimOrder.inHeap(var));
        varElimComplexity[var] = strategyCalcVarElimScore(var);
        varElimOrder.insert(var);
    }
    assert(varElimOrder.heapProperty());

    //Print sorted listed list
    #ifdef VERBOSE_DEBUG_VARELIM
    cout << "-----------" << endl;
    for(size_t i = 0; i < varElimOrder.size(); i++) {
        cout
        << "varElimOrder[" << i << "]: "
        << " var: " << varElimOrder[i]+1
        << " val: " << varElimComplexity[varElimOrder[i]].first
        << " , " << varElimComplexity[varElimOrder[i]].second
        << endl;
    }
    #endif
}

std::pair<int, int> Simplifier::strategyCalcVarElimScore(const Var var)
{
    std::pair<int, int> cost;
    if (solver->conf.var_elim_strategy == elimstrategy_heuristic) {
        cost = heuristicCalcVarElimScore(var);
    } else {
        int ret = test_elim_and_fill_resolvents(var);

        cost.first = ret;
        cost.second = 0;
    }

    return cost;
}

void Simplifier::checkElimedUnassigned() const
{
    for (size_t i = 0; i < solver->nVarsOuter(); i++) {
        if (solver->varData[i].removed == Removed::elimed) {
            assert(solver->value(i) == l_Undef);
        }
    }
}

void Simplifier::checkElimedUnassignedAndStats() const
{
    assert(solver->ok);
    int64_t checkNumElimed = 0;
    for (size_t i = 0; i < solver->nVarsOuter(); i++) {
        if (solver->varData[i].removed == Removed::elimed) {
            checkNumElimed++;
            assert(solver->value(i) == l_Undef);
        }
    }
    if (globalStats.numVarsElimed != checkNumElimed) {
        cout
        << "ERROR: globalStats.numVarsElimed is "<<
        globalStats.numVarsElimed
        << " but checkNumElimed is: " << checkNumElimed
        << endl;

        assert(false);
    }
}

size_t Simplifier::memUsed() const
{
    size_t b = 0;
    b += poss_gate_parts.capacity()*sizeof(char);
    b += negs_gate_parts.capacity()*sizeof(char);
    b += gate_lits_of_elim_cls.capacity()*sizeof(Lit);
    b += seen.capacity()*sizeof(char);
    b += seen2.capacity()*sizeof(char);
    b += dummy.capacity()*sizeof(char);
    b += toClear.capacity()*sizeof(Lit);
    b += subsumeStrengthen->memUsed();
    for(map<Var, vector<size_t> >::const_iterator
        it = blk_var_to_cl.begin(), end = blk_var_to_cl.end()
        ; it != end
        ; it++
    ) {
        b += it->second.capacity()*sizeof(size_t);
    }
    b += blockedClauses.capacity()*sizeof(BlockedClause);
    for(vector<BlockedClause>::const_iterator
        it = blockedClauses.begin(), end = blockedClauses.end()
        ; it != end
        ; it++
    ) {
        b += it->lits.capacity()*sizeof(Lit);
    }
    b += blk_var_to_cl.size()*(sizeof(Var)+sizeof(vector<size_t>)); //TODO under-counting
    b += varElimOrder.memUsed();
    b += varElimComplexity.capacity()*sizeof(int)*2;
    b += touched.memUsed();
    b += clauses.capacity()*sizeof(ClOffset);

    return b;
}

size_t Simplifier::memUsedXor() const
{
    if (xorFinder)
        return xorFinder->memUsed();
    else
        return 0;
}

void Simplifier::freeXorMem()
{
    delete xorFinder;
    xorFinder = NULL;
}

void Simplifier::linkInClause(Clause& cl)
{
    assert(cl.size() > 3);
    ClOffset offset = solver->clAllocator.getOffset(&cl);
    std::sort(cl.begin(), cl.end());
    for (const Lit lit: cl) {
        watch_subarray ws = solver->watches[lit.toInt()];
        *limit_to_decrease -= (long)ws.size();

        ws.push(Watched(offset, cl.abst));
    }
    assert(cl.abst == calcAbstraction(cl));
    cl.setOccurLinked(true);
}


void Simplifier::printGateFinderStats() const
{
    if (gateFinder) {
        gateFinder->getStats().print(solver->nVarsOuter());
    }
}

Lit Simplifier::least_occurring_except(const OccurClause& c)
{
    *limit_to_decrease -= (long)m_lits.size();
    for(const lit_pair lits: m_lits) {
        seen[lits.lit1.toInt()] = 1;
        if (lits.lit2 != lit_Undef) {
            seen[lits.lit2.toInt()] = 1;
        }
    }

    Lit smallest = lit_Undef;
    size_t smallest_val = std::numeric_limits<size_t>::max();
    const auto check_smallest = [&] (const Lit lit) {
        //Must not be in m_lits
        if (seen[lit.toInt()] != 0)
            return;

        const size_t watch_size = solver->watches[lit.toInt()].size();
        if (watch_size < smallest_val) {
            smallest = lit;
            smallest_val = watch_size;
        }
    };
    solver->for_each_lit_except_watched(c, check_smallest, limit_to_decrease);

    for(const lit_pair lits: m_lits) {
        seen[lits.lit1.toInt()] = 0;
        if (lits.lit2 != lit_Undef) {
            seen[lits.lit2.toInt()] = 0;
        }
    }

    return smallest;
}

Simplifier::lit_pair Simplifier::lit_diff_watches(const OccurClause& a, const OccurClause& b)
{
    //assert(solver->cl_size(a.ws) == solver->cl_size(b.ws));
    assert(a.lit != b.lit);
    solver->for_each_lit(b, [&](const Lit lit) {seen[lit.toInt()] = 1;}, limit_to_decrease);

    size_t num = 0;
    lit_pair toret = lit_pair(lit_Undef, lit_Undef);
    const auto check_seen = [&] (const Lit lit) {
        if (seen[lit.toInt()] == 0) {
            if (num == 0)
                toret.lit1 = lit;
            else
                toret.lit2 = lit;

            num++;
        }
    };
    solver->for_each_lit(a, check_seen, limit_to_decrease);
    solver->for_each_lit(b, [&](const Lit lit) {seen[lit.toInt()] = 0;}, limit_to_decrease);

    if (num >= 1 && num <= 2)
        return toret;
    else
        return lit_Undef;
}

Simplifier::lit_pair Simplifier::most_occuring_lit_in_potential(size_t& largest)
{
    largest = 0;
    lit_pair most_occur = lit_pair(lit_Undef, lit_Undef);
    if (potential.size() > 1) {
        *limit_to_decrease -= (double)potential.size()*(double)std::log(potential.size())*0.2;
        std::sort(potential.begin(), potential.end());
    }

    lit_pair last_occur = lit_pair(lit_Undef, lit_Undef);
    size_t num = 0;
    for(const PotentialClause pot: potential) {
        if (last_occur != pot.lits) {
            if (num >= largest) {
                largest = num;
                most_occur = last_occur;
            }
            last_occur = pot.lits;
            num = 1;
        } else {
            num++;
        }
    }
    if (num >= largest) {
        largest = num;
        most_occur = last_occur;
    }

    if (solver->conf.verbosity >= 5) {
        cout
        << "c [bva] ---> Most occuring lit in p: " << most_occur.lit1 << ", " << most_occur.lit2
        << " occur num: " << largest
        << endl;
    }

    return most_occur;
}

bool Simplifier::inside(const vector<Lit>& lits, const Lit notin) const
{
    for(const Lit lit: lits) {
        if (lit == notin)
            return true;
    }
    return false;
}

bool Simplifier::simplifies_system(const size_t num_occur) const
{
    //If first run, at least 2 must match, nothing else matters
    if (m_lits.size() == 1) {
        return num_occur >= 2;
    }

    assert(m_lits.size() > 1);
    int orig_num_red = simplification_size(m_lits.size(), m_cls.size());
    int new_num_red = simplification_size(m_lits.size()+1, num_occur);

    if (new_num_red <= 0)
        return false;

    if (new_num_red < orig_num_red)
        return false;

    return true;
}

int Simplifier::simplification_size(
    const int m_lits_size
    , const int m_cls_size
) const {
    return m_lits_size*m_cls_size-m_lits_size-m_cls_size;
}

void Simplifier::fill_potential(const Lit lit)
{
    for(const OccurClause& c: m_cls) {
        if (*limit_to_decrease < 0)
            break;

        const Lit l_min = least_occurring_except(c);
        if (l_min == lit_Undef)
            continue;

        m_lits_this_cl = m_lits;
        *limit_to_decrease -= m_lits_this_cl.size();
        for(const lit_pair lits: m_lits_this_cl) {
            seen2[lits.hash(seen2.size())] = 1;
        }

        if (solver->conf.verbosity >= 6 || bva_verbosity) {
            cout
            << "c [bva] Examining clause for addition to 'potential':"
            << solver->watched_to_string(c.lit, c.ws)
            << " -- Least occurring in this CL: " << l_min
            << endl;
        }

        *limit_to_decrease -= (long)solver->watches[l_min.toInt()].size()*3;
        for(const Watched& d_ws: solver->watches[l_min.toInt()]) {
            if (*limit_to_decrease < 0)
                goto end;

            OccurClause d(l_min, d_ws);
            const size_t sz_c = solver->cl_size(c.ws);
            const size_t sz_d = solver->cl_size(d.ws);
            if (c.ws != d.ws
                && (sz_c == sz_d
                    || (sz_c+1 == sz_d
                        && solver->conf.bva_also_twolit_diff
                        && (long)solver->sumConflicts() >= solver->conf.bva_extra_lit_and_red_start
                    )
                )
                && !solver->redundant(d.ws)
                && lit_diff_watches(c, d) == lit
            ) {
                const lit_pair diff = lit_diff_watches(d, c);
                if (seen2[diff.hash(seen2.size())] == 0) {
                    *limit_to_decrease -= 3;
                    potential.push_back(PotentialClause(diff, c));
                    m_lits_this_cl.push_back(diff);
                    seen2[diff.hash(seen2.size())] = 1;

                    if (solver->conf.verbosity >= 6 || bva_verbosity) {
                        cout
                        << "c [bva] Added to P: "
                        << potential.back().to_string(solver)
                        << endl;
                    }
                }
            }
        }

        end:
        for(const lit_pair lits: m_lits_this_cl) {
            seen2[lits.hash(seen2.size())] = 0;
        }
    }
}


bool Simplifier::VarBVAOrder::operator()(const uint32_t lit1_uint, const uint32_t lit2_uint) const
{
    return watch_irred_sizes[lit1_uint] > watch_irred_sizes[lit2_uint];
}

size_t Simplifier::calc_watch_irred_size(const Lit lit) const
{
    size_t num = 0;
    watch_subarray_const ws = solver->watches[lit.toInt()];
    for(const Watched w: ws) {
        if (w.isBinary() || w.isTri()) {
            num += !w.red();
            continue;
        }

        assert(w.isClause());
        const Clause& cl = *solver->clAllocator.getPointer(w.getOffset());
        num += !cl.red();
    }

    return num;
}

void Simplifier::calc_watch_irred_sizes()
{
    watch_irred_sizes.clear();
    for(size_t i = 0; i < solver->nVars()*2; i++) {
        const Lit lit = Lit::toLit(i);
        const size_t irred_size = calc_watch_irred_size(lit);
        watch_irred_sizes.push_back(irred_size);
    }
}

bool Simplifier::bounded_var_addition()
{
    bva_verbosity = false;
    assert(solver->ok);
    if (!solver->conf.do_bva)
        return solver->okay();

    if (solver->conf.verbosity >= 3 || bva_verbosity) {
        cout << "c [bva] Running BVA" << endl;
    }

    if (!solver->propagate_occur())
        return false;

    limit_to_decrease = &bounded_var_elim_time_limit;
    int64_t limit_orig = *limit_to_decrease;
    solver->clauseCleaner->clean_implicit_clauses();
    if (solver->conf.doStrSubImplicit) {
        solver->subsumeImplicit->subsume_implicit(false);
    }

    bva_worked = 0;
    bva_simp_size = 0;
    var_bva_order.clear();
    calc_watch_irred_sizes();
    for(size_t i = 0; i < solver->nVars()*2; i++) {
        const Lit lit = Lit::toLit(i);
        if (solver->value(lit) != l_Undef
            || solver->varData[lit.var()].removed != Removed::none
        ) {
            continue;
        }
        var_bva_order.insert(lit.toInt());
    }

    double my_time = cpuTime();
    while(!var_bva_order.empty()) {
        if (*limit_to_decrease < 0
            || bva_worked >= solver->conf.bva_limit_per_call
            || solver->must_interrupt_asap()
        ) {
            break;
        }

        const Lit lit = Lit::toLit(var_bva_order.removeMin());
        if (solver->conf.verbosity >= 5 || bva_verbosity) {
            cout << "c [bva] trying lit " << lit << endl;
        }
        bool ok = try_bva_on_lit(lit);
        if (!ok)
            break;
    }
    solver->bva_changed();

    bool time_out = *limit_to_decrease <= 0;
    double time_used = cpuTime() - my_time;
    double time_remain = ((double)*limit_to_decrease/(double)limit_orig);
    if (solver->conf.verbosity >= 2) {
        cout
        << "c [bva] added: " << bva_worked
        << " simp: " << bva_simp_size
        << " 2lit: " << ((solver->conf.bva_also_twolit_diff
            && (long)solver->sumConflicts() >= solver->conf.bva_extra_lit_and_red_start) ? "Y" : "N")
        << " T: " << time_used
        << " T-out: " << (time_out ? "Y" : "N")
        << " T-r: " << time_remain*100.0  << "%"
        << endl;
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "bva"
            , time_used
            , time_out
            , time_remain
        );
    }

    return solver->okay();
}

void Simplifier::remove_duplicates_from_m_cls()
{
    if (m_cls.size() <= 1)
        return;

    std::function<bool (const OccurClause&, const OccurClause&)> mysort
        = [&] (const OccurClause& a, const OccurClause& b) {
            WatchType atype = a.ws.getType();
            WatchType btype = b.ws.getType();
            if (atype == watch_binary_t && btype != CMSat::watch_binary_t) {
                return true;
            }
            if (btype == watch_binary_t && atype != CMSat::watch_binary_t) {
                return false;
            }
            if (atype == watch_tertiary_t && btype != CMSat::watch_tertiary_t) {
                return true;
            }
            if (btype == watch_tertiary_t && atype != CMSat::watch_tertiary_t) {
                return false;
            }

            assert(atype == btype);
            switch(atype) {
                case CMSat::watch_binary_t: {
                    //subsumption could have time-outed
                    //assert(a.ws.lit2() != b.ws.lit2() && "Implicit has been cleaned of duplicates!!");
                    return a.ws.lit2() < b.ws.lit2();
                }
                case CMSat::watch_tertiary_t: {
                    if (a.ws.lit2() != b.ws.lit2()) {
                        return a.ws.lit2() < b.ws.lit2();
                    }
                    //subsumption could have time-outed
                    //assert(a.ws.lit3() != b.ws.lit3() && "Implicit has been cleaned of duplicates!!");
                    return a.ws.lit3() < b.ws.lit3();
                }
                case CMSat::watch_clause_t: {
                    *limit_to_decrease -= 20;
                    const Clause& cl_a = *solver->clAllocator.getPointer(a.ws.getOffset());
                    const Clause& cl_b = *solver->clAllocator.getPointer(b.ws.getOffset());
                    if (cl_a.size() != cl_b.size()) {
                        return cl_a.size() < cl_b.size();
                    }
                    //Clauses' lits are sorted, yay!
                    for(size_t i = 0; i < cl_a.size(); i++) {
                        *limit_to_decrease -= 1;
                        if (cl_a[i] != cl_b[i]) {
                            return cl_a[i] < cl_b[i];
                        }
                    }
                    return false;
                }
            }

            assert(false);
            return false;
    };

    *limit_to_decrease -= 2*(long)m_cls.size()*(long)std::sqrt(m_cls.size());
    std::sort(m_cls.begin(), m_cls.end(), mysort);
    size_t i = 0;
    size_t j = 0;
    for(; i+1 < m_cls.size(); i++) {
        const Watched& prev = m_cls[j].ws;
        const Watched& next = m_cls[i+1].ws;
        if (prev.getType() != next.getType()) {
            m_cls[j+1] = m_cls[i+1];
            j++;
            continue;
        }

        bool del = false;
        switch(prev.getType()) {
            case CMSat::watch_binary_t: {
                if (prev.lit2() == next.lit2()) {
                    del = true;
                }
                break;
            }

            case CMSat::watch_tertiary_t: {
                if (prev.lit2() == next.lit2() && prev.lit3() == next.lit3())
                    del = true;
                break;
            }

            case CMSat::watch_clause_t: {
                *limit_to_decrease -= 10;
                const Clause& cl1 = *solver->clAllocator.getPointer(prev.getOffset());
                const Clause& cl2 = *solver->clAllocator.getPointer(next.getOffset());
                del = true;
                if (cl1.size() != cl2.size()) {
                    break;
                }
                for(size_t at = 0; at < cl1.size(); at++) {
                    *limit_to_decrease -= 1;
                    if (cl1[at] != cl2[at]) {
                        del = false;
                        break;
                    }
                }
                break;
            }
        }

        if (!del) {
            m_cls[j+1] = m_cls[i+1];
            //if (mark_irred) {
            //    m_cls[j+1].ws.setRed(false);
            //}
            j++;
        }
    }
    m_cls.resize(m_cls.size()-(i-j));

    if (solver->conf.verbosity >= 6 || bva_verbosity) {
        cout << "m_cls after cleaning: " << endl;
        for(const OccurClause& w: m_cls) {
            cout << "-> " << solver->watched_to_string(w.lit, w.ws) << endl;
        }
    }
}

bool Simplifier::try_bva_on_lit(const Lit lit)
{
    assert(solver->value(lit) == l_Undef);
    assert(solver->varData[lit.var()].removed == Removed::none);

    m_cls.clear();
    m_lits.clear();
    m_lits.push_back(lit);
    *limit_to_decrease -= solver->watches[lit.toInt()].size();
    for(const Watched w: solver->watches[lit.toInt()]) {
        if (!solver->redundant(w)) {
            m_cls.push_back(OccurClause(lit, w));
            if (solver->conf.verbosity >= 6 || bva_verbosity) {
                cout << "1st adding to m_cls "
                << solver->watched_to_string(lit, w)
                << endl;
            }
        }
    }
    remove_duplicates_from_m_cls();

    while(true) {
        potential.clear();
        fill_potential(lit);
        if (*limit_to_decrease < 0)
            break;

        size_t num_occur;
        const lit_pair l_max = most_occuring_lit_in_potential(num_occur);
        if (simplifies_system(num_occur)) {
            m_lits.push_back(l_max);
            m_cls.clear();
            *limit_to_decrease -= potential.size()*3;
            for(const PotentialClause pot: potential) {
                if (pot.lits == l_max) {
                    m_cls.push_back(pot.occur_cl);
                    if (solver->conf.verbosity >= 6 || bva_verbosity) {
                        cout << "-- max is : (" << l_max.lit1 << ", " << l_max.lit2 << "), adding to m_cls "
                        << solver->watched_to_string(pot.occur_cl.lit, pot.occur_cl.ws)
                        << endl;
                    }
                    assert(pot.occur_cl.lit == lit);
                }
            }
        } else {
            break;
        }
    }

    if (*limit_to_decrease < 0)
        return solver->okay();

    const int simp_size = simplification_size(m_lits.size(), m_cls.size());
    if (simp_size <= 0) {
        return solver->okay();
    }

    const bool ok = bva_simplify_system();
    return ok;
}

bool Simplifier::bva_simplify_system()
{
    touched.clear();
    int simp_size = simplification_size(m_lits.size(), m_cls.size());
    if (solver->conf.verbosity >= 6 || bva_verbosity) {
        cout
        << "c [bva] YES Simplification by "
        << simp_size
        << " with matching lits: ";
        for(const lit_pair l: m_lits) {
            cout << "(" << l.lit1;
            if (l.lit2 != lit_Undef) {
                cout << ", " << l.lit2;
            }
            cout << "), ";
        }
        cout << endl;
        cout << "c [bva] cls: ";
        for(OccurClause cl: m_cls) {
            cout
            << "(" << solver->watched_to_string(cl.lit, cl.ws) << ")"
            << ", ";
        }
        cout << endl;
    }
    bva_worked++;
    bva_simp_size += simp_size;

    solver->new_var(true);
    const Var newvar = solver->nVars()-1;
    const Lit new_lit(newvar, false);

    for(const lit_pair m_lit: m_lits) {
        bva_tmp_lits.clear();
        bva_tmp_lits.push_back(m_lit.lit1);
        if (m_lit.lit2 != lit_Undef) {
            bva_tmp_lits.push_back(m_lit.lit2);
        }
        bva_tmp_lits.push_back(new_lit);
        solver->addClauseInt(bva_tmp_lits, false, ClauseStats(), false, &bva_tmp_lits, true, new_lit);
        touched.touch(bva_tmp_lits);
    }

    for(const OccurClause m_cl: m_cls) {
        bool ok = add_longer_clause(~new_lit, m_cl);
        if (!ok)
            return false;
    }

    fill_m_cls_lits_and_red();
    for(const lit_pair replace_lit: m_lits) {
       //cout << "Doing lit " << replace_lit << " replacing lit " << lit << endl;
        for(const m_cls_lits_and_red& cl_lits_and_red: m_cls_lits) {
            remove_matching_clause(cl_lits_and_red, replace_lit);
        }
    }

    update_touched_lits_in_bva();

    return solver->okay();
}

void Simplifier::update_touched_lits_in_bva()
{
    const vector<uint32_t>& touched_list = touched.getTouchedList();
    for(const uint32_t lit_uint: touched_list) {
        const Lit lit = Lit::toLit(lit_uint);
        if (var_bva_order.inHeap(lit.toInt())) {
            watch_irred_sizes[lit.toInt()] = calc_watch_irred_size(lit);
            var_bva_order.update(lit.toInt());
        }

        if (var_bva_order.inHeap((~lit).toInt())) {
            watch_irred_sizes[(~lit).toInt()] = calc_watch_irred_size(~lit);
            var_bva_order.update((~lit).toInt());
        }
    }
    touched.clear();
}

void Simplifier::fill_m_cls_lits_and_red()
{
    m_cls_lits.clear();
    vector<Lit> tmp;
    for(OccurClause& cl: m_cls) {
        tmp.clear();
        bool red;
        switch(cl.ws.getType()) {
            case CMSat::watch_binary_t:
                tmp.push_back(cl.ws.lit2());
                red = cl.ws.red();
                break;

            case CMSat::watch_tertiary_t:
                tmp.push_back(cl.ws.lit2());
                tmp.push_back(cl.ws.lit3());
                red = cl.ws.red();
                break;

            case CMSat::watch_clause_t:
                const Clause* cl_orig = solver->clAllocator.getPointer(cl.ws.getOffset());
                for(const Lit lit: *cl_orig) {
                    if (cl.lit != lit) {
                        tmp.push_back(lit);
                    }
                }
                red = cl_orig->red();
                break;
        }
        m_cls_lits.push_back(m_cls_lits_and_red(tmp, red));
    }
}

void Simplifier::remove_matching_clause(
    const m_cls_lits_and_red& cl_lits_and_red
    , const lit_pair lit_replace
) {
    if (solver->conf.verbosity >= 6 || bva_verbosity) {
        cout
        << "c [bva] Removing cl "
        //<< solver->watched_to_string(lit_replace, cl.ws)
        << endl;
    }

    to_remove.clear();
    to_remove.push_back(lit_replace.lit1);
    if (lit_replace.lit2 != lit_Undef) {
        to_remove.push_back(lit_replace.lit2);
    }
    for(const Lit cl_lit: cl_lits_and_red.lits) {
        to_remove.push_back(cl_lit);
    }
    touched.touch(to_remove);

    switch(to_remove.size()) {
        case 2: {
            *limit_to_decrease -= 2*solver->watches[to_remove[0].toInt()].size();
            //bool red = !findWBin(solver->watches, to_remove[0], to_remove[1], false);
            bool red = false;
            *(solver->drup) << del << to_remove << fin;
            solver->detachBinClause(to_remove[0], to_remove[1], red);
            break;
        }

        case 3: {
            std::sort(to_remove.begin(), to_remove.end());
            *limit_to_decrease -= 2*solver->watches[to_remove[0].toInt()].size();
            //bool red = !findWTri(solver->watches, to_remove[0], to_remove[1], to_remove[2], false);
            bool red = false;
            *(solver->drup) << del << to_remove << fin;
            solver->detachTriClause(to_remove[0], to_remove[1], to_remove[2], red);
            break;
        }

        default:
            Clause* cl_new = find_cl_for_bva(to_remove, cl_lits_and_red.red);
            unlinkClause(solver->clAllocator.getOffset(cl_new));
            break;
    }
}

Clause* Simplifier::find_cl_for_bva(
    const vector<Lit>& torem
    , const bool red
) const {
    Clause* cl = NULL;
    for(const Lit lit: torem) {
        seen[lit.toInt()] = 1;
    }
    for(Watched w: solver->watches[torem[0].toInt()]) {
        if (!w.isClause())
            continue;

        cl = solver->clAllocator.getPointer(w.getOffset());
        if (cl->red() != red
            || cl->size() != torem.size()
        ) {
            continue;
        }

        bool OK = true;
        for(const Lit lit: *cl) {
            if (seen[lit.toInt()] == 0) {
                OK = false;
                break;
            }
        }

        if (OK)
            break;
    }

    for(const Lit lit: torem) {
        seen[lit.toInt()] = 0;
    }

    assert(cl != NULL);
    return cl;
}

bool Simplifier::add_longer_clause(const Lit new_lit, const OccurClause& cl)
{
    vector<Lit>& lits = bva_tmp_lits;
    lits.clear();
    switch(cl.ws.getType()) {
        case CMSat::watch_binary_t: {
            lits.resize(2);
            lits[0] = new_lit;
            lits[1] = cl.ws.lit2();
            solver->addClauseInt(lits, false, ClauseStats(), false, &lits, true, new_lit);
            break;
        }

        case CMSat::watch_tertiary_t: {
            lits.resize(3);
            lits[0] = new_lit;
            lits[1] = cl.ws.lit2();
            lits[2] = cl.ws.lit3();
            solver->addClauseInt(lits, false, ClauseStats(), false, &lits, true, new_lit);
            break;
        }

        case CMSat::watch_clause_t: {
            const Clause& orig_cl = *solver->clAllocator.getPointer(cl.ws.getOffset());
            lits.resize(orig_cl.size());
            for(size_t i = 0; i < orig_cl.size(); i++) {
                if (orig_cl[i] == cl.lit) {
                    lits[i] = new_lit;
                } else {
                    lits[i] = orig_cl[i];
                }
            }
            Clause* newCl = solver->addClauseInt(lits, false, orig_cl.stats, false, &lits, true, new_lit);
            if (newCl != NULL) {
                linkInClause(*newCl);
                ClOffset offset = solver->clAllocator.getOffset(newCl);
                clauses.push_back(offset);
            }
            break;
        }
    }
    touched.touch(lits);

    return solver->okay();
}

string Simplifier::PotentialClause::to_string(const Solver* solver) const
{
    std::stringstream ss;
    ss << solver->watched_to_string(occur_cl.lit, occur_cl.ws)
    << " -- lit: " << lits.lit1 << ", " << lits.lit2;

    return ss.str();
}

/*const GateFinder* Simplifier::getGateFinder() const
{
    return gateFinder;
}*/
