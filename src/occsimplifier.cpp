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


#include "occsimplifier.h"
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
#include "bva.h"
#include "trim.h"

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

OccSimplifier::OccSimplifier(Solver* _solver):
    solver(_solver)
    , seen(solver->seen)
    , seen2(solver->seen2)
    , toClear(solver->toClear)
    , velim_order(VarOrderLt(varElimComplexity))
    , xorFinder(NULL)
    , gateFinder(NULL)
    , anythingHasBeenBlocked(false)
    , blockedMapBuilt(false)
{
    bva = new BVA(solver, this);
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

OccSimplifier::~OccSimplifier()
{
    delete bva;
    delete xorFinder;
    delete subsumeStrengthen;
    delete gateFinder;
}

void OccSimplifier::new_var(const Var /*orig_outer*/)
{
}

void OccSimplifier::new_vars(size_t)
{
}

void OccSimplifier::save_on_var_memory()
{
    clauses.clear();
    clauses.shrink_to_fit();

    cl_to_free_later.shrink_to_fit();

    touched.shrink_to_fit();
    resolvents.shrink_to_fit();
    poss_gate_parts.shrink_to_fit();
    negs_gate_parts.shrink_to_fit();
    blockedClauses.shrink_to_fit();;
}

void OccSimplifier::print_blocked_clauses_reverse() const
{
    for(vector<BlockedClause>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; ++it
    ) {
        if (it->dummy) {
            cout
            << "dummy blocked clause for literal (internal number) " << it->blockedOn
            << endl;
        } else {
            cout
            << "blocked clause (internal number) " << it->lits
            << " blocked on var (internal numbering) "
            << solver->map_outer_to_inter(it->blockedOn.var()) + 1
            << endl;
        }
    }
}

void OccSimplifier::dump_blocked_clauses(std::ostream* outfile) const
{
    for (BlockedClause blocked: blockedClauses) {
        if (blocked.dummy)
            continue;

        //Print info about clause
        *outfile
        << "c next clause is eliminated/blocked on lit "
        << blocked.blockedOn
        << endl;

        //Print clause
        *outfile
        << sortLits(blocked.lits)
        << " 0"
        << endl;
    }
}

void OccSimplifier::extend_model(SolutionExtender* extender)
{
    //Either a variable is not eliminated, or its value is undef
    for(size_t i = 0; i < solver->nVarsOuter(); i++) {
        const Var outer = solver->map_inter_to_outer(i);
        assert(solver->varData[i].removed != Removed::elimed
            || (solver->value(i) == l_Undef && solver->model[outer] == l_Undef)
        );
    }

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "Number of blocked clauses:" << blockedClauses.size() << endl;
    print_blocked_clauses_reverse();
    #endif

    //go through in reverse order
    for (vector<BlockedClause>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; ++it
    ) {
        if (it->toRemove) {
            continue;
        }

        if (it->dummy) {
            extender->dummyBlocked(it->blockedOn);
        } else {
            extender->addClause(it->lits, it->blockedOn);
        }
    }
}

void OccSimplifier::unlink_clause(
    const ClOffset offset
    , bool doDrup
    , bool allow_empty_watch
    , bool only_set_is_removed
) {
    Clause& cl = *solver->cl_alloc.ptr(offset);
    if (solver->drup->enabled() && doDrup) {
       (*solver->drup) << del << cl << fin;
    }

    if (!cl.red()) {
        for (const Lit lit: cl) {
            touched.touch(lit);
        }
    }

    if (!only_set_is_removed) {
        for (const Lit lit: cl) {
            if (!(allow_empty_watch && solver->watches[lit.toInt()].empty())) {
                *limit_to_decrease -= 2*(long)solver->watches[lit.toInt()].size();
                removeWCl(solver->watches[lit.toInt()], offset);
            }
        }
    } else {
        for (const Lit lit: cl) {
            solver->watches.smudge(lit);
        }
        cl.setRemoved();
    }

    if (cl.red()) {
        solver->litStats.redLits -= cl.size();
    } else {
        solver->litStats.irredLits -= cl.size();
    }

    if (!only_set_is_removed) {
        solver->cl_alloc.clauseFree(&cl);
    } else {
        cl_to_free_later.push_back(offset);
    }
}

lbool OccSimplifier::clean_clause(ClOffset offset)
{
    assert(!solver->drup->something_delayed());
    assert(solver->ok);

    bool satisfied = false;
    Clause& cl = *solver->cl_alloc.ptr(offset);
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
    cl.recalc_abst_if_needed();

    //Update lits stat
    if (cl.red()) {
        solver->litStats.redLits -= i-j;
    } else {
        solver->litStats.irredLits -= i-j;
    }

    if (satisfied) {
        #ifdef VERBOSE_DEBUG
        cout << "Clause cleaning -- satisfied, removing" << endl;
        #endif
        (*solver->drup) << findelay;
        unlink_clause(offset, false);
        return l_True;
    }

    if (solver->conf.verbosity >= 6) {
        cout << "-> Clause became after cleaning:" << cl << endl;
    }

    if (i-j > 0) {
        (*solver->drup) << cl << fin << findelay;
    } else {
        solver->drup->forget_delay();
    }

    switch(cl.size()) {
        case 0:
            unlink_clause(offset, false);
            solver->ok = false;
            return l_False;

        case 1:
            solver->enqueue(cl[0]);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
            unlink_clause(offset, false);
            return l_True;

        case 2:
            solver->attach_bin_clause(cl[0], cl[1], cl.red());
            unlink_clause(offset, false);
            return l_True;

        case 3:
            solver->attach_tri_clause(cl[0], cl[1], cl[2], cl.red());
            unlink_clause(offset, false);
            return l_True;

        default:
            cl.setStrenghtened();
            cl.recalc_abst_if_needed();
            return l_Undef;
    }
}

uint64_t OccSimplifier::calc_mem_usage_of_occur(const vector<ClOffset>& toAdd) const
{
     uint64_t memUsage = 0;
    for (vector<ClOffset>::const_iterator
        it = toAdd.begin(), end = toAdd.end()
        ; it !=  end
        ; ++it
    ) {
        Clause* cl = solver->cl_alloc.ptr(*it);
        //*2 because of the overhead of allocation
        memUsage += cl->size()*sizeof(Watched)*2;
    }

    //Estimate malloc overhead
    memUsage += solver->num_active_vars()*2*40;

    return memUsage;
}

void OccSimplifier::print_mem_usage_of_occur(bool irred, uint64_t memUsage) const
{
    if (solver->conf.verbosity >= 2) {
        cout
        << "c [simp] mem usage for occur of "
        << (irred ?  "irred" : "red  ")
        << " " << std::setw(6) << memUsage/(1024ULL*1024ULL) << " MB"
        << endl;
    }
}

void OccSimplifier::print_linkin_data(const LinkInData link_in_data) const
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


OccSimplifier::LinkInData OccSimplifier::link_in_clauses(
    const vector<ClOffset>& toAdd
    , bool irred
    , bool alsoOccur
) {
    LinkInData link_in_data;
    uint64_t linkedInLits = 0;
    for (const ClOffset offs: toAdd) {
        Clause* cl = solver->cl_alloc.ptr(offs);

        //Sanity check that the value given as irred is correct
        assert(
            (irred && !cl->red())
            || (!irred && cl->red())
        );
        cl->recalc_abst_if_needed();
        assert(cl->abst == calcAbstraction(*cl));

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
            cl->set_occur_linked(false);
            link_in_data.cl_not_linked++;
        }

        clauses.push_back(offs);
    }
    clause_lits_added += linkedInLits;

    return link_in_data;
}

