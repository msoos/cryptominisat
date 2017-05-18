/******************************************
Copyright (c) 2016, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "time_mem.h"
#include <cassert>
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


#include "popcnt.h"
#include "occsimplifier.h"
#include "clause.h"
#include "solver.h"
#include "clausecleaner.h"
#include "constants.h"
#include "solutionextender.h"
#include "varreplacer.h"
#include "varupdatehelper.h"
#include "completedetachreattacher.h"
#include "subsumestrengthen.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "toplevelgaussabst.h"
#include "subsumeimplicit.h"
#include "sqlstats.h"
#include "datasync.h"
#include "xorfinder.h"
#include "bva.h"
#include "trim.h"

#ifdef USE_M4RI
#include "toplevelgauss.h"
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
    , topLevelGauss(NULL)
    //, gateFinder(NULL)
    , anythingHasBeenBlocked(false)
    , blockedMapBuilt(false)
{
    bva = new BVA(solver, this);
    topLevelGauss = new TopLevelGaussAbst;
    #ifdef USE_M4RI
    delete topLevelGauss;
    topLevelGauss = new TopLevelGauss(solver);
    #endif
    sub_str = new SubsumeStrengthen(this, solver);

    if (solver->conf.doGateFind) {
        //gateFinder = new GateFinder(this, solver);
    }
}

OccSimplifier::~OccSimplifier()
{
    delete bva;
    delete topLevelGauss;
    delete sub_str;
    //delete gateFinder;
}

void OccSimplifier::new_var(const uint32_t /*orig_outer*/)
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
        const uint32_t outer = solver->map_inter_to_outer(i);
        assert(solver->varData[i].removed != Removed::elimed
            || (solver->value(i) == l_Undef && solver->model_value(outer) == l_Undef)
        );
    }

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "Number of blocked clauses:" << blockedClauses.size() << endl;
    print_blocked_clauses_reverse();
    #endif

    //go through in reverse order
    for (int i = (int)blockedClauses.size()-1; i >= 0; i--) {
        BlockedClause* it = &blockedClauses[i];
        if (i > 3) {
            BlockedClause* it2 = &blockedClauses[i-3];
            if (!it2->dummy
                && !it->toRemove
            ) {
                __builtin_prefetch(it->lits.data());
            }
        }
        if (it->toRemove) {
            continue;
        }

        it->blockedOn = solver->varReplacer->get_lit_replaced_with_outer(it->blockedOn);
        if (it->dummy) {
            extender->dummyBlocked(it->blockedOn);
        } else {
            //Check if clause can be removed, update literals to replaced literals
            for(Lit& l: it->lits) {
                l = solver->varReplacer->get_lit_replaced_with_outer(l);

                //Check if clause can be removed because it's set at dec level 0
                Lit inter = solver->map_outer_to_inter(l);
                if (solver->value(inter) == l_True
                    && solver->varData[inter.var()].level == 0
                ) {
                    it->toRemove = true;
                    can_remove_blocked_clauses = true;
                    goto next;
                }

                //Blocked clause can be skipped, it's satisfied
                if (solver->model_value(l) == l_True) {
                    goto next;
                }
            }
            extender->addClause(it->lits, it->blockedOn);
        }
        next:;
    }
    if (solver->conf.verbosity) {
        cout << "c [extend] Extended " << blockedClauses.size() << " var-elim clauses" << endl;
    }
}