bool OccSimplifier::decide_occur_limit(bool irred, uint64_t memUsage)
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

bool OccSimplifier::add_from_solver(
    vector<ClOffset>& toAdd
    , bool alsoOccur
    , bool irred
) {
    //solver->print_watch_mem_used();

    if (alsoOccur) {
        uint64_t memUsage = calc_mem_usage_of_occur(toAdd);
        print_mem_usage_of_occur(irred, memUsage);
        alsoOccur = decide_occur_limit(irred, memUsage);
        if (irred && !alsoOccur)
            return false;
    }

    if (!irred && alsoOccur) {
        std::sort(toAdd.begin(), toAdd.end(), ClauseSizeSorter(solver->cl_alloc));
    }

    LinkInData link_in_data = link_in_clauses(toAdd, irred, alsoOccur);
    toAdd.clear();
    if (!irred)
        print_linkin_data(link_in_data);

    return true;
}

bool OccSimplifier::check_varelim_when_adding_back_cl(const Clause* cl) const
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
        ) {
            std::cerr
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

void OccSimplifier::add_back_to_solver()
{
    for (vector<ClOffset>::const_iterator
        it = clauses.begin(), end = clauses.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = solver->cl_alloc.ptr(*it);
        if (cl->freed())
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
            solver->cl_alloc.clauseFree(cl);
            continue;
        }

        if (complete_clean_clause(*cl)) {
            solver->attachClause(*cl);
            if (cl->red()) {
                solver->longRedCls.push_back(*it);
            } else {
                solver->longIrredCls.push_back(*it);
            }
        } else {
            solver->cl_alloc.clauseFree(cl);
        }
    }
}

bool OccSimplifier::complete_clean_clause(Clause& cl)
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
    cl.recalc_abst_if_needed();

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
            solver->attach_bin_clause(cl[0], cl[1], cl.red());
            return false;

        case 3:
            solver->attach_tri_clause(cl[0], cl[1], cl[2], cl.red());
            return false;

        default:
            return true;
    }
}

void OccSimplifier::remove_all_longs_from_watches()
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

void OccSimplifier::eliminate_empty_resolvent_vars()
{
    uint32_t var_elimed = 0;
    double myTime = cpuTime();
    const int64_t orig_empty_varelim_time_limit = empty_varelim_time_limit;
    limit_to_decrease = &empty_varelim_time_limit;
    assert(cl_to_free_later.empty());

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
        if (!check_empty_resolvent(lit))
            continue;

        create_dummy_blocked_clause(lit);
        rem_cls_from_watch_due_to_varelim(solver->watches[lit.toInt()], lit);
        rem_cls_from_watch_due_to_varelim(solver->watches[(~lit).toInt()], ~lit);
        set_var_as_eliminated(var, lit);
        var_elimed++;
    }

    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain = (double)*limit_to_decrease/(double)orig_empty_varelim_time_limit;
    if (solver->conf.verbosity >= 2) {
        cout
        << "c Empty resolvent elimed: " << var_elimed
        << solver->conf.print_times(time_used, time_out)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "empty resolvent"
            , time_used
            , time_out
            , time_remain
        );
    }
}