void OccSimplifier::unlink_clause(
    const ClOffset offset
    , bool doDrat
    , bool allow_empty_watch
    , bool only_set_is_removed
) {
    Clause& cl = *solver->cl_alloc.ptr(offset);
    if (solver->drat->enabled() && doDrat) {
       (*solver->drat) << del << cl << fin;
    }

    if (!cl.red()) {
        for (const Lit lit: cl) {
            touched.touch(lit);
        }
    }

    if (!only_set_is_removed) {
        for (const Lit lit: cl) {
            if (!(allow_empty_watch && solver->watches[lit].empty())) {
                *limit_to_decrease -= 2*(long)solver->watches[lit].size();
                removeWCl(solver->watches[lit], offset);
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
    assert(!solver->drat->something_delayed());
    assert(solver->ok);

    bool satisfied = false;
    Clause& cl = *solver->cl_alloc.ptr(offset);
    (*solver->drat) << deldelay << cl << fin;

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
            removeWCl(solver->watches[*i], offset);
            touched.touch(*i);
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
        (*solver->drat) << findelay;
        unlink_clause(offset, false);
        return l_True;
    }

    if (solver->conf.verbosity >= 6) {
        cout << "-> Clause became after cleaning:" << cl << endl;
    }

    if (i-j > 0) {
        (*solver->drat) << cl << fin << findelay;
    } else {
        solver->drat->forget_delay();
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

        default:
            cl.setStrenghtened();
            cl.recalc_abst_if_needed();
            sub_str_with.push_back(offset);
            return l_Undef;
    }
}


bool OccSimplifier::complete_clean_clause(Clause& cl)
{
    assert(!solver->drat->something_delayed());
    assert(cl.size() > 2);
    (*solver->drat) << deldelay << cl << fin;

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

            (*solver->drat) << findelay;
            return false;
        }

        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    cl.shrink(i-j);
    cl.recalc_abst_if_needed();

    //Drat
    if (i - j > 0) {
        (*solver->drat) << cl << fin << findelay;
    } else {
        solver->drat->forget_delay();
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

        default:
            return true;
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

void OccSimplifier::print_mem_usage_of_occur(uint64_t memUsage) const
{
    if (solver->conf.verbosity) {
        cout
        << "c [simp] mem usage for occur "
        << std::setw(6) << memUsage/(1024ULL*1024ULL) << " MB"
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
        val = float_div(link_in_data.cl_not_linked, link_in_data.cl_linked+link_in_data.cl_not_linked)*100.0;
    }

    cout
    << "c [simp] Not linked in "
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
    , bool alsoOccur
    , uint32_t max_size
    , int64_t link_in_lit_limit
) {
    LinkInData link_in_data;
    for (const ClOffset offs: toAdd) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        cl->recalc_abst_if_needed();
        assert(cl->abst == calcAbstraction(*cl));

        if (alsoOccur
            && cl->size() < max_size
            && link_in_lit_limit > 0
        ) {
            linkInClause(*cl);
            link_in_data.cl_linked++;
            link_in_lit_limit -= cl->size();
            clause_lits_added += cl->size();
        } else {
            /*cout << "alsoOccur: " << alsoOccur
            << " cl->size() < max_size: " << (cl->size() < max_size)
            << " link_in_lit_limit: " << link_in_lit_limit << endl;*/
            //assert(cl->red());
            cl->set_occur_linked(false);
            link_in_data.cl_not_linked++;
            std::sort(cl->begin(), cl->end());
        }

        clauses.push_back(offs);
    }

    return link_in_data;
}

bool OccSimplifier::decide_occur_limit(bool irred, uint64_t memUsage)
{
    //over + irred -> exit
    if (irred
        && memUsage/(1024ULL*1024ULL) >= solver->conf.maxOccurIrredMB
    ) {
        if (solver->conf.verbosity) {
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
        if (solver->conf.verbosity) {
            cout
            << "c [simp] Not linking in red due to excessive expected memory usage"
            << endl;
        }

        return false;
    }

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
    for (ClOffset offs: clauses) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->freed())
            continue;

        assert(!cl->getRemoved());

        //All clauses are larger than 2-long
        assert(cl->size() > 2);

        if (check_varelim_when_adding_back_cl(cl)) {
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
                if (cl->stats.glue <= solver->conf.glue_put_lev0_if_below_or_eq) {
                    cl->stats.which_red_array = 0;
                } else if (cl->stats.glue <= solver->conf.glue_put_lev1_if_below_or_eq) {
                    cl->stats.which_red_array = 1;
                }
                solver->longRedCls[cl->stats.which_red_array].push_back(offs);
            } else {
                solver->longIrredCls.push_back(offs);
            }
        } else {
            solver->cl_alloc.clauseFree(cl);
        }
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

        Watched* i = ws.begin();
        Watched* j = i;
        for (Watched *end2 = ws.end(); i != end2; i++) {
            if (i->isClause()) {
                continue;
            } else {
                assert(i->isBin());
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
    assert(solver->watches.get_smudged_list().empty());

    for(size_t var = solver->mtrand.randInt(solver->nVars()), num = 0
        ; num < solver->nVars() && *limit_to_decrease > 0
        ; var = (var + 1) % solver->nVars(), num++
    ) {
        if (!can_eliminate_var(var))
            continue;

        const Lit lit = Lit(var, false);
        if (!check_empty_resolvent(lit))
            continue;

        create_dummy_blocked_clause(lit);
        rem_cls_from_watch_due_to_varelim(solver->watches[lit], lit);
        rem_cls_from_watch_due_to_varelim(solver->watches[~lit], ~lit);
        set_var_as_eliminated(var, lit);
        var_elimed++;
    }

    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain =  float_div(*limit_to_decrease, orig_empty_varelim_time_limit);
    if (solver->conf.verbosity) {
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

bool OccSimplifier::can_eliminate_var(const uint32_t var) const
{
    assert(var <= solver->nVars());
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
    bvestats.clear();
    bvestats.numCalls = 1;

    //Go through the ordered list of variables to eliminate
    int64_t last_elimed = 1;
    while(last_elimed > 0
        && varelim_num_limit > 0
        && *limit_to_decrease > 0
    ) {
        last_elimed = 0;
        while(!velim_order.empty()
            && *limit_to_decrease > 0
            && varelim_num_limit > 0
            && !solver->must_interrupt_asap()
        ) {
            assert(limit_to_decrease == &norm_varelim_time_limit);
            uint32_t var = velim_order.removeMin();

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
                last_elimed++;
            }
            if (!solver->ok)
                goto end;
        }
        double after_sub_time = cpuTime();
        if (!sub_str->handle_sub_str_with(20ULL*1000ULL*1000ULL)) {
            goto end;
        }

        for(uint32_t l: impl_sub_lits.getTouchedList()) {
            Lit lit = Lit::toLit(l);
            if (!sub_str->backw_sub_str_with_bins_watch(lit, true)) {
                goto end;
            }
            if (*limit_to_decrease <= 0)
                break;
        }

        for(uint32_t l: impl_sub_lits.getTouchedList()) {
            if (*limit_to_decrease <= 0)
                break;

            Lit lit = Lit::toLit(l);
            if (!velim_order.inHeap(lit.var())
                && can_eliminate_var(lit.var())
            ) {
                varElimComplexity[lit.var()] = strategyCalcVarElimScore(lit.var());
                velim_order.insert(lit.var());
            }
        }

        impl_sub_lits.clear();
        if (solver->conf.verbosity) {
            double time_used = cpuTime() - after_sub_time;
            cout << "c [occ-bve] process impls_sub_lits "
            << solver->conf.print_times(time_used)
            << endl;

            cout << "c [occ-bve] iter v-elim " << last_elimed << endl;
        }
    }

end:
    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain = float_div(*limit_to_decrease, orig_norm_varelim_time_limit);

    if (solver->conf.verbosity) {
        cout
        << "c  #try to eliminate: " << wenThrough << endl
        << "c  #var-elim: " << vars_elimed << endl
        << "c  #T-o: " << (time_out ? "Y" : "N") << endl
        << "c  #T-r: " << std::fixed << std::setprecision(2) << (time_remain*100.0) << "%" << endl
        << "c  #T: " << time_used << endl;
    }
    if (solver->conf.verbosity) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.print_short();
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
    bvestats.varElimTimeOut += time_out;
    bvestats.timeUsed = cpuTime() - myTime;
    bvestats_global += bvestats;

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
    if (solver->conf.verbosity) {
        double vm_usage = 0;
        solver->print_watch_mem_used(memUsedTotal(vm_usage));
    }

    return true;
}

bool OccSimplifier::execute_simplifier_strategy(const string& strategy)
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

        #ifdef SLOW_DEBUG
        solver->check_implicit_stats(true);
        #endif
        if (!solver->propagate_occur()) {
            return false;
        }
        set_limits();

        token = trim(token);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (token != "" && solver->conf.verbosity) {
            cout << "c --> Executing OCC strategy token: " << token << '\n';
        }
        if (token == "occ-backw-sub-str") {
            backward_sub_str();
        } else if (token == "occ-xor") {
            #ifdef USE_M4RI
            if (solver->conf.doFindXors
                && topLevelGauss != NULL
            ) {
                XorFinder finder(this, solver);
                finder.find_xors();
                topLevelGauss->toplevelgauss(finder.xors);
            }
            #endif
        } else if (token == "occ-gauss") {
            if (solver->conf.doFindXors) {
                #ifdef USE_GAUSS
                XorFinder finder(this, solver);
                finder.find_xors();
                finder.xor_together_xors();
                const bool ok = finder.add_new_truths_from_xors();
                if (ok) {
                    finder.add_xors_to_gauss();
                }
                #endif
            }
        } else if (token == "occ-clean-implicit") {
            solver->clauseCleaner->clean_implicit_clauses();
        } else if (token == "occ-bve") {
            if (solver->conf.doVarElim && solver->conf.do_empty_varelim) {
                solver->xorclauses.clear();
                solver->clear_gauss();

                eliminate_empty_resolvent_vars();
                eliminate_vars();
            }
        } else if (token == "occ-bva") {
            bva->bounded_var_addition();
        } /*else if (token == "occ-gates") {
            if (solver->conf.doCache
                && solver->conf.doGateFind
            ) {
                gateFinder->doAll();
            }
        }*/ else if (token == "") {
            //nothing, ignore empty token
        } else {
             cout << "ERROR: occur strategy '" << token << "' not recognised!" << endl;
            exit(-1);
        }
    }

    return solver->ok;
}

bool OccSimplifier::setup()
{
    assert(solver->okay());
    assert(toClear.empty());
    sub_str_with.clear();

    //Test & debug
    solver->test_all_clause_attached();
    solver->check_wrong_attach();

    //Clean the clauses before playing with them
    solver->clauseCleaner->remove_and_clean_all();

    //If too many clauses, don't do it
    if (solver->getNumLongClauses() > 10ULL*1000ULL*1000ULL
        || solver->litStats.irredLits > 50ULL*1000ULL*1000ULL
    ) {
        return false;
    }

    //Setup
    clause_lits_added = 0;
    runStats.clear();
    runStats.numCalls++;
    clauses.clear();
    set_limits(); //to calculate strengthening_time_limit
    limit_to_decrease = &strengthening_time_limit;
    if (!fill_occur_and_print_stats()) {
        return false;
    }

    set_limits();
    return solver->ok;
}

bool OccSimplifier::simplify(const bool _startup, const std::string schedule)
{
    startup = _startup;
    if (!setup()) {
        return solver->ok;
    }

    const size_t origBlockedSize = blockedClauses.size();
    const size_t origTrailSize = solver->trail_size();
    execute_simplifier_strategy(schedule);

    remove_by_drat_recently_blocked_clauses(origBlockedSize);
    finishUp(origTrailSize);

    return solver->ok;
}

bool OccSimplifier::backward_sub_str()
{
    assert(cl_to_free_later.empty());
    assert(solver->watches.get_smudged_list().empty());

    if (!sub_str->backward_sub_str_with_bins()) {
        goto end;
    }
    if (solver->must_interrupt_asap())
        goto end;

    sub_str->backward_subsumption_long_with_long();
    if (solver->must_interrupt_asap())
        goto end;

    if (!sub_str->backward_strengthen_long_with_long()) {
        goto end;
    }
    if (solver->must_interrupt_asap())
        goto end;

    if (!sub_str->handle_sub_str_with()) {
        goto end;
    }

    end:
    free_clauses_to_free();
    solver->clean_occur_from_removed_clauses_only_smudged();

    return solver->ok;
}

bool OccSimplifier::fill_occur()
{
    //Add irredundant to occur
    uint64_t memUsage = calc_mem_usage_of_occur(solver->longIrredCls);
    print_mem_usage_of_occur(memUsage);
    if (memUsage > solver->conf.maxOccurIrredMB*1000ULL*1000ULL) {
        CompleteDetachReatacher detRet(solver);
        detRet.reattachLongs(true);
        return false;
    }

    LinkInData link_in_data = link_in_clauses(
        solver->longIrredCls
        , true //add to occur list
        , std::numeric_limits<uint32_t>::max()
        , std::numeric_limits<int64_t>::max()
    );
    solver->longIrredCls.clear();
    print_linkin_data(link_in_data);

    //Add redundant to occur
    memUsage = calc_mem_usage_of_occur(solver->longRedCls[0]);
    print_mem_usage_of_occur(memUsage);
    bool linkin = true;
    if (memUsage > solver->conf.maxOccurRedMB*1000ULL*1000ULL) {
        linkin = false;
    }
    //Sort, so we get the shortest ones in at least
    std::sort(solver->longRedCls[0].begin(), solver->longRedCls[0].end()
        , ClauseSizeSorter(solver->cl_alloc));

    link_in_data = link_in_clauses(
        solver->longRedCls[0]
        , linkin
        , solver->conf.maxRedLinkInSize
        , solver->conf.maxOccurRedLitLinkedM*1000ULL*1000ULL
    );
    solver->longRedCls[0].clear();

    //Don't really link in the rest
    for(auto& lredcls: solver->longRedCls) {
        link_in_clauses(lredcls, linkin, 0, 0);
    }
    for(auto& lredcls: solver->longRedCls) {
        lredcls.clear();
    }
    print_linkin_data(link_in_data);

    return true;
}

//This must NEVER be called during solve. Only JUST BEFORE Solver::solve() is called
//otherwise, uneliminated_vars_since_last_solve will be wrong
bool OccSimplifier::uneliminate(uint32_t var)
{
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "calling uneliminate() on var" << var+1 << endl;
    #endif
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
    bvestats_global.numVarsElimed--;
    solver->varData[var].removed = Removed::none;
    solver->set_decision_var(var);

    //Find if variable is really needed to be eliminated
    var = solver->map_inter_to_outer(var);
    map<uint32_t, vector<size_t> >::iterator it = blk_var_to_cl.find(var);
    if (it == blk_var_to_cl.end())
        return solver->okay();

    //Eliminate it in practice
    //NOTE: Need to eliminate in theory first to avoid infinite loops
    for(size_t i = 0; i < it->second.size(); i++) {
        size_t at = it->second[i];

        //Mark for removal from blocked list
        blockedClauses[at].toRemove = true;
        can_remove_blocked_clauses = true;
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

void OccSimplifier::remove_by_drat_recently_blocked_clauses(size_t origBlockedSize)
{
    if (!(*solver->drat).enabled())
        return;

    if (solver->conf.verbosity >= 6) {
        cout << "c Deleting blocked clauses for DRAT" << endl;
    }

    for(size_t i = origBlockedSize; i < blockedClauses.size(); i++) {
        if (blockedClauses[i].dummy)
            continue;

        //If doing stamping or caching, we cannot delete binary redundant
        //clauses, because they are stored in the stamp/cache and so
        //will be used -- and DRAT will complain when used
        if (blockedClauses[i].lits.size() <= 2
            && (solver->conf.doCache
                || solver->conf.doStamp)
        ) {
            continue;
        }

        (*solver->drat) << del;
        for(vector<Lit>::const_iterator
            it = blockedClauses[i].lits.begin(), end = blockedClauses[i].lits.end()
            ; it != end
            ; ++it
        ) {
            (*solver->drat) << *it;
        }
        (*solver->drat) << fin;
    }
}

void OccSimplifier::buildBlockedMap()
{
    blk_var_to_cl.clear();
    for(size_t i = 0; i < blockedClauses.size(); i++) {
        const BlockedClause& blocked = blockedClauses[i];
        map<uint32_t, vector<size_t> >::iterator it
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
    sub_str->finishedRun();

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
        for (const Watched* it2 = ws.begin(), *end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBin()) {
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
    subsumption_time_limit     = 450LL*1000LL*solver->conf.subsumption_time_limitM
        *solver->conf.global_timeout_multiplier;
    strengthening_time_limit   = 200LL*1000LL*solver->conf.strengthening_time_limitM
        *solver->conf.global_timeout_multiplier;
    norm_varelim_time_limit    = 4ULL*1000LL*1000LL*solver->conf.varelim_time_limitM
        *solver->conf.global_timeout_multiplier;
    empty_varelim_time_limit   = 200LL*1000LL*solver->conf.empty_varelim_time_limitM
        *solver->conf.global_timeout_multiplier;
    aggressive_elim_time_limit = 300LL *1000LL*solver->conf.aggressive_elim_time_limitM
        *solver->conf.global_timeout_multiplier;

    //If variable elimination isn't going so well
    if (bvestats_global.testedToElimVars > 0
        && float_div(bvestats_global.numVarsElimed, bvestats_global.testedToElimVars) < 0.1
    ) {
        norm_varelim_time_limit /= 2;
    }

    #ifdef BIT_MORE_VERBOSITY
    cout << "c clause_lits_added: " << clause_lits_added << endl;
    #endif

    norm_varelim_time_limit *= 4;
    empty_varelim_time_limit *= 4;
    subsumption_time_limit *= 2;
    strengthening_time_limit *= 2;

    varelim_num_limit = ((double)solver->get_num_free_vars() * solver->conf.varElimRatioPerIter);

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

void OccSimplifier::cleanBlockedClausesIfDirty()
{
    if (can_remove_blocked_clauses) {
        cleanBlockedClauses();
    }
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
        const uint32_t blockedOn = solver->map_outer_to_inter(i->blockedOn.var());
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
    can_remove_blocked_clauses = false;
}

size_t OccSimplifier::rem_cls_from_watch_due_to_varelim(
    watch_subarray todo
    , const Lit lit
) {
    blockedMapBuilt = false;
    const size_t orig_blocked_cls_size = blockedClauses.size();

    //Copy&clear i.e. MOVE
    todo.moveTo(tmp_rem_cls_copy);
    assert(solver->watches[lit].empty());

    vector<Lit>& lits = tmp_rem_lits;
    for (const Watched watch :tmp_rem_cls_copy) {
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
                bvestats.clauses_elimed_long++;
                bvestats.clauses_elimed_sumsize += cl.size();

                lits.resize(cl.size());
                std::copy(cl.begin(), cl.end(), lits.begin());
                add_clause_to_blck(lit, lits);
                for(Lit lit: lits) {
                    touched.touch(lit);
                }
            } else {
                red = true;
                bvestats.longRedClRemThroughElim++;
            }

            //Remove -- only DRAT the ones that are redundant
            //The irred will be removed thanks to 'blocked' system
            unlink_clause(offset, cl.red(), true, true);
        }

        if (watch.isBin()) {

            //Update stats
            if (!watch.red()) {
                bvestats.clauses_elimed_bin++;
                bvestats.clauses_elimed_sumsize += 2;
            } else {
                red = true;
                bvestats.binRedClRemThroughElim++;
            }

            //Put clause into blocked status
            lits.resize(2);
            lits[0] = lit;
            lits[1] = watch.lit2();
            if (!watch.red()) {
                add_clause_to_blck(lit, lits);
                touched.touch(lits[1]);
            } else {
                //If redundant, delayed blocked-based DRAT deletion will not work
                //so delete explicitly

                //Drat
                if (!solver->conf.doStamp && !solver->conf.doCache) {
                   (*solver->drat) << del << lits[0] << lits[1] << fin;
                }
            }

            //Remove
            //*limit_to_decrease -= (long)solver->watches[lits[0]].size()/4; //This is zero
            *limit_to_decrease -= (long)solver->watches[lits[1]].size()/4;
            solver->detach_bin_clause(lits[0], lits[1], red, true, true);
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

void OccSimplifier::find_gate(
    Lit elim_lit
    , watch_subarray_const a
    , watch_subarray_const b
) {
    assert(toClear.empty());
    for(const Watched w: a) {
        if (w.isBin() && !w.red()) {
            seen[(~w.lit2()).toInt()] = 1;
            toClear.push_back(~w.lit2());
        }
    }

    //Have to find the corresponding gate. Finding one is good enough
    for(const Watched w: b) {
        if (w.isBin()) {
            continue;
        }

        if (w.isClause()) {
            Clause* cl = solver->cl_alloc.ptr(w.get_offset());
            if (cl->getRemoved()) {
                continue;
            }

            assert(cl->size() > 2);
            if (!cl->red()) {
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
                    cl->stats.marked_clause = true;
                    gate_varelim_clause = cl;
                    break;
                }
            }
        }
    }

    for(Lit l: toClear) {
        seen[l.toInt()] = 0;
    }
    toClear.clear();
}

void OccSimplifier::mark_gate_in_poss_negs(
    Lit elim_lit
    , watch_subarray_const poss
    , watch_subarray_const negs
) {
    //Either of the two is OK. Let's just find ONE, not the biggest one.
    //We could find the biggest one, but it's expensive.
    bool found_pos = false;
    gate_varelim_clause = NULL;
    find_gate(elim_lit, poss, negs);
    if (gate_varelim_clause == NULL) {
        find_gate(~elim_lit, negs, poss);
        found_pos = true;
    }

    if (gate_varelim_clause != NULL && solver->conf.verbosity >= 10) {
        cout
        << "Lit: " << elim_lit
        << " gate_found_elim_pos:" << found_pos
        << endl;
    }
}

int OccSimplifier::test_elim_and_fill_resolvents(const uint32_t var)
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
    bvestats.usedAggressiveCheckToELim += aggressive;

    //set-up
    const Lit lit = Lit(var, false);
    watch_subarray poss = solver->watches[lit];
    watch_subarray negs = solver->watches[~lit];
    std::sort(poss.begin(), poss.end(), watch_sort_smallest_first());
    std::sort(negs.begin(), negs.end(), watch_sort_smallest_first());
    resolvents.clear();

    //Pure literal, no resolvents
    //we look at "pos" and "neg" (and not poss&negs) because we don't care about redundant clauses
    if (pos.totalCls() == 0 || neg.totalCls() == 0) {
        return -100;
    }

    //Too expensive to check, it's futile
    if ((uint64_t)neg.totalCls() * (uint64_t)pos.totalCls()
        >= solver->conf.varelim_cutoff_too_many_clauses
    ) {
        return std::numeric_limits<int>::max();
    }

    gate_varelim_clause = NULL;
    if (solver->conf.skip_some_bve_resolvents) {
        mark_gate_in_poss_negs(lit, poss, negs);
    }

    // Count clauses/literals after elimination
    uint32_t before_clauses = pos.bin + pos.longer + neg.bin + neg.longer;
    uint32_t after_clauses = 0;
    uint32_t after_long = 0;
    uint32_t after_bin = 0;
    uint32_t after_literals = 0;

    size_t at_poss = 0;
    for (const Watched* it = poss.begin(), *end = poss.end()
        ; it != end
        ; ++it, at_poss++
    ) {
        *limit_to_decrease -= 3;
        if (solver->redundant_or_removed(*it))
            continue;

        size_t at_negs = 0;
        for (const Watched *it2 = negs.begin(), *end2 = negs.end()
            ; it2 != end2
            ; it2++, at_negs++
        ) {
            *limit_to_decrease -= 3;
            if (solver->redundant_or_removed(*it2))
                continue;

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
            if (dummy.size() >= 3)
                after_long++;
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
                if (gate_varelim_clause) {
                    gate_varelim_clause->stats.marked_clause = false;
                }
                return std::numeric_limits<int>::max();
            }

            //Calculate new clause stats
            ClauseStats stats;
            if (it->isBin() && it2->isClause())
                stats = solver->cl_alloc.ptr(it2->get_offset())->stats;
            else if (it2->isBin() && it->isClause())
                stats = solver->cl_alloc.ptr(it->get_offset())->stats;
            else if (it->isClause() && it2->isClause())
                stats = ClauseStats::combineStats(
                    solver->cl_alloc.ptr(it->get_offset())->stats
                    , solver->cl_alloc.ptr(it2->get_offset())->stats
            );

            resolvents.push_back(Resolvent(dummy, stats));
        }
    }

    if (gate_varelim_clause) {
        gate_varelim_clause->stats.marked_clause = false;
    }

    //Smaller value returned, the better
    int cost = (int)after_long + (int)after_bin*(int)3
        - (int)pos.longer - (int)neg.longer
        - (int)pos.bin*3 - (int)neg.bin*(int)3;

    return cost;
}

void OccSimplifier::printOccur(const Lit lit) const
{
    for(size_t i = 0; i < solver->watches[lit].size(); i++) {
        const Watched& w = solver->watches[lit][i];
        if (w.isBin()) {
            cout
            << "Bin   --> "
            << lit << ", "
            << w.lit2()
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
    << solver->watches[lit].size() << " , "
    << solver->watches[~lit].size()
    << endl;

    cout << "POS: " << endl;
    printOccur(lit);
    cout << "NEG: " << endl;
    printOccur(~lit);
}

bool OccSimplifier::add_varelim_resolvent(
    vector<Lit>& finalLits
    , const ClauseStats& stats
) {
    bvestats.newClauses++;
    Clause* newCl = NULL;

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
        SubsumeStrengthen::Sub1Ret ret = sub_str->strengthen_subsume_and_unlink_and_markirred(offset);
        bvestats.subsumedByVE += ret.sub;

    } else if (finalLits.size() == 2) {
        std::sort(finalLits.begin(), finalLits.end());
        SubsumeStrengthen::Sub1Ret ret = sub_str->sub_str_with_implicit(finalLits);
        bvestats.subsumedByVE += ret.sub;
        if (!solver->ok) {
            return false;
        }
    }
    for(const Lit lit: finalLits) {
        impl_sub_lits.touch(lit);
    }

    //Touch every var of the new clause, so we re-estimate
    //elimination complexity for this var
    for(Lit lit: finalLits)
        touched.touch(lit);

    return true;
}

void OccSimplifier::update_varelim_complexity_heap(const uint32_t elimed_var)
{
    //Update var elim complexity heap
    if (!solver->conf.updateVarElimComplexityOTF)
        return;

    if (num_otf_update_until_now > solver->conf.updateVarElimComplexityOTF_limitvars
        || time_spent_on_calc_otf_update > solver->conf.updateVarElimComplexityOTF_limitavg*100ULL*1000ULL
    ) {
        const double avg = float_div(time_spent_on_calc_otf_update, num_otf_update_until_now);

        if (avg > solver->conf.updateVarElimComplexityOTF_limitavg) {
            solver->conf.updateVarElimComplexityOTF = false;
            if (solver->conf.verbosity) {
                cout
                << "c [occ-bve] Turning off OTF complexity update, it's too expensive"
                << endl;
            }
            return;
        }
    }

    int64_t limit_before = *limit_to_decrease;
    num_otf_update_until_now++;
    for(uint32_t var: touched.getTouchedList()) {
        //No point in updating the score of this var
        //it's eliminated already, or not to be eliminated at all
        if (var == elimed_var || !can_eliminate_var(var)) {
            continue;
        }

        varElimComplexity[var] = strategyCalcVarElimScore(var);
        velim_order.update(var);
    }
    time_spent_on_calc_otf_update += limit_before - *limit_to_decrease;
}

void OccSimplifier::print_var_elim_complexity_stats(const uint32_t var) const
{
    if (solver->conf.verbosity < 5)
        return;

    cout << "trying complexity: "
    << varElimComplexity[var].first
    << ", " << varElimComplexity[var].second
    << endl;
}

void OccSimplifier::set_var_as_eliminated(const uint32_t var, const Lit lit)
{
    if (solver->conf.verbosity >= 5) {
        cout << "Elimination of var "
        <<  solver->map_inter_to_outer(lit)
        << " finished " << endl;
    }
    assert(solver->varData[var].removed == Removed::none);
    solver->varData[var].removed = Removed::elimed;

    bvestats_global.numVarsElimed++;
}

void OccSimplifier::create_dummy_blocked_clause(const Lit lit)
{
    blockedClauses.push_back(
        BlockedClause(solver->map_inter_to_outer(lit))
    );
}

bool OccSimplifier::maybe_eliminate(const uint32_t var)
{
    assert(solver->ok);
    print_var_elim_complexity_stats(var);
    bvestats.testedToElimVars++;

    //Heuristic says no, or we ran out of time
    if (test_elim_and_fill_resolvents(var) == std::numeric_limits<int>::max()
        || *limit_to_decrease < 0
    ) {
        return false;  //didn't eliminate :(
    }
    bvestats.triedToElimVars++;

    const Lit lit = Lit(var, false);
    print_var_eliminate_stat(lit);

    //Remove clauses
    touched.clear();
    create_dummy_blocked_clause(lit);
    rem_cls_from_watch_due_to_varelim(solver->watches[lit], lit);
    rem_cls_from_watch_due_to_varelim(solver->watches[~lit], ~lit);

    //It's best to add resolvents with largest first. Then later, the smaller ones
    //can subsume the larger ones. While adding, we do subsumption check.
    std::sort(resolvents.begin(), resolvents.end());

    //Add resolvents
    for(Resolvent& resolvent: resolvents) {
        if (!add_varelim_resolvent(resolvent.lits, resolvent.stats)) {
            goto end;
        }
    }
    limit_to_decrease = &norm_varelim_time_limit;

    if (*limit_to_decrease > 0) {
        update_varelim_complexity_heap(var);
    }

end:
    set_var_as_eliminated(var, lit);

    return true; //elininated!
}

void OccSimplifier::add_pos_lits_to_dummy_and_seen(
    const Watched ps
    , const Lit posLit
) {
    if (ps.isBin()) {
        *limit_to_decrease -= 1;
        assert(ps.lit2() != posLit);

        seen[ps.lit2().toInt()] = 1;
        dummy.push_back(ps.lit2());
    }

    if (ps.isClause()) {
        Clause& cl = *solver->cl_alloc.ptr(ps.get_offset());
        *limit_to_decrease -= (long)cl.size()/2;
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
    if (qs.isBin()) {
        *limit_to_decrease -= 1;
        assert(qs.lit2() != ~posLit);

        if (seen[(~qs.lit2()).toInt()]) {
            return true;
        }
        if (!seen[qs.lit2().toInt()]) {
            dummy.push_back(qs.lit2());
            seen[qs.lit2().toInt()] = 1;
        }
    }

    if (qs.isClause()) {
        Clause& cl = *solver->cl_alloc.ptr(qs.get_offset());
        *limit_to_decrease -= (long)cl.size()/2;
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
    if (ps.isBin()
        || qs.isBin()
        || !solver->conf.doCache
        || (!solver->conf.otfHyperbin && solver->drat->enabled())
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
        const vector<LitExtra>& cache = solver->implCache[lit].lits;
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

    if (!ps.isBin() && !qs.isBin()) {
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
    Clause *cl1 = NULL;
    if (ps.isClause()) {
         cl1 = solver->cl_alloc.ptr(ps.get_offset());
        if (cl1->freed()) {
            return true;
        }
    }

    Clause *cl2 = NULL;
    if (qs.isClause()) {
         cl2 = solver->cl_alloc.ptr(qs.get_offset());
        if (cl2->freed()) {
            return true;
        }
    }
    if (gate_varelim_clause
        && cl1 && cl2
        && !cl1->stats.marked_clause
        && !cl2->stats.marked_clause
    ) {
        //for G (U) R, we only neede to resolve to
        // (Gx * R!x) (U) (G!x * Rx)
        // So Rx * R!x is skipped
        //
        // Here: Both are long clauses, so only one could be in G. But neither
        // are marked, hence neither are in G, so both are in R.
        // see:  http://baldur.iti.kit.edu/sat/files/ex04.pdf
        return true;
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
    watch_subarray_const ws = solver->watches[lit];
    aggressive_elim_time_limit -= (int64_t)ws.size()/3 + 2;
    for(const Watched* it = ws.begin(), *end = ws.end()
        ; it != end
        ; ++it
    ) {
        //Can't do much with clauses, too expensive
        if (it->isClause())
            continue;

        //Handle binary
        if (it->isBin() && !it->red()) {
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

    watch_subarray_const ws_list = solver->watches[lit];
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
    if (solver->watches[~lit].size() < solver->watches[lit].size())
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

    watch_subarray_const watch_list = solver->watches[lit];
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
        if (ws.isBin()){
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
                        int num = my_popcnt(seen[(~ws.lit2()).toInt()]);
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
                    int num = my_popcnt(tmp);
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



pair<int, int> OccSimplifier::heuristicCalcVarElimScore(const uint32_t var)
{
    const Lit lit(var, false);
    const HeuristicData pos = calc_data_for_heuristic(lit);
    const HeuristicData neg = calc_data_for_heuristic(~lit);

    //Estimate cost
    int posTotalLonger = pos.longer;
    int negTotalLonger = neg.longer;
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

        case 2:
            normCost =  pos.totalCls() * neg.totalCls();
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

    if (pos.totalCls() == 0
        || neg.totalCls() == 0
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
        assert(!velim_order.inHeap(var));
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

std::pair<int, int> OccSimplifier::strategyCalcVarElimScore(const uint32_t var)
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
    if (bvestats_global.numVarsElimed != checkNumElimed) {
        std::cerr
        << "ERROR: globalStats.numVarsElimed is "<<
        bvestats_global.numVarsElimed
        << " but checkNumElimed is: " << checkNumElimed
        << endl;

        assert(false);
    }
}

size_t OccSimplifier::mem_used() const
{
    size_t b = 0;
    b += dummy.capacity()*sizeof(char);
    b += sub_str_with.capacity()*sizeof(ClOffset);
    b += sub_str->mem_used();
    for(map<uint32_t, vector<size_t> >::const_iterator
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
    b += blk_var_to_cl.size()*(sizeof(uint32_t)+sizeof(vector<size_t>)); //TODO under-counting
    b += velim_order.mem_used();
    b += varElimComplexity.capacity()*sizeof(int)*2;
    b += touched.mem_used();
    b += clauses.capacity()*sizeof(ClOffset);

    return b;
}

size_t OccSimplifier::mem_used_xor() const
{
    if (topLevelGauss)
        return topLevelGauss->mem_used();
    else
        return 0;
}

size_t OccSimplifier::mem_used_bva() const
{
    if (bva)
        return bva->mem_used();
    else
        return 0;
}


void OccSimplifier::freeXorMem()
{
    delete topLevelGauss;
    topLevelGauss = NULL;
}

void OccSimplifier::linkInClause(Clause& cl)
{
    assert(cl.size() > 2);
    ClOffset offset = solver->cl_alloc.get_offset(&cl);
    cl.recalc_abst_if_needed();

    std::sort(cl.begin(), cl.end());
    for (const Lit lit: cl) {
        watch_subarray ws = solver->watches[lit];
        *limit_to_decrease -= (long)ws.size();

        ws.push(Watched(offset, cl.abst));
    }
    cl.set_occur_linked(true);
}


/*void OccSimplifier::print_gatefinder_stats() const
{
    if (gateFinder) {
        gateFinder->get_stats().print(solver->nVarsOuter());
    }
}*/

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
    zeroDepthAssings += other.zeroDepthAssings;

    return *this;
}

BVEStats& BVEStats::operator+=(const BVEStats& other)
{
    numVarsElimed += other.numVarsElimed;
    varElimTimeOut += other.varElimTimeOut;
    clauses_elimed_long += other.clauses_elimed_long;
    clauses_elimed_tri += other.clauses_elimed_tri;
    clauses_elimed_bin += other.clauses_elimed_bin;
    clauses_elimed_sumsize += other.clauses_elimed_sumsize;
    longRedClRemThroughElim += other.longRedClRemThroughElim;
    binRedClRemThroughElim += other.binRedClRemThroughElim;
    numRedBinVarRemAdded += other.numRedBinVarRemAdded;
    testedToElimVars += other.testedToElimVars;
    triedToElimVars += other.triedToElimVars;
    usedAggressiveCheckToELim += other.usedAggressiveCheckToELim;
    newClauses += other.newClauses;
    subsumedByVE  += other.subsumedByVE;

    return *this;
}

void OccSimplifier::Stats::print_short() const
{

    cout
    << "c [occur] " << linkInTime+finalCleanupTime << " is overhead"
    << endl;

    cout
    << "c [occur] link-in T: " << linkInTime
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

    print_stats_line("c called"
        ,  numCalls
        , float_div(total_time(), numCalls)
        , "s per call"
    );




    print_stats_line("c 0-depth assigns"
        , zeroDepthAssings
        , stats_line_percent(zeroDepthAssings, nVars)
        , "% vars"
    );

    cout << "c -------- OccSimplifier STATS END ----------" << endl;
}

void OccSimplifier::save_state(SimpleOutFile& f)
{
    assert(solver->decisionLevel() == 0);
    cleanBlockedClauses();
    f.put_uint64_t(blockedClauses.size());
    for(const BlockedClause& c: blockedClauses) {
        c.save_to_file(f);
    }
    f.put_struct(globalStats);
    f.put_uint32_t(anythingHasBeenBlocked);


}
void OccSimplifier::load_state(SimpleInFile& f)
{
    const uint64_t sz = f.get_uint64_t();
    for(uint64_t i = 0; i < sz; i++) {
        BlockedClause b;
        b.load_from_file(f);
        blockedClauses.push_back(b);
    }
    f.get_struct(globalStats);
    anythingHasBeenBlocked = f.get_uint32_t();

    blockedMapBuilt = false;
    buildBlockedMap();

    //Sanity check
    for(size_t i = 0; i < solver->nVars(); i++) {
        if (solver->varData[i].removed == Removed::elimed) {
            assert(solver->value(i) == l_Undef);
        }
    }
}