bool OccSimplifier::can_eliminate_var(const Var var) const
{
    if (solver->value(var) != l_Undef
        || solver->varData[var].removed != Removed::none
        ||  solver->var_inside_assumptions(var)
    ) {
        return false;
    }

    return true;
}

bool OccSimplifier::eliminate_vars()
{
    //Set-up
    double myTime = cpuTime();
    size_t vars_elimed = 0;
    size_t wenThrough = 0;
    time_spent_on_calc_otf_update = 0;
    num_otf_update_until_now = 0;
    int64_t orig_norm_varelim_time_limit = norm_varelim_time_limit;
    limit_to_decrease = &norm_varelim_time_limit;
    cl_to_free_later.clear();
    assert(solver->watches.get_smudged_list().empty());

    order_vars_for_elim();

    //Go through the ordered list of variables to eliminate
    while(!velim_order.empty()
        && *limit_to_decrease > 0
        && varelim_num_limit > 0
        && !solver->must_interrupt_asap()
    ) {
        assert(limit_to_decrease == &norm_varelim_time_limit);
        Var var = velim_order.remove_min();

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
        if (maybe_eliminate(var)) {
            vars_elimed++;
            varelim_num_limit--;
        }
        if (!solver->ok)
            goto end;
    }

end:
    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain = (double)*limit_to_decrease/(double)orig_norm_varelim_time_limit;

    if (solver->conf.verbosity >= 2) {
        cout
        << "c  #try to eliminate: " << wenThrough << endl
        << "c  #var-elim: " << vars_elimed << endl
        << "c  #T-o: " << (time_out ? "Y" : "N") << endl
        << "c  #T-r: " << std::fixed << std::setprecision(2) << (time_remain*100.0) << "%" << endl
        << "c  #T: " << time_used << endl;
    }
    if (solver->sqlStats) {
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

void OccSimplifier::free_clauses_to_free()
{
    for(ClOffset off: cl_to_free_later) {
        Clause* cl = solver->cl_alloc.ptr(off);
        solver->cl_alloc.clauseFree(cl);
    }
    cl_to_free_later.clear();
}

bool OccSimplifier::fill_occur_and_print_stats()
{
    double myTime = cpuTime();
    remove_all_longs_from_watches();
    if (!fill_occur()) {
        return false;
    }
    sanityCheckElimedVars();
    const double linkInTime = cpuTime() - myTime;
    runStats.linkInTime += linkInTime;
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "occur build"
            , linkInTime
        );
    }

    //Print memory usage after occur link-in
    if (solver->conf.verbosity >= 2) {
        double vm_usage = 0;
        solver->print_watch_mem_used(memUsedTotal(vm_usage));
    }

    return true;
}

bool OccSimplifier::execute_simplifier_sched(const string& strategy)
{
    std::istringstream ss(strategy);
    std::string token;

    while(std::getline(ss, token, ',')) {
        if (cpuTime() > solver->conf.maxTime
            || solver->must_interrupt_asap()
            || solver->nVars() == 0
            || !solver->ok
        ) {
            return solver->ok;
        }

        trim(token);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (solver->conf.verbosity >= 2) {
            cout << "c --> Executing OCC strategy token: " << token << '\n';
        }
        if (token == "backw-subsume") {
            backward_subsume();
        } else if (token == "xor") {
            #ifdef USE_M4RI
            if (solver->conf.doFindXors
                && xorFinder != NULL
            ) {
                xorFinder->do_all_with_xors();
            }
            #endif
        } else if (token == "prop") {
            solver->propagate_occur();
        } else if (token == "clean-implicit") {
            solver->clauseCleaner->clean_implicit_clauses();
        } else if (token == "bve") {
            if (solver->conf.doVarElim && solver->conf.do_empty_varelim) {
                eliminate_empty_resolvent_vars();
                eliminate_vars();
            }
        }
        else if (token == "bva") {
            bva->bounded_var_addition();
        } else if (token == "gates") {
            if (solver->conf.doCache
                && solver->conf.doGateFind
            ) {
                gateFinder->doAll();
            }
        }
        else if (token == "") {
            //nothing, ignore empty token
        } else {
             cout << "ERROR: occur strategy " << token << " not recognised!" << endl;
            exit(-1);
        }
    }

    return solver->ok;
}


bool OccSimplifier::simplify(const bool _startup)
{
    startup = _startup;
    assert(solver->okay());
    assert(toClear.empty());

    //Test & debug
    solver->test_all_clause_attached();
    solver->check_wrong_attach();

    //Clean the clauses before playing with them
    solver->clauseCleaner->remove_and_clean_all();

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
    set_limits(); //to calculate strengthening_time_limit
    limit_to_decrease = &strengthening_time_limit;
    if (!fill_occur_and_print_stats()) {
        return solver->okay();
    }

    set_limits();
    runStats.origNumFreeVars = solver->get_num_free_vars();
    const size_t origBlockedSize = blockedClauses.size();
    const size_t origTrailSize = solver->trail_size();

    //subsumeStrengthen->subsumeWithTris();
    if (startup) {
        execute_simplifier_sched(solver->conf.occsimp_schedule_startup);
    } else {
        execute_simplifier_sched(solver->conf.occsimp_schedule_nonstartup);
    }

end:

    remove_by_drup_recently_blocked_clauses(origBlockedSize);
    finishUp(origTrailSize);

    //Print stats
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.print_short(solver, solver->conf.doVarElim);
    }

    return solver->ok;
}

bool OccSimplifier::backward_subsume()
{
    assert(cl_to_free_later.empty());
    assert(solver->watches.get_smudged_list().empty());
    bool ret = true;

    subsumeStrengthen->backward_subsumption_long_with_long();
    if (!subsumeStrengthen->backward_strengthen_long_with_long()
        || solver->must_interrupt_asap()
    ) {
        ret = false;
    }

    free_clauses_to_free();
    solver->clean_occur_from_removed_clauses_only_smudged();

    return ret;
}

bool OccSimplifier::fill_occur()
{
    //Try to add irreducible to occur
    runStats.origNumIrredLongClauses = solver->longIrredCls.size();
    bool ret = add_from_solver(solver->longIrredCls
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
    add_from_solver(solver->longRedCls
        , true //try to add to occur list
        , false //irreduntant?
    );

    return true;
}

//This must NEVER be called during solve. Only JUST BEFORE Solver::solve() is called
//otherwise, uneliminated_vars_since_last_solve will be wrong and stamp dominators will not be cleared
bool OccSimplifier::uneliminate(Var var)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->okay());

    //Check that it was really eliminated
    //NOTE: it's already been made a decision var, as the variable has been re-added already
    solver->set_decision_var(var);
    assert(solver->varData[var].removed == Removed::elimed);
    assert(solver->value(var) == l_Undef);

    if (!blockedMapBuilt) {
        cleanBlockedClauses();
        buildBlockedMap();
    }

    //Uneliminate it in theory
    globalStats.numVarsElimed--;
    solver->varData[var].removed = Removed::none;
    solver->set_decision_var(var);

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

void OccSimplifier::remove_by_drup_recently_blocked_clauses(size_t origBlockedSize)
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
            ; ++it
        ) {
            (*solver->drup) << *it;
        }
        (*solver->drup) << fin;
    }
}

void OccSimplifier::buildBlockedMap()
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

void OccSimplifier::finishUp(
    size_t origTrailSize
) {
    bool somethingSet = (solver->trail_size() - origTrailSize) > 0;
    runStats.zeroDepthAssings = solver->trail_size() - origTrailSize;
    const double myTime = cpuTime();

    //Add back clauses to solver
    solver->propagate_occur();
    remove_all_longs_from_watches();
    add_back_to_solver();
    solver->propagate_occur();
    if (solver->ok) {
        solver->clauseCleaner->remove_and_clean_all();
    }

    //Update global stats
    const double time_used = cpuTime() - myTime;
    runStats.finalCleanupTime += time_used;
    if (solver->sqlStats) {
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
        solver->test_all_clause_attached();
        solver->check_wrong_attach();
        solver->check_stats();
        solver->check_implicit_propagated();
    }

    if (solver->ok) {
        check_elimed_vars_are_unassignedAndStats();
    }
}

void OccSimplifier::sanityCheckElimedVars()
{
    //First, sanity-check the long clauses
    for (vector<ClOffset>::const_iterator
        it =  clauses.begin(), end = clauses.end()
        ; it != end
        ; ++it
    ) {
        const Clause* cl = solver->cl_alloc.ptr(*it);

        //Already removed
        if (cl->freed())
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

void OccSimplifier::set_limits()
{
    subsumption_time_limit     = 850LL*1000LL*solver->conf.subsumption_time_limitM
        *solver->conf.global_timeout_multiplier;
    strengthening_time_limit   = 400LL*1000LL*solver->conf.strengthening_time_limitM
        *solver->conf.global_timeout_multiplier;
    norm_varelim_time_limit    = 4ULL*1000LL*1000LL*solver->conf.varelim_time_limitM
        *solver->conf.global_timeout_multiplier;
    empty_varelim_time_limit   = 200LL*1000LL*solver->conf.empty_varelim_time_limitM
        *solver->conf.global_timeout_multiplier;
    aggressive_elim_time_limit = 300LL *1000LL*solver->conf.aggressive_elim_time_limitM
        *solver->conf.global_timeout_multiplier;

    //numMaxElim = 0;
    //numMaxElim = std::numeric_limits<int64_t>::max();

    //If variable elimination isn't going so well
    if (globalStats.testedToElimVars > 0
        && (double)globalStats.numVarsElimed/(double)globalStats.testedToElimVars < 0.1
    ) {
        norm_varelim_time_limit /= 2;
    }

    #ifdef BIT_MORE_VERBOSITY
    cout << "c clause_lits_added: " << clause_lits_added << endl;
    #endif

    //if (clause_lits_added < 10ULL*1000ULL*1000ULL) {
        norm_varelim_time_limit *= 2;
        empty_varelim_time_limit *= 2;
        subsumption_time_limit *= 2;
        strengthening_time_limit *= 2;
    //}

    //if (clause_lits_added < 3ULL*1000ULL*1000ULL) {
        norm_varelim_time_limit *= 2;
        empty_varelim_time_limit *= 2;
        subsumption_time_limit *= 2;
        strengthening_time_limit *= 2;
    //}

    varelim_num_limit = ((double)solver->get_num_free_vars() * solver->conf.varElimRatioPerIter);
    if (globalStats.numCalls > 0) {
        varelim_num_limit = (double)varelim_num_limit * (globalStats.numCalls+0.5);
    }
    runStats.origNumMaxElimVars = varelim_num_limit;

    if (!solver->conf.do_strengthen_with_occur) {
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

void OccSimplifier::cleanBlockedClauses()
{
    assert(solver->decisionLevel() == 0);
    vector<BlockedClause>::iterator i = blockedClauses.begin();
    vector<BlockedClause>::iterator j = blockedClauses.begin();

    for (vector<BlockedClause>::iterator
        end = blockedClauses.end()
        ; i != end
        ; i++
    ) {
        const Var blockedOn = solver->map_outer_to_inter(i->blockedOn.var());
        if (solver->varData[blockedOn].removed == Removed::elimed
            && solver->value(blockedOn) != l_Undef
        ) {
            std::cerr
            << "ERROR: lit " << *i << " elimed,"
            << " value: " << solver->value(blockedOn)
            << endl;
            assert(false);
            std::exit(-1);
        }

        if (i->toRemove) {
            blockedMapBuilt = false;
        } else {
            assert(solver->varData[blockedOn].removed == Removed::elimed);
            *j++ = *i;
        }
    }
    blockedClauses.resize(blockedClauses.size()-(i-j));
}

size_t OccSimplifier::rem_cls_from_watch_due_to_varelim(
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
            const ClOffset offset = watch.get_offset();
            const Clause& cl = *solver->cl_alloc.ptr(offset);
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
            unlink_clause(offset, cl.red(), true, true);
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
                touched.touch(lits[1]);
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
            solver->detach_bin_clause(lits[0], lits[1], red, true);
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
                touched.touch(lits[1]);
                touched.touch(lits[2]);
            } else {
                //If redundant, delayed blocked-based DRUP deletion will not work
                //so delete explicitly
                (*solver->drup) << del << lits[0] << lits[1] << lits[2] << fin;
            }

            //Remove
            *limit_to_decrease -= (long)solver->watches[lits[0].toInt()].size();
            *limit_to_decrease -= (long)solver->watches[lits[1].toInt()].size();
            *limit_to_decrease -= (long)solver->watches[lits[2].toInt()].size();
            solver->detach_tri_clause(lits[0], lits[1], lits[2], watch.red(), true);
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

void OccSimplifier::add_clause_to_blck(const Lit lit, const vector<Lit>& lits)
{
    const Lit lit_outer = solver->map_inter_to_outer(lit);
    vector<Lit> lits_outer = lits;
    solver->map_inter_to_outer(lits_outer);
    blockedClauses.push_back(BlockedClause(lit_outer, lits_outer));
}

bool OccSimplifier::find_gate(
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
            const Clause* cl = solver->cl_alloc.ptr(w.get_offset());
            if (cl->getRemoved()) {
                continue;
            }

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

void OccSimplifier::mark_gate_in_poss_negs(
    Lit elim_lit
    , watch_subarray_const poss
    , watch_subarray_const negs
) {
    gate_lits_of_elim_cls.clear();

    //Either of the two is OK
    find_gate(elim_lit, poss, negs);
    bool gate_found_elim_pos = find_gate(~elim_lit, negs, poss);

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

void OccSimplifier::mark_gate_parts(
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
            const Clause* cl = solver->cl_alloc.ptr(w.get_offset());
            if (cl->getRemoved()) {
                continue;
            }

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

bool OccSimplifier::skip_resolution_thanks_to_gate(
    const size_t at_poss
    , const size_t at_negs
) const {
    if (!gate_found_elim)
        return false;

    return poss_gate_parts[at_poss] == negs_gate_parts[at_negs];
}

int OccSimplifier::test_elim_and_fill_resolvents(const Var var)
{
    assert(solver->ok);
    assert(solver->varData[var].removed == Removed::none);
    assert(solver->value(var) == l_Undef);

    //Gather data
    HeuristicData pos = calc_data_for_heuristic(Lit(var, false));
    HeuristicData neg = calc_data_for_heuristic(Lit(var, true));

    //Heuristic calculation took too much time
    if (*limit_to_decrease < 0) {
        return std::numeric_limits<int>::max();
    }

    //Check if we should do aggressive check or not
    const bool aggressive = (aggressive_elim_time_limit > 0 && !startup);
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
    if ((neg.totalCls() * pos.totalCls())
        >= solver->conf.varelim_cutoff_too_many_clauses
    ) {
        return std::numeric_limits<int>::max();
    }

    gate_found_elim = false;
    if (solver->conf.skip_some_bve_resolvents) {
        //mark_gate_in_poss_negs(lit, poss, negs);
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
        ; ++it, at_poss++
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
                //Too long resolvent
                || (solver->conf.velim_resolvent_too_large != -1
                    && ((int)dummy.size() > solver->conf.velim_resolvent_too_large))
                //Over-time
                || *limit_to_decrease < -10LL*1000LL

            ) {
                return std::numeric_limits<int>::max();
            }

            //Calculate new clause stats
            ClauseStats stats;
            if ((it->isBinary() || it->isTri()) && it2->isClause())
                stats = solver->cl_alloc.ptr(it2->get_offset())->stats;
            else if ((it2->isBinary() || it2->isTri()) && it->isClause())
                stats = solver->cl_alloc.ptr(it->get_offset())->stats;
            else if (it->isClause() && it2->isClause())
                stats = ClauseStats::combineStats(
                    solver->cl_alloc.ptr(it->get_offset())->stats
                    , solver->cl_alloc.ptr(it2->get_offset())->stats
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

void OccSimplifier::printOccur(const Lit lit) const
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
            << *solver->cl_alloc.ptr(w.get_offset())
            << "(red: " << solver->cl_alloc.ptr(w.get_offset())->red()
            << ")"
            << endl;
        }
    }
}

void OccSimplifier::print_var_eliminate_stat(const Lit lit) const
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

bool OccSimplifier::check_if_new_2_long_subsumes_3_long_return_already_inside(const vector<Lit>& lits_orig)
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

bool OccSimplifier::add_varelim_resolvent(
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

    newCl = solver->add_clause_int(
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
        ClOffset offset = solver->cl_alloc.get_offset(newCl);
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

void OccSimplifier::try_to_subsume_with_new_bin_or_tri(const vector<Lit>& lits)
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

void OccSimplifier::update_varelim_complexity_heap(const Var var)
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
            || !velim_order.in_heap(touchVar)
            || solver->value(touchVar) != l_Undef
            || solver->varData[touchVar].removed != Removed::none
        ) {
            continue;
        }

        varElimComplexity[touchVar] = strategyCalcVarElimScore(touchVar);
        velim_order.update_if_inside(touchVar);
    }
    time_spent_on_calc_otf_update += limit_before - *limit_to_decrease;
}

void OccSimplifier::print_var_elim_complexity_stats(const Var var) const
{
    if (solver->conf.verbosity < 5)
        return;

    cout << "trying complexity: "
    << varElimComplexity[var].first
    << ", " << varElimComplexity[var].second
    << endl;
}

void OccSimplifier::set_var_as_eliminated(const Var var, const Lit lit)
{
    if (solver->conf.verbosity >= 5) {
        cout << "Elimination of var "
        <<  solver->map_inter_to_outer(lit)
        << " finished " << endl;
    }
    assert(solver->varData[var].removed == Removed::none);
    solver->varData[var].removed = Removed::elimed;

    runStats.numVarsElimed++;

    solver->unset_decision_var(var);
}

void OccSimplifier::create_dummy_blocked_clause(const Lit lit)
{
    blockedClauses.push_back(
        BlockedClause(solver->map_inter_to_outer(lit))
    );
}

bool OccSimplifier::maybe_eliminate(const Var var)
{
    assert(solver->ok);
    print_var_elim_complexity_stats(var);
    runStats.testedToElimVars++;

    //Heuristic says no, or we ran out of time
    if (test_elim_and_fill_resolvents(var) == std::numeric_limits<int>::max()
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

/*void OccSimplifier::addRedBinaries(const Var var)
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
            Clause* tmpOK = solver->add_clause_int(tmp, true);
            runStats.numRedBinVarRemAdded++;
            release_assert(tmpOK == NULL);
            release_assert(solver->ok);
        }
    }
    assert(solver->value(lit) == l_Undef);
}*/

void OccSimplifier::add_pos_lits_to_dummy_and_seen(
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
        Clause& cl = *solver->cl_alloc.ptr(ps.get_offset());
        *limit_to_decrease -= (long)cl.size();
        for (const Lit lit : cl){
            if (lit != posLit) {
                seen[lit.toInt()] = 1;
                dummy.push_back(lit);
            }
        }
    }
}

bool OccSimplifier::add_neg_lits_to_dummy_and_seen(
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
        Clause& cl = *solver->cl_alloc.ptr(qs.get_offset());
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

bool OccSimplifier::reverse_distillation_of_dummy(
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
        aggressive_elim_time_limit -= (int64_t)cache.size()/3;
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

bool OccSimplifier::subsume_dummy_through_stamping(
    const Watched ps
    , const Watched qs
) {
    //only if none of the clauses were binary
    //Otherwise we cannot tell if the value in the cache is dependent
    //on the binary clause itself, so that would cause a circular de-
    //pendency

    if (!ps.isBinary() && !qs.isBinary()) {
        aggressive_elim_time_limit -= (int64_t)toClear.size()*5;
        if (solver->stamp.stampBasedClRem(toClear)) {
            return true;
        }
    }

    return false;
}

bool OccSimplifier::resolve_clauses(
    const Watched ps
    , const Watched qs
    , const Lit posLit
    , const bool aggressive
) {
    //If clause has already been freed, skip
    if (ps.isClause()
        && solver->cl_alloc.ptr(ps.get_offset())->freed()
    ) {
        return false;
    }
    if (qs.isClause()
        && solver->cl_alloc.ptr(qs.get_offset())->freed()
    ) {
        return false;
    }

    dummy.clear();
    assert(toClear.empty());
    add_pos_lits_to_dummy_and_seen(ps, posLit);
    bool tautological = add_neg_lits_to_dummy_and_seen(qs, posLit);
    toClear = dummy;

    if (!tautological && aggressive) {
        tautological = reverse_distillation_of_dummy(ps, qs, posLit);
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

bool OccSimplifier::aggressiveCheck(
    const Lit lit
    , const Lit noPosLit
    , bool& retval
) {
    watch_subarray_const ws = solver->watches[lit.toInt()];
    aggressive_elim_time_limit -= (int64_t)ws.size()/3 + 2;
    for(watch_subarray::const_iterator it =
        ws.begin(), end = ws.end()
        ; it != end
        ; ++it
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

OccSimplifier::HeuristicData OccSimplifier::calc_data_for_heuristic(const Lit lit)
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
                const Clause* cl = solver->cl_alloc.ptr(ws.get_offset());
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

bool OccSimplifier::check_empty_resolvent(Lit lit)
{
    //Take the smaller of the two
    if (solver->watches[(~lit).toInt()].size() < solver->watches[lit.toInt()].size())
        lit = ~lit;

    int num_bits_set = check_empty_resolvent_action(
        lit
        , ResolvCount::set
        , 0
    );

    int num_resolvents = std::numeric_limits<int>::max();

    //Can only count if the POS was small enough
    //otherwise 'seen' cannot properly store the data
    if (num_bits_set < 16) {
        num_resolvents = check_empty_resolvent_action(
            ~lit
            , ResolvCount::count
            , num_bits_set
        );
    }

    //Clear the 'seen' array
    check_empty_resolvent_action(
        lit
        , ResolvCount::unset
        , 0
    );

    //Okay, this would be great
    return (num_resolvents == 0);
}


int OccSimplifier::check_empty_resolvent_action(
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
            const Clause* cl = solver->cl_alloc.ptr(ws.get_offset());
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



pair<int, int> OccSimplifier::heuristicCalcVarElimScore(const Var var)
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
            normCost =
                  posTotalLonger + negTotalLonger + pos.bin + neg.bin;
            break;

        case 1:
            normCost =  posTotalLonger*negTotalLonger
                + pos.bin*negTotalLonger*2
                + neg.bin*posTotalLonger*2
                + pos.bin*neg.bin*3;
            break;

        default:
            std::cerr
            << "ERROR: Invalid var-elim cost estimation strategy"
            << endl;
            std::exit(-1);
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

void OccSimplifier::order_vars_for_elim()
{
    velim_order.clear();
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
        assert(!velim_order.in_heap(var));
        varElimComplexity[var] = strategyCalcVarElimScore(var);
        velim_order.insert(var);
    }
    assert(velim_order.heap_property());

    //Print sorted listed list
    #ifdef VERBOSE_DEBUG_VARELIM
    /*cout << "-----------" << endl;
    for(size_t i = 0; i < velim_order.size(); i++) {
        cout
        << "velim_order[" << i << "]: "
        << " var: " << velim_order[i]+1
        << " val: " << varElimComplexity[velim_order[i]].first
        << " , " << varElimComplexity[velim_order[i]].second
        << endl;
    }*/
    #endif
}

std::pair<int, int> OccSimplifier::strategyCalcVarElimScore(const Var var)
{
    std::pair<int, int> cost;
    if (solver->conf.var_elim_strategy == ElimStrategy::heuristic) {
        cost = heuristicCalcVarElimScore(var);
    } else {
        int ret = test_elim_and_fill_resolvents(var);

        cost.first = ret;
        cost.second = 0;
    }

    return cost;
}

void OccSimplifier::check_elimed_vars_are_unassigned() const
{
    for (size_t i = 0; i < solver->nVarsOuter(); i++) {
        if (solver->varData[i].removed == Removed::elimed) {
            assert(solver->value(i) == l_Undef);
        }
    }
}

void OccSimplifier::check_elimed_vars_are_unassignedAndStats() const
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
        std::cerr
        << "ERROR: globalStats.numVarsElimed is "<<
        globalStats.numVarsElimed
        << " but checkNumElimed is: " << checkNumElimed
        << endl;

        assert(false);
    }
}

size_t OccSimplifier::mem_used() const
{
    size_t b = 0;
    b += poss_gate_parts.capacity()*sizeof(char);
    b += negs_gate_parts.capacity()*sizeof(char);
    b += gate_lits_of_elim_cls.capacity()*sizeof(Lit);
    b += dummy.capacity()*sizeof(char);
    b += subsumeStrengthen->mem_used();
    for(map<Var, vector<size_t> >::const_iterator
        it = blk_var_to_cl.begin(), end = blk_var_to_cl.end()
        ; it != end
        ; ++it
    ) {
        b += it->second.capacity()*sizeof(size_t);
    }
    b += blockedClauses.capacity()*sizeof(BlockedClause);
    for(vector<BlockedClause>::const_iterator
        it = blockedClauses.begin(), end = blockedClauses.end()
        ; it != end
        ; ++it
    ) {
        b += it->lits.capacity()*sizeof(Lit);
    }
    b += blk_var_to_cl.size()*(sizeof(Var)+sizeof(vector<size_t>)); //TODO under-counting
    b += velim_order.mem_used();
    b += varElimComplexity.capacity()*sizeof(int)*2;
    b += touched.mem_used();
    b += clauses.capacity()*sizeof(ClOffset);

    return b;
}

size_t OccSimplifier::mem_used_xor() const
{
    if (xorFinder)
        return xorFinder->mem_used();
    else
        return 0;
}

void OccSimplifier::freeXorMem()
{
    delete xorFinder;
    xorFinder = NULL;
}

void OccSimplifier::linkInClause(Clause& cl)
{
    assert(cl.size() > 3);
    ClOffset offset = solver->cl_alloc.get_offset(&cl);
    cl.recalc_abst_if_needed();

    std::sort(cl.begin(), cl.end());
    for (const Lit lit: cl) {
        watch_subarray ws = solver->watches[lit.toInt()];
        *limit_to_decrease -= (long)ws.size();

        ws.push(Watched(offset, cl.abst));
    }
    cl.set_occur_linked(true);
}


void OccSimplifier::print_gatefinder_stats() const
{
    if (gateFinder) {
        gateFinder->get_stats().print(solver->nVarsOuter());
    }
}

double OccSimplifier::Stats::total_time() const
{
    return linkInTime + blockTime
        + varElimTime + finalCleanupTime;
}

void OccSimplifier::Stats::clear()
{
    Stats stats;
    *this = stats;
}

OccSimplifier::Stats& OccSimplifier::Stats::operator+=(const Stats& other)
{
    numCalls += other.numCalls;

    //Time
    linkInTime += other.linkInTime;
    blockTime += other.blockTime;
    varElimTime += other.varElimTime;
    finalCleanupTime += other.finalCleanupTime;

    //Startup stats
    origNumFreeVars += other.origNumFreeVars;
    origNumMaxElimVars += other.origNumMaxElimVars;
    origNumIrredLongClauses += other.origNumIrredLongClauses;
    origNumRedLongClauses += other.origNumRedLongClauses;

    //Each algo
    subsumedByVE  += other.subsumedByVE;

    //Elim
    numVarsElimed += other.numVarsElimed;
    varElimTimeOut += other.varElimTimeOut;
    clauses_elimed_long += other.clauses_elimed_long;
    clauses_elimed_tri += other.clauses_elimed_tri;
    clauses_elimed_bin += other.clauses_elimed_bin;
    clauses_elimed_sumsize += other.clauses_elimed_sumsize;
    longRedClRemThroughElim += other.longRedClRemThroughElim;
    triRedClRemThroughElim += other.triRedClRemThroughElim;
    binRedClRemThroughElim += other.binRedClRemThroughElim;
    numRedBinVarRemAdded += other.numRedBinVarRemAdded;
    testedToElimVars += other.testedToElimVars;
    triedToElimVars += other.triedToElimVars;
    usedAggressiveCheckToELim += other.usedAggressiveCheckToELim;
    newClauses += other.newClauses;

    zeroDepthAssings += other.zeroDepthAssings;

    return *this;
}

void OccSimplifier::Stats::print_short(const Solver* solver, const bool print_var_elim) const
{

    cout
    << "c [occur] " << linkInTime+finalCleanupTime << " is overhead"
    << endl;

    //About elimination
    if (print_var_elim) {
        cout
        << "c [v-elim]"
        << " elimed: " << numVarsElimed
        << " / " << origNumMaxElimVars
        << " / " << origNumFreeVars
        //<< " cl-elim: " << (clauses_elimed_long+clauses_elimed_bin)
        << solver->conf.print_times(varElimTime, varElimTimeOut)
        << endl;

        cout
        << "c [v-elim]"
        << " cl-new: " << newClauses
        << " tried: " << triedToElimVars
        << " tested: " << testedToElimVars
        << " ("
        << stats_line_percent(usedAggressiveCheckToELim, testedToElimVars)
        << " % aggressive)"
        << endl;

        cout
        << "c [v-elim]"
        << " subs: "  << subsumedByVE
        << " red-bin rem: " << binRedClRemThroughElim
        << " red-tri rem: " << triRedClRemThroughElim
        << " red-long rem: " << longRedClRemThroughElim
        << " v-fix: " << std::setw(4) << zeroDepthAssings
        << endl;
    }

    cout
    << "c [simp] link-in T: " << linkInTime
    << " cleanup T: " << finalCleanupTime
    << endl;
}

void OccSimplifier::Stats::print(const size_t nVars) const
{
    cout << "c -------- OccSimplifier STATS ----------" << endl;
    print_stats_line("c time"
        , total_time()
        , stats_line_percent(varElimTime, total_time())
        , "% var-elim"
    );

    print_stats_line("c timeouted"
        , stats_line_percent(varElimTimeOut, numCalls)
        , "% called"
    );

    print_stats_line("c called"
        ,  numCalls
        , (double)total_time()/(double)numCalls
        , "s per call"
    );

    print_stats_line("c v-elimed"
        , numVarsElimed
        , stats_line_percent(numVarsElimed, nVars)
        , "% vars"
    );

    cout << "c"
    << " v-elimed: " << numVarsElimed
    << " / " << origNumMaxElimVars
    << " / " << origNumFreeVars
    << endl;

    print_stats_line("c 0-depth assigns"
        , zeroDepthAssings
        , stats_line_percent(zeroDepthAssings, nVars)
        , "% vars"
    );

    print_stats_line("c cl-new"
        , newClauses
    );

    print_stats_line("c tried to elim"
        , triedToElimVars
        , stats_line_percent(usedAggressiveCheckToELim, triedToElimVars)
        , "% aggressively"
    );

    print_stats_line("c elim-bin-lt-cl"
        , binRedClRemThroughElim);

    print_stats_line("c elim-tri-lt-cl"
        , triRedClRemThroughElim);

    print_stats_line("c elim-long-lt-cl"
        , longRedClRemThroughElim);

    print_stats_line("c lt-bin added due to v-elim"
        , numRedBinVarRemAdded);

    print_stats_line("c cl-elim-bin"
        , clauses_elimed_bin);

    print_stats_line("c cl-elim-tri"
        , clauses_elimed_tri);

    print_stats_line("c cl-elim-long"
        , clauses_elimed_long);

    print_stats_line("c cl-elim-avg-s",
        ((double)clauses_elimed_sumsize
        /(double)(clauses_elimed_bin + clauses_elimed_tri + clauses_elimed_long))
    );

    print_stats_line("c v-elim-sub"
        , subsumedByVE
    );

    cout << "c -------- OccSimplifier STATS END ----------" << endl;
}
