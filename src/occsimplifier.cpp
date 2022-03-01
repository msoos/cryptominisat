/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
#include "gatefinder.h"
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
using std::sort;
using std::unique;
using std::array;

//#define VERBOSE_DEBUG_VARELIM
//#define VERBOSE_DEBUG_XOR_FINDER
//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS
//#define VERBOSE_ORGATE_REPLACE
//#define VERBOSE_DEBUG_ASYMTE
//#define VERBOSE_GATE_REMOVAL
//#define VERBOSE_XORGATE_MIX
//#define CHECK_N_OCCUR
//#define DEBUG_VARELIM

OccSimplifier::OccSimplifier(Solver* _solver):
    solver(_solver)
    , seen(solver->seen)
    , seen2(solver->seen2)
    , toClear(solver->toClear)
    , velim_order(VarOrderLt(varElimComplexity))
    , topLevelGauss(NULL)
    , gateFinder(NULL)
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

    tmp_bin_cl.resize(2);
}

OccSimplifier::~OccSimplifier()
{
    delete bva;
    delete topLevelGauss;
    delete sub_str;
    delete gateFinder;
}

void OccSimplifier::new_var(const uint32_t /*orig_outer*/)
{
    n_occurs.insert(n_occurs.end(), 2, 0);
    if (solver->conf.sampling_vars) {
        sampling_vars_occsimp.insert(sampling_vars_occsimp.end(), 1, 0);
    }
}

void OccSimplifier::new_vars(size_t n)
{
    n_occurs.insert(n_occurs.end(), n*2ULL, 0);
    if (solver->conf.sampling_vars) {
        sampling_vars_occsimp.insert(sampling_vars_occsimp.end(), n, 0);
    }
}

void OccSimplifier::save_on_var_memory()
{
    clauses.clear();
    clauses.shrink_to_fit();
    blkcls.shrink_to_fit();

    cl_to_free_later.shrink_to_fit();

    elim_calc_need_update.shrink_to_fit();
    blockedClauses.shrink_to_fit();
}

void OccSimplifier::print_blocked_clauses_reverse() const
{
    for(vector<BlockedClauses>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; ++it
    ) {
        size_t at = 1;
        vector<Lit> lits;
        while(at < it->size()) {
            Lit l = it->at(at, blkcls);
            if (l == lit_Undef) {
                cout
                << "blocked clause (internal number):";
                for(size_t i = 0; i < it->size(); i++) {
                    cout << it->at(i, blkcls) << " ";
                }
                cout << endl;
                lits.clear();
            } else {
                lits.push_back(l);
            }
            at++;
        }

        cout
        << "dummy blocked clause for var (internal number) " << it->at(0, blkcls).var()
        << endl;

    }
}

uint32_t OccSimplifier::dump_blocked_clauses(std::ostream* outfile) const
{
    uint32_t num_cls = 0;
    for (BlockedClauses blocked: blockedClauses) {
        if (blocked.toRemove)
            continue;

        for (size_t i = 0; i < blocked.size(); i++) {
            //It's blocked on this variable
            if (i == 0) {
                continue;
            }
            Lit l = blocked.at(i, blkcls);
            if (outfile != NULL) {
                if (l == lit_Undef) {
                    *outfile
                    << " 0"
                    << endl;
                } else {
                    *outfile
                    << l << " ";
                }
            }
            if (l == lit_Undef) {
                num_cls++;
            }
        }
    }
    return num_cls;
}

bool OccSimplifier::get_blocked_clause_at(uint32_t& at,uint32_t& at2, vector<Lit>& out) const
{
    out.clear();
    while(at < blockedClauses.size()) {
        const auto& blocked = blockedClauses[at];
        if (blocked.toRemove) {
            at++;
            continue;
        }

        while(at2 <  blocked.size()) {
            //It's blocked on this variable
            if (at2 == 0) {
                at2++;
                continue;
            }
            Lit l = blocked.at(at2, blkcls);
            if (l == lit_Undef) {
                at2++;
                return true;
                //nothing, return?
            } else {
                out.push_back(l);
            }
            at2++;
        }
        at2 = 0;
        at++;
    }
    return false;
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
    vector<Lit> lits;
    for (long int i = (int)blockedClauses.size()-1; i >= 0; i--) {
        BlockedClauses* it = &blockedClauses[i];
        if (it->toRemove) {
            continue;
        }

        Lit blockedOn = solver->varReplacer->get_lit_replaced_with_outer(it->at(0, blkcls));
        size_t at = 1;
        bool satisfied = false;
        lits.clear();
        while(at < it->size()) {
            //built clause, reached marker, "lits" is now valid
            if (it->at(at, blkcls) == lit_Undef) {
                if (!satisfied) {
                    bool var_set = extender->addClause(lits, blockedOn.var());

                    #ifndef DEBUG_VARELIM
                    //all should be satisfied in fact
                    //no need to go any further
                    if (var_set) {
                        break;
                    }
                    #endif
                }
                satisfied = false;
                lits.clear();

            //Building clause, "lits" is not yet valid
            } else if (!satisfied) {
                Lit l = it->at(at, blkcls);
                l = solver->varReplacer->get_lit_replaced_with_outer(l);
                lits.push_back(l);

                //Blocked clause can be skipped, it's satisfied
                if (solver->model_value(l) == l_True) {
                    satisfied = true;
                }
            }
            at++;
        }
        extender->dummyBlocked(blockedOn.var());
    }
    if (solver->conf.verbosity >= 2) {
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
    if (doDrat && (solver->drat->enabled() || solver->conf.simulate_drat)) {
       (*solver->drat) << del << cl << fin;
    }

    if (!cl.red()) {
        for (const Lit lit: cl) {
            elim_calc_need_update.touch(lit.var());
#ifdef CHECK_N_OCCUR
            assert(n_occurs[lit.toInt()]>0);
#endif
            n_occurs[lit.toInt()]--;
            removed_cl_with_var.touch(lit.var());
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
    }
    cl.setRemoved();

    if (cl.red()) {
        solver->litStats.redLits -= cl.size();
    } else {
        solver->litStats.irredLits -= cl.size();
    }

    if (!only_set_is_removed) {
        solver->free_cl(&cl);
    } else {
        cl_to_free_later.push_back(offset);
    }
}

bool OccSimplifier::clean_clause(
    ClOffset offset,
    bool only_set_is_removed)
{
    assert(!solver->drat->something_delayed());
    assert(solver->okay());

    bool satisfied = false;
    Clause& cl = *solver->cl_alloc.ptr(offset);
    (*solver->drat) << deldelay << cl << fin;

    Lit* i = cl.begin();
    Lit* j = cl.begin();
    const Lit* end = cl.end();
    *limit_to_decrease -= (long)cl.size();
    for(; i != end; i++) {
        if (solver->value(*i) == l_Undef) {
            //clean_clause() is called when the clause changed, so this is relevant
            added_cl_to_var.touch(i->var());
            *j++ = *i;
            continue;
        }

        if (solver->value(*i) == l_True)
            satisfied = true;

        if (solver->value(*i) == l_True
            || solver->value(*i) == l_False
        ) {
            removeWCl(solver->watches[*i], offset);
            if (!cl.red()) {
                removed_cl_with_var.touch(i->var());
                elim_calc_need_update.touch(i->var());
                n_occurs[i->toInt()]--;
            }
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
        unlink_clause(offset, false, false, only_set_is_removed);
        return true;
    }

    if (solver->conf.verbosity >= 6) {
        cout << "-> Clause became after cleaning:" << cl << endl;
    }

    if (i-j > 0) {
        cl.stats.ID = solver->clauseID++;
        (*solver->drat) << add << cl << fin << findelay;
    } else {
        solver->drat->forget_delay();
    }

    switch(cl.size()) {
        case 0:
            unlink_clause(offset, false, false, only_set_is_removed);
            solver->ok = false;
            return false;

        case 1: {
            assert(solver->unit_cl_IDs[cl[0].var()] == 0);
            solver->enqueue<false>(cl[0]);
            solver->unit_cl_IDs[cl[0].var()] = cl.stats.ID;
            unlink_clause(offset, false, false, only_set_is_removed);
            solver->ok = solver->propagate_occur<false>();
            return solver->okay();
        }

        case 2: {
            solver->attach_bin_clause(cl[0], cl[1], cl.red(), cl.stats.ID);
            if (!cl.red()) {
                std::pair<Lit, Lit> tmp = {cl[0], cl[1]};
                added_irred_bin.push_back(tmp);
                n_occurs[tmp.first.toInt()]++;
                n_occurs[tmp.second.toInt()]++;
            }
            unlink_clause(offset, false,  false, only_set_is_removed);
            return true;
        }
        default:
            cl.setStrenghtened();
            cl.recalc_abst_if_needed();
            if (!cl.red()) {
                added_long_cl.push_back(offset);
            }
            return true;
    }
}


bool OccSimplifier::complete_clean_clause(Clause& cl)
{
    assert(solver->okay());
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
        cl.stats.ID = solver->clauseID++;
        (*solver->drat) << add << cl << fin << findelay;
    } else {
        solver->drat->forget_delay();
    }

    switch (cl.size()) {
        case 0:
            solver->ok = false;
            return false;

        case 1: {
            assert(solver->unit_cl_IDs[cl[0].var()] == 0);
            solver->enqueue<false>(cl[0]);
            solver->unit_cl_IDs[cl[0].var()] = cl.stats.ID;
            return false;
        }
        case 2:
            solver->attach_bin_clause(cl[0], cl[1], cl.red(), cl.stats.ID);
            return false;

        default:
            return true;
    }
}

struct sort_smallest_first {
    sort_smallest_first(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}

    bool operator()(const Watched& first, const Watched& second)
    {
        if (second.isBin() && first.isClause()) {
            //wrong order
            return false;
        }
        if (first.isBin() && second.isClause()) {
            //this is the right order
            return true;
        }

        if (first.isBin() && second.isBin()) {
            //correct order if first has lit2() smaller.
            if (first.lit2() != second.lit2()) {
                return first.lit2() < second.lit2();
            }
            return first.get_ID() < second.get_ID();
        }

        if (first.isClause() && second.isClause()) {
            Clause& cl1 = *cl_alloc.ptr(first.get_offset());
            Clause& cl2 = *cl_alloc.ptr(second.get_offset());
            if (cl1.size() != cl2.size()) {
                //Smaller clause size first is correct order
                return cl1.size() < cl2.size();
            }

            //we don't care, let's use offset as a distinguisher
            return first.get_offset() < second.get_offset();
        }

        assert(false && "This cannot happen");
        return false;
    }

    ClauseAllocator& cl_alloc;
};

uint64_t OccSimplifier::calc_mem_usage_of_occur(const vector<ClOffset>& toAdd) const
{
    uint64_t memUsage = 0;
    for (const ClOffset offs: toAdd) {
        Clause* cl = solver->cl_alloc.ptr(offs);

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
        << "c [occ] mem usage for occur "
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
    << "c [occ] Not linked in "
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
        assert(!cl->red() || cl->stats.glue > 0);

        if (alsoOccur
            && cl->size() < max_size
            && link_in_lit_limit > 0
        ) {
            link_in_clause(*cl);
            link_in_data.cl_linked++;
            link_in_lit_limit -= cl->size();
            clause_lits_added += cl->size();
        } else {
            /*cout << "alsoOccur: " << alsoOccur
            << " cl->size() < max_size: " << (cl->size() < max_size)
            << " link_in_lit_limit: " << link_in_lit_limit << endl;*/
            //assert(cl->red());
            cl->setOccurLinked(false);
            link_in_data.cl_not_linked++;
            std::sort(cl->begin(), cl->end());
        }

        clauses.push_back(offs);
    }

    return link_in_data;
}

bool OccSimplifier::check_varelim_when_adding_back_cl(const Clause* cl) const
{
    bool notLinkedNeedFree = false;
    for (Clause::const_iterator
        it2 = cl->begin(), end2 = cl->end()
        ; it2 != end2
        ; ++it2
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
    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();

    for (ClOffset offs: clauses) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->getRemoved() || cl->freed()) {
            continue;
        }
        assert(!cl->stats.marked_clause);
        assert(cl->size() > 2);

        if (check_varelim_when_adding_back_cl(cl)) {
            //The clause wasn't linked in but needs removal now
            if (cl->red()) {
                solver->litStats.redLits -= cl->size();
            } else {
                solver->litStats.irredLits -= cl->size();
            }
            solver->free_cl(cl);
            continue;
        }

        if (solver->okay() && complete_clean_clause(*cl)) {
            solver->attachClause(*cl);
            if (cl->red()) {
                assert(cl->stats.glue > 0);
                assert(cl->stats.which_red_array < solver->longRedCls.size());
                solver->longRedCls[cl->stats.which_red_array].push_back(offs);
            } else {
                solver->longIrredCls.push_back(offs);
            }
        } else {
            solver->free_cl(cl);
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
    assert(added_long_cl.empty());
    assert(solver->okay());
    assert(solver->prop_at_head());
    assert(added_irred_bin.empty());

    uint32_t var_elimed = 0;
    double myTime = cpuTime();
    const int64_t orig_empty_varelim_time_limit = empty_varelim_time_limit;
    limit_to_decrease = &empty_varelim_time_limit;
    assert(cl_to_free_later.empty());
    assert(solver->watches.get_smudged_list().empty());

    ///Nothing to do
    if (solver->nVars() == 0)
        return;

    for(size_t var = solver->mtrand.randInt(solver->nVars()-1), num = 0
        ; num < solver->nVars() && *limit_to_decrease > 0
        ; var = (var + 1) % solver->nVars(), num++
    ) {
        assert(var == var % solver->nVars());
        if (!can_eliminate_var(var))
            continue;

        const Lit lit = Lit(var, false);
        if (!check_empty_resolvent(lit))
            continue;

        create_dummy_blocked_clause(lit);
        rem_cls_from_watch_due_to_varelim(lit);
        rem_cls_from_watch_due_to_varelim(~lit);
        set_var_as_eliminated(var);
        var_elimed++;
    }

    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain =  float_div(*limit_to_decrease, orig_empty_varelim_time_limit);
    if (solver->conf.verbosity) {
        cout
        << "c [occ-empty-res] Empty resolvent elimed: " << var_elimed
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
    #ifdef SLOW_DEBUG
    if (solver->conf.sampling_vars) {
        assert(var < solver->nVars());
        assert(var < sampling_vars_occsimp.size());
    }
    #endif

    assert(var < solver->nVars());
    if (solver->value(var) != l_Undef ||
        solver->varData[var].removed != Removed::none ||
        solver->var_inside_assumptions(var) != l_Undef ||
        ((solver->conf.sampling_vars || solver->fast_backw.fast_backw_on) &&
            sampling_vars_occsimp[var])
    ) {
        return false;
    }

    return true;
}

uint32_t OccSimplifier::sum_irred_cls_longs() const
{
    uint32_t sum = 0;
    for (ClOffset offs: clauses) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->freed() || cl->getRemoved() || cl->red())
            continue;

        assert(cl->size() > 2);
        sum++;
    }
    return sum;
}

uint32_t OccSimplifier::sum_irred_cls_longs_lits() const
{
    uint32_t sum = 0;
    for (ClOffset offs: clauses) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->freed() || cl->getRemoved() || cl->red())
            continue;

        assert(cl->size() > 2);
        sum += cl->size();
    }
    return sum;
}

//backward subsumes with added long and bin
bool OccSimplifier::deal_with_added_long_and_bin(const bool main)
{
    assert(solver->okay());
    assert(solver->prop_at_head());

    while (!added_long_cl.empty() || !added_irred_bin.empty())
    {
        if (!sub_str->handle_added_long_cl(main)) {
            return false;
        }
        assert(solver->okay());
        assert(solver->prop_at_head());

        //NOTE: added_irred_bin CAN CHANGE while this is running!!
        for (size_t i = 0; i < added_irred_bin.size(); i++) {
            tmp_bin_cl[0] = added_irred_bin[i].first;
            tmp_bin_cl[1] = added_irred_bin[i].second;

            Sub1Ret ret; //TODO use this in the stats
            if (!sub_str->backw_sub_str_with_impl(tmp_bin_cl, ret)) {
                return false;
            }
        }
        added_irred_bin.clear();
    }
    assert(added_long_cl.empty());
    assert(added_irred_bin.empty());

    return solver->okay();
}

bool OccSimplifier::clear_vars_from_cls_that_have_been_set()
{
    assert(solver->okay());
    assert(solver->decisionLevel() == 0);
    assert(solver->prop_at_head());

    //This only matters in terms of var elim complexity
    //solver->clauseCleaner->clean_implicit_clauses();
    cls_to_clean_tmp.clear();
    while(last_trail_cleared < solver->trail_size()) {
        Lit l = solver->trail_at(last_trail_cleared++);
        elim_calc_need_update.touch(l.var());
        watch_subarray ws = solver->watches[l];

        //Everything here is satisfied
        uint32_t j = 0;
        for (uint32_t i = 0; i < ws.size(); i ++) {
            Watched& w = ws[i];
            if (w.isBin()) {
                removeWBin(solver->watches, w.lit2(), l, w.red(), w.get_ID());
                if (w.red()) {
                    solver->binTri.redBins--;
                } else {
                    n_occurs[l.toInt()]--;
                    n_occurs[w.lit2().toInt()]--;
                    elim_calc_need_update.touch(w.lit2());
                    solver->binTri.irredBins--;
                }
                *(solver->drat) << del << w.get_ID() << l << w.lit2() << fin;
                continue;
            }

            ws[j++] = w;
            assert(w.isClause());
            ClOffset offs = w.get_offset();
            Clause* cl = solver->cl_alloc.ptr(offs);
            if (cl->getRemoved() || cl->freed()) {
                //Satisfied and removed
                continue;
            }
            //We'll need to set removed, etc.
            cls_to_clean_tmp.push_back(offs);
        }
        ws.resize(j);

        l = ~l;
        watch_subarray ws2 = solver->watches[l];

        //Remove literal
        j = 0;
        for (uint32_t i = 0; i < ws2.size(); i ++) {
            Watched& w = ws2[i];
            if (w.isBin()) {
                assert(solver->value(w.lit2()) == l_True); //we propagate and it'd be UNSAT otherwise
                removeWBin(solver->watches, w.lit2(), l, w.red(), w.get_ID());
                if (w.red()) {
                    solver->binTri.redBins--;
                } else {
                    n_occurs[l.toInt()]--;
                    n_occurs[w.lit2().toInt()]--;
                    elim_calc_need_update.touch(w.lit2());
                    solver->binTri.irredBins--;
                }
                *(solver->drat) << del << w.get_ID() << l << w.lit2() << fin;
                continue;
            }

            ws2[j++] = w;
            assert(w.isClause());
            ClOffset offs = w.get_offset();
            Clause* cl = solver->cl_alloc.ptr(offs);
            if (cl->getRemoved() || cl->freed()) {
                continue;
            }
            cls_to_clean_tmp.push_back(offs);
        }
        ws2.resize(j);
    }

    for(ClOffset offs: cls_to_clean_tmp) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (!cl->getRemoved() && !cl->freed()) {
            if (!clean_clause(offs, true)) {
                return false;
            }
        }
    }

    if (!deal_with_added_long_and_bin(false)) {
        return false;
    }

    return solver->okay();
}

bool OccSimplifier::mark_and_push_to_added_long_cl_cls_containing(const Lit lit)
{
    watch_subarray_const cs = solver->watches[lit];
    *limit_to_decrease -= (long)cs.size()*2+ 40;
    for (const Watched *it = cs.begin(), *end = cs.end()
        ; it != end
        ; ++it
    ) {
        if (it->isClause()) {
            ClOffset offs = it->get_offset();
            Clause* cl = solver->cl_alloc.ptr(offs);

            //Has already been removed or added to "added_long_cl"
            if (cl->freed() || cl->getRemoved() || cl->stats.marked_clause)
                continue;

            cl->stats.marked_clause = 1;
            added_long_cl.push_back(offs);
        }
    }
    return true;
}

bool OccSimplifier::simulate_frw_sub_str_with_added_cl_to_var()
{
    limit_to_decrease = &varelim_sub_str_limit;

    //during the mark_and_push_to_added_long_cl_cls_containing() below, we mark the clauses
    //so we don't add the same clause twice
    for(uint32_t i = 0
        ; i < added_cl_to_var.getTouchedList().size()
        && *limit_to_decrease > 0
        && !solver->must_interrupt_asap()
        ; i++
    ) {
        uint32_t var = added_cl_to_var.getTouchedList()[i];
        Lit lit = Lit(var, true);
        if (!sub_str->backw_sub_str_long_with_bins_watch(lit, true)) {
            return false;
        }
        if (!mark_and_push_to_added_long_cl_cls_containing(lit)) {
            return false;
        }

        lit = ~lit;
        if (!sub_str->backw_sub_str_long_with_bins_watch(lit, true)) {
            return false;
        }
        if (!mark_and_push_to_added_long_cl_cls_containing(lit)) {
            return false;
        }
    }
    added_cl_to_var.clear();

    //here, we clean the marks on the clauses, even in case of timeout/abort
    if (!deal_with_added_long_and_bin(false)) {
        return false;
    }

    limit_to_decrease = &norm_varelim_time_limit;
    #ifdef SLOW_DEBUG
    check_no_marked_clauses();
    #endif

    return true;
}

void OccSimplifier::check_no_marked_clauses()
{
    for(const auto& off: clauses) {
        Clause* cl = solver->cl_alloc.ptr(off);
        if (!cl->getRemoved()) {
            assert(!cl->stats.marked_clause);
        }
    }
}

bool OccSimplifier::eliminate_vars()
{
    assert(solver->okay());
    assert(solver->prop_at_head());
    assert(added_irred_bin.empty());
    assert(added_long_cl.empty());

    //solver->conf.verbosity = 1;
    //Set-up
    double myTime = cpuTime();
    size_t vars_elimed = 0;
    size_t wenThrough = 0;
    time_spent_on_calc_otf_update = 0;
    num_otf_update_until_now = 0;
    int64_t orig_norm_varelim_time_limit = norm_varelim_time_limit;
    limit_to_decrease = &norm_varelim_time_limit;
    assert(cl_to_free_later.empty());
    assert(solver->watches.get_smudged_list().empty());
    bvestats.clear();
    bvestats.numCalls = 1;

    //Go through the ordered list of variables to eliminate
    int64_t last_elimed = 1;
    grow = 0;
    uint32_t n_cls_last  = sum_irred_cls_longs() + solver->binTri.irredBins;
    uint32_t n_cls_init = n_cls_last;
    uint32_t n_vars_last = solver->get_num_free_vars();

    //For debug ONLY
    // subsume with bins everywhere first
//     for(uint32_t i = 0; i < solver->nVars(); i++) {
//         Lit lit = Lit(i, false);
//         if (!sub_str->backw_sub_str_long_with_bins_watch(lit, false)) {
//             goto end;
//         }
//
//         lit = Lit(i, true);
//         if (!sub_str->backw_sub_str_long_with_bins_watch(lit, false)) {
//             goto end;
//         }
//         if (*limit_to_decrease <= 0)
//             break;
//     }
    //

    if (!clear_vars_from_cls_that_have_been_set()) {
        goto end;
    }

    while(varelim_num_limit > 0
        && varelim_linkin_limit_bytes > 0
        && *limit_to_decrease > 0
        && grow < (uint32_t)solver->conf.min_bva_gain
        //&& grow < 1 //solver->conf.min_bva_gain
    ) {
        if (solver->conf.verbosity >= 2) {
            cout << "c x n vars       : " << solver->get_num_free_vars() << endl;
            #ifdef DEBUG_VARELIM
            cout << "c x cls long     : " << sum_irred_cls_longs() << endl;
            cout << "c x cls bin      : " << solver->binTri.irredBins << endl;
            cout << "c x long cls lits: " << sum_irred_cls_longs_lits() << endl;
            #endif
        }

        last_elimed = 0;
        limit_to_decrease = &norm_varelim_time_limit;
        order_vars_for_elim();
        if (velim_order.size() < 400) {
            for(auto& v: solver->varData) {
                v.occ_simp_tried = 0;
            }
        }

        added_cl_to_var.clear();
        removed_cl_with_var.touch(Lit(0, false));
        while(!removed_cl_with_var.getTouchedList().empty()
            && *limit_to_decrease > 0
            && !solver->must_interrupt_asap()
            && solver->okay()
        ) {
            assert(solver->prop_at_head());
            removed_cl_with_var.clear();
            update_varelim_complexity_heap();
            while(!velim_order.empty()
                && *limit_to_decrease > 0
                && varelim_num_limit > 0
                && varelim_linkin_limit_bytes > 0
                && solver->okay()
                && !solver->must_interrupt_asap()
            ) {
                assert(solver->prop_at_head());
                assert(limit_to_decrease == &norm_varelim_time_limit);
                uint32_t var = velim_order.removeMin();

                //Stats
                *limit_to_decrease -= 20;
                wenThrough++;

                if (!can_eliminate_var(var))
                    continue;

                //Try to eliminate
                if (maybe_eliminate(var)) {
                    vars_elimed++;
                    varelim_num_limit--;
                    last_elimed++;
                }
                if (!solver->okay()) {
                    goto end;
                }
                assert(solver->prop_at_head());
                assert(solver->okay());

                if (!clear_vars_from_cls_that_have_been_set()) {
                    goto end;
                }

                //SUB and STR for newly added long and short cls
                limit_to_decrease = &varelim_sub_str_limit;
                if (!deal_with_added_long_and_bin(false)) {
                    limit_to_decrease = &norm_varelim_time_limit;
                    goto end;
                }
                limit_to_decrease = &norm_varelim_time_limit;
                assert(solver->okay());
                assert(solver->prop_at_head());
                update_varelim_complexity_heap();
            }
            assert(solver->prop_at_head());
            assert(added_long_cl.empty());
            assert(added_irred_bin.empty());

            solver->clean_occur_from_removed_clauses_only_smudged();

            if (solver->conf.verbosity >= 2) {
                cout <<"c size of added_cl_to_var    : " << added_cl_to_var.getTouchedList().size() << endl;
                cout <<"c size of removed_cl_with_var: " << removed_cl_with_var.getTouchedList().size() << endl;
            }

            if (!simulate_frw_sub_str_with_added_cl_to_var()) {
                limit_to_decrease = &norm_varelim_time_limit;
                goto end;
            }

            //These WILL ADD VARS BACK even though it's not changed.
            for(uint32_t var: removed_cl_with_var.getTouchedList()) {
                if (!can_eliminate_var(var)) {
                    continue;
                }
                varElimComplexity[var] = heuristicCalcVarElimScore(var);
                velim_order.update(var);
            }


            if (solver->conf.verbosity >= 2) {
                cout << "c x n vars       : " << solver->get_num_free_vars() << endl;
                #ifdef DEBUG_VARELIM
                cout << "c x cls long     : " << sum_irred_cls_longs() << endl;
                cout << "c x cls bin      : " << solver->binTri.irredBins << endl;
                cout << "c x long cls lits: " << sum_irred_cls_longs_lits() << endl;
                #endif
                cout << "c another run ?"<< endl;
            }
        }
        #ifdef DEBUG_VARELIM
        if (solver->conf.verbosity >= 2) {
            cout << "c finished here" << endl;
        }
        #endif
        solver->clean_occur_from_removed_clauses_only_smudged();

        //For debug ONLY
        ///////////////
//         free_clauses_to_free();
// //         backward_sub_str();
// //         limit_to_decrease = &norm_varelim_time_limit;
//         solver->clauseCleaner->clean_implicit_clauses();
//         solver->clean_occur_from_removed_clauses();
        ///////////////

        uint32_t n_cls_now   = sum_irred_cls_longs() + solver->binTri.irredBins;
        uint32_t n_vars_now  = solver->get_num_free_vars();
        double cl_inc_rate = 2.0;
        if (n_cls_last != 0) {
          cl_inc_rate = (double)n_cls_now   / n_cls_last;
        }

        double var_dec_rate = 1.0;
        if (n_vars_now != 0) {
            var_dec_rate = (double)n_vars_last / n_vars_now;
        }
        if (solver->conf.verbosity) {
            cout << "c [occ-bve] iter v-elim " << last_elimed << endl;
            cout << "c cl_inc_rate=" << cl_inc_rate
            << ", var_dec_rate=" << var_dec_rate
            << " (grow=" << grow << ")" << endl;

            cout << "c Reduced to " << solver->get_num_free_vars() << " vars"
            << ", " << sum_irred_cls_longs() + solver->binTri.irredBins
            << " cls (grow=" << grow << ")" << endl;

            if (varelim_num_limit < 0
                || varelim_linkin_limit_bytes < 0
                || *limit_to_decrease < 0
            ) {
                cout << "c [occ-bve] stopped varelim due to outage. "
                << " varelim_num_limit: " << print_value_kilo_mega(varelim_num_limit)
                << " varelim_linkin_limit_bytes: " << print_value_kilo_mega(varelim_linkin_limit_bytes)
                << " *limit_to_decrease: " << print_value_kilo_mega(*limit_to_decrease)
                << endl;
            }
        }

        if (n_cls_now > n_cls_init || cl_inc_rate > (var_dec_rate)) {
            break;
        }
        n_cls_last = n_cls_now;
        n_vars_last = n_vars_now;

        if (grow == 0) {
            grow = 8;
        } else {
            grow *= 2;
        }
        assert(solver->prop_at_head());
        assert(added_long_cl.empty());
        assert(added_irred_bin.empty());
    }

    if (solver->conf.verbosity) {
        cout << "c x n vars       : " << solver->get_num_free_vars() << endl;
        #ifdef DEBUG_VARELIM
        cout << "c x cls long     : " << sum_irred_cls_longs() << endl;
        cout << "c x cls bin      : " << solver->binTri.irredBins << endl;
        cout << "c x long cls lits: " << sum_irred_cls_longs_lits() << endl;
        #endif
    }

end:
    if (solver->okay()) {
        assert(solver->prop_at_head());
        assert(added_long_cl.empty());
        assert(added_irred_bin.empty());
        #ifdef SLOW_DEBUG
        check_no_marked_clauses();
        #endif
    }
    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();

    assert(solver->watches.get_smudged_list().empty());
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain = float_div(*limit_to_decrease, orig_norm_varelim_time_limit);

    if (solver->conf.verbosity) {
        cout
        << "c  #try to eliminate: "<< print_value_kilo_mega(wenThrough) << endl
        << "c  #var-elim        : "<< print_value_kilo_mega(vars_elimed) << endl
        << "c  #T-o: " << (time_out ? "Y" : "N") << endl
        << "c  #T-r: " << std::fixed << std::setprecision(2) << (time_remain*100.0) << "%" << endl
        << "c  #T  : " << time_used << endl;
    }
    if (solver->conf.verbosity) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVarsOuter(), this);
        else
            runStats.print_extra_times();
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

    return solver->okay();
}

void OccSimplifier::free_clauses_to_free()
{
    for(ClOffset off: cl_to_free_later) {
        Clause* cl = solver->cl_alloc.ptr(off);
        solver->free_cl(cl);
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

struct OrderByDecreasingIncidence {
    OrderByDecreasingIncidence(const vector<uint32_t>& _n_occurs):
        n_occurs(_n_occurs)
    {
    }
    bool operator()(const uint32_t v1, const uint32_t v2)
    {
        uint32_t v1_occ = n_occurs[Lit(v1, false).toInt()] + n_occurs[Lit(v1, true).toInt()];
        uint32_t v2_occ = n_occurs[Lit(v2, false).toInt()] + n_occurs[Lit(v2, true).toInt()];

        return v1_occ > v2_occ;
    }

    const vector<uint32_t>& n_occurs;
};

struct MyOccSorter
{
    MyOccSorter(const Solver* _solver) :
        solver(_solver)
    {
    }
    bool operator()(const Watched& w1, const Watched& w2)
    {
        if (w2.isBin())
            return false;

        if (w1.isBin() && !w2.isBin())
            return true;

        //both are non-bin
        const Clause* cl1 = solver->cl_alloc.ptr(w1.get_offset());
        const Clause* cl2 = solver->cl_alloc.ptr(w2.get_offset());

        //The other is at least as good, this is removed
        if (cl1->freed() || cl1->getRemoved())
            return false;

        //The other is not removed, so it's better
        if (cl2->freed() || cl2->getRemoved())
            return true;

        const uint32_t sz1 = cl1->size();
        const uint32_t sz2 = cl2->size();
        return sz1 < sz2;
    }

    const Solver* solver;
};

void OccSimplifier::sort_occurs_and_set_abst()
{
    for(auto& ws: solver->watches) {
        std::sort(ws.begin(), ws.end(), MyOccSorter(solver));

        for(Watched& w: ws) {
            if (w.isClause()) {
                Clause* cl = solver->cl_alloc.ptr(w.get_offset());
                if (cl->freed() || cl->getRemoved()) {
                    w.setBlockedLit(lit_Error);
                } else if (cl->size() >solver->conf.maxXorToFind) {
                    w.setBlockedLit(lit_Undef);
                } else {
                    w.setBlockedLit(Lit::toLit(cl->abst));
                }
            }
        }
    }
}

vector<OrGate> OccSimplifier::get_recovered_or_gates()
{
    vector<OrGate> or_gates;
    auto origTrailSize = solver->trail_size();
    gateFinder = new GateFinder(this, solver);

    startup = false;
    double backup = solver->conf.maxOccurRedMB;
    solver->conf.maxOccurRedMB = 0;
    if (!setup()) {
        delete gateFinder;
        gateFinder = NULL;
        return or_gates;
    }

    gateFinder->find_all();
    or_gates = gateFinder->get_gates();
    gateFinder->cleanup();

    solver->conf.maxOccurRedMB = backup;
    delete gateFinder;
    gateFinder = NULL;
    finishUp(origTrailSize);
    return or_gates;
}

vector<ITEGate> OccSimplifier::get_recovered_ite_gates()
{
    vector<ITEGate> or_gates;
    auto origTrailSize = solver->trail_size();

    startup = false;
    double backup = solver->conf.maxOccurRedMB;
    solver->conf.maxOccurRedMB = 0;
    if (!setup()) {
        delete gateFinder;
        gateFinder = NULL;
        return or_gates;
    }

    vec<Watched> out_a_all;
    for(uint32_t i = 0; i < solver->nVars()*2; i++) {
        const Lit lit = Lit::toLit(i);
        out_a_all.clear();
        gates_poss.clear(); //temps, not needed
        gates_negs.clear(); //temps, not needed
        find_ite_gate(lit, solver->watches[lit], solver->watches[~lit],
                      gates_poss, gates_negs, //temporaries, actually not used
                      &out_a_all); // what we are looking for

        if (out_a_all.empty()) {
            continue;
        }

        //out_a_all contains N*2 clauses, every 2 make up an ITE
        //For each of 2:
        //     x -> (a=f)
        //    -x -> (a=g)
        // and we get back 2x 3-long clauses:
        // -a V  f V -x
        // -a V  g V  x
        // i.e. lhs[0] = f, lhs[2] = g, lhs[1] --> if TRUE selects lhs[2] otherwise lhs[1]
        //cout << "out_a_all.s: " << out_a_all.size() << endl;
        for(uint32_t i2 = 0; i2 < out_a_all.size(); i2+=2) {
            ITEGate gate;
            gate.rhs = lit;
            seen[lit.var()] = 1;
            uint32_t at = 0;

            for(uint32_t x = 0; x < 2; x++) {
                Watched& w = out_a_all[i2+x];
                assert(w.isClause());
                Clause* cl = solver->cl_alloc.ptr(w.get_offset());
                //cout << "c: " << *cl << endl;
                for(const auto&l : *cl) {
                    if (!seen[l.var()]) {
                        gate.lhs[at++] = l;
                        seen[l.var()] = 1;
                    }
                }
            }
            assert(at == 3);

            //Cleanup
            for(const auto& l: gate.get_all()) {
                seen[l.var()] = 0;
            }
            or_gates.push_back(gate);
        }
    }

    solver->conf.maxOccurRedMB = backup;
    finishUp(origTrailSize);
    return or_gates;
}

bool OccSimplifier::occ_rem_with_gates()
{
    assert(solver->okay());
    assert(solver->prop_at_head());
    assert(added_irred_bin.empty());
    assert(added_long_cl.empty());

    double myTime = cpuTime();
    gateFinder = new GateFinder(this, solver);
    gateFinder->find_all();
    const auto gates = gateFinder->get_gates();
    gateFinder->cleanup();
    delete gateFinder;
    gateFinder = NULL;

    uint32_t shortened = 0;
    for(const auto& gate: gates) {
        if (solver->value(gate.rhs) != l_Undef) {
            continue;
        }

        Lit l1 = gate.lit1;
        Lit l2 = gate.lit2;
        if (solver->watches[l1].size() > solver->watches[l1].size()) {
            std::swap(l1, l2);
        }
        solver->watches[l1].copyTo(poss);
        for(const auto& w: poss) {
            if (w.isBin()) {
                continue;
            }
            assert(w.isClause());
            const auto off = w.get_offset();
            Clause* cl = solver->cl_alloc.ptr(w.get_offset());
            if (cl->size() == 3 || //could be the gate definition, skip
                cl->red()) //no need, slow
            {
                continue;
            }
            assert(!cl->getRemoved());
            assert(!cl->freed());

            //TODO check calcAbst!
            uint32_t at = std::numeric_limits<uint32_t>::max();
            for(uint32_t i = 0, sz = cl->size(); i < sz; i++) {
                const Lit l = (*cl)[i];
                if (l > l2) {
                    //lits are sorted, early-abort
                    break;
                }

                if (l == l2) {
                    at = i;
                    break;
                }
            }
            if (at == numeric_limits<uint32_t>::max()) {
                continue;
            }

            (*solver->drat) << deldelay << *cl << fin;
            shortened++;
            (*cl)[at] = gate.rhs; //replace l2
            cl->strengthen(l1); //remove l1
            cl->reCalcAbstraction(); //abst is wrong
            std::sort(cl->begin(), cl->end());
            removeWCl(solver->watches[l2], off);
            removeWCl(solver->watches[l1], off); //TODO we can NOT copy +get rid of this, speedup!
            (*solver->drat) << add << *cl
            #ifdef STATS_NEEDED
            << solver->sumConflicts
            #endif
            << fin << findelay;
            solver->watches[gate.rhs].push(Watched(off, cl->abst));
            n_occurs[l1.toInt()]--;
            n_occurs[l2.toInt()]--;
            n_occurs[gate.rhs.toInt()]++;
            elim_calc_need_update.touch(l1);
            elim_calc_need_update.touch(l2);
            elim_calc_need_update.touch(gate.rhs);
            removed_cl_with_var.touch(l1);
            removed_cl_with_var.touch(l2);
            solver->litStats.irredLits--;
        }
    }
    //Update global stats
    const double time_used = cpuTime() - myTime;
    //const bool time_out = (*limit_to_decrease <= 0);
    if (solver->conf.verbosity) {
        cout
        << "c [occ-gate-based-lit-rem]"
        << " lit-rem: " << shortened
        << solver->conf.print_times(time_used, false)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "occ-gate-based-lit-rem"
            , time_used
        );
    }


    assert(solver->okay());
    assert(solver->prop_at_head());
    return solver->okay();
}

bool OccSimplifier::execute_simplifier_strategy(const string& strategy)
{
    std::istringstream ss(strategy);
    std::string token;

    while(std::getline(ss, token, ',')) {
        if (cpuTime() > solver->conf.maxTime
            || solver->must_interrupt_asap()
            || solver->nVars() == 0
            || !solver->okay()
        ) {
            return solver->okay();
        }
        assert(added_long_cl.empty());
        assert(solver->prop_at_head());
        assert(solver->decisionLevel() == 0);
        assert(cl_to_free_later.empty());
        set_limits();

        #ifdef SLOW_DEBUG
        #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
        for (ClOffset offs: clauses) {
            Clause* cl = solver->cl_alloc.ptr(offs);
            if (cl->freed())
                continue;
            if (!cl->red()) {
                continue;
            }
            assert(solver->red_stats_extra[cl->stats.extra_pos].introduced_at_conflict != 0);
        }
        #endif
        solver->check_implicit_stats(true);
        solver->check_assumptions_sanity();
        check_no_marked_clauses();
        #endif

        token = trim(token);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (token != "" && solver->conf.verbosity) {
            cout << "c --> Executing OCC strategy token: " << token << '\n';
        }

        if (token == "occ-backw-sub-str") {
            backward_sub_str();
        } else if (token == "occ-backw-sub") {
            backward_sub();
        } else if (token == "occ-ternary-res") {
            if (solver->conf.doTernary) {
                ternary_res();
            }
        } else if (token == "occ-xor") {
            if (solver->conf.doFindXors) {
                XorFinder finder(this, solver);
                finder.find_xors();
                #ifdef USE_M4RI
                if (topLevelGauss != NULL && !solver->drat->enabled()) {
                    auto xors = solver->xorclauses;
                    assert(solver->okay());
                    solver->ok = finder.xor_together_xors(xors);
                    if (solver->ok) {
                        vector<Lit> out_changed_occur;
                        finder.remove_xors_without_connecting_vars(xors);
                        topLevelGauss->toplevelgauss(xors, &out_changed_occur);
                        //these may have changed, recalculating occur
                        for(Lit lit: out_changed_occur) {
                            n_occurs[lit.toInt()] = calc_occ_data(lit);
                            n_occurs[(~lit).toInt()] = calc_occ_data(~lit);
                        }
                    }
                }
                #endif
                runStats.xorTime += finder.get_stats().findTime;
            } else {
                //TODO this is something VERY fishy
                if (solver->conf.verbosity) {
                    cout << "c [occ-xor] simulating occ-xor with occur mangling and clause mark cleaning -- TODO very fishy" << endl;
                }
                sort_occurs_and_set_abst();
                for(ClOffset offset: clauses) {
                    Clause* cl = solver->cl_alloc.ptr(offset);
                    cl->stats.marked_clause = 0;
                }
            }
        } else if (token == "occ-lit-rem") {
            all_occ_based_lit_rem();
        } else if (token == "occ-clean-implicit") {
            //BUG TODO
            //solver->clauseCleaner->clean_implicit_clauses();
        } else if (token == "occ-bve") {
            if (solver->conf.doVarElim) {
                solver->removed_xorclauses_clash_vars.clear();
                solver->xor_clauses_updated = true;
                solver->xorclauses.clear();
                solver->xorclauses_unused.clear();

                if (solver->conf.do_empty_varelim) {
                    eliminate_empty_resolvent_vars();
                }
                if (solver->conf.do_full_varelim) {
                    if (!eliminate_vars()) {
                        continue;
                    }
                    if (solver->conf.varelim_check_resolvent_subs && !occ_rem_with_gates()) {
                        continue;
                    }
                }
            }
        } else if (token == "occ-bva") {
            if (solver->conf.do_bva) {
                if (solver->conf.verbosity) {
                    cout << "c [occ-bva] global numcalls: " << globalStats.numCalls << endl;
                }
                if ((globalStats.numCalls % solver->conf.bva_every_n) == (solver->conf.bva_every_n-1)) {
                    if (!bva->bounded_var_addition()) {
                        continue;
                    }
                    added_irred_bin.clear();
                    added_cl_to_var.clear();
                    added_long_cl.clear();
                }
            }
        } else if (token == "") {
            //nothing, ignore empty token
        } else {
             cout << "ERROR: occur strategy '" << token << "' not recognised!" << endl;
            exit(-1);
        }

        #ifdef CHECK_N_OCCUR
        check_n_occur();
        #endif //CHECK_N_OCCUR
    }

    if (solver->okay()) {
        assert(solver->prop_at_head());
    }

    return solver->okay();
}

bool OccSimplifier::setup()
{
    assert(solver->okay());
    assert(toClear.empty());
    added_long_cl.clear();
    added_irred_bin.clear();
    added_cl_to_var.clear();
    n_occurs.clear();
    n_occurs.resize(solver->nVars()*2, 0);

    //Test & debug
    #ifdef DEBUG_ATTACH_MORE
    solver->test_all_clause_attached();
    solver->check_wrong_attach();
    #endif

    //Clean the clauses before playing with them
    solver->clauseCleaner->remove_and_clean_all();

    //If too many clauses, don't do it
    if (solver->getNumLongClauses() > 40ULL*1000ULL*1000ULL*solver->conf.var_and_mem_out_mult
        || solver->litStats.irredLits > 100ULL*1000ULL*1000ULL*solver->conf.var_and_mem_out_mult
    ) {
        if (solver->conf.verbosity) {
            cout << "c [occ] will not link in occur, CNF has too many clauses/irred lits" << endl;
        }
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
    return solver->okay();
}

bool OccSimplifier::simplify(const bool _startup, const std::string& schedule)
{
    #ifdef DEBUG_MARKED_CLAUSE
    assert(solver->no_marked_clauses());
    #endif

    assert(solver->detached_xor_repr_cls.empty());
    #ifdef USE_GAUSS
    assert(solver->gmatrices.empty());
    assert(solver->gqueuedata.empty());
    #endif

    startup = _startup;
    if (!setup()) {
        return solver->okay();
    }

    const size_t origBlockedSize = blockedClauses.size();
    const size_t origTrailSize = solver->trail_size();

    sampling_vars_occsimp.clear();
    if (solver->conf.sampling_vars) {
        // sampling vars should not be eliminated
        assert(!solver->fast_backw.fast_backw_on);
        sampling_vars_occsimp.resize(solver->nVars(), false);
        for(uint32_t outside_var: *solver->conf.sampling_vars) {
            uint32_t outer_var = solver->map_to_with_bva(outside_var);
            outer_var = solver->varReplacer->get_var_replaced_with_outer(outer_var);
            uint32_t int_var = solver->map_outer_to_inter(outer_var);
            if (int_var < solver->nVars()) {
                sampling_vars_occsimp[int_var] = true;
            }
        }
    } else if (solver->fast_backw.fast_backw_on) {
        // fast backward arjun system
        sampling_vars_occsimp.resize(solver->nVars(), false);
        for(Lit p: *solver->fast_backw._assumptions) {
            uint32_t var = solver->fast_backw.indic_to_var->at(p.var());
            p = solver->varReplacer->get_lit_replaced_with_outer(p);
            p = solver->map_outer_to_inter(p);
            assert(solver->varData[p.var()].removed == Removed::none);
            sampling_vars_occsimp[p.var()] = true;

            //Deal with indicators: var, var + orig_num_vars
            if (var == var_Undef) {
                continue;
            }
            uint32_t var2 = var + solver->fast_backw.orig_num_vars;
            var = solver->varReplacer->get_var_replaced_with_outer(var);
            var = solver->map_outer_to_inter(var);
            assert(solver->varData[var].removed == Removed::none);
            if (sampling_vars_occsimp.size() > var) {
                sampling_vars_occsimp[var] = true;
            }

            var2 = solver->varReplacer->get_var_replaced_with_outer(var2);
            var2 = solver->map_outer_to_inter(var2);
            assert(solver->varData[var2].removed == Removed::none);
            if (sampling_vars_occsimp.size() > var2) {
                sampling_vars_occsimp[var2] = true;
            }
        }

        //Deal with test_indic
        uint32_t v = *(solver->fast_backw.test_indic);
        if (v != var_Undef) {
            v = solver->varReplacer->get_var_replaced_with_outer(v);
            v = solver->map_outer_to_inter(v);
            if (sampling_vars_occsimp.size() > v) {
                sampling_vars_occsimp[v] = true;
            }
        }
    } else {
        sampling_vars_occsimp.shrink_to_fit();
    }

    last_trail_cleared = solver->getTrailSize();
    execute_simplifier_strategy(schedule);

    remove_by_drat_recently_blocked_clauses(origBlockedSize);
    finishUp(origTrailSize);

    return solver->okay();
}

bool OccSimplifier::ternary_res()
{
    assert(solver->okay());
    assert(cl_to_add_ternary.empty());
    assert(solver->prop_at_head());
    assert(cl_to_free_later.empty());
    if (clauses.empty()) {
        return solver->okay();
    }

    double myTime = cpuTime();
    int64_t orig_ternary_res_time_limit = ternary_res_time_limit;
    limit_to_decrease = &ternary_res_time_limit;
    Sub1Ret sub1_ret;

    //NOTE: the "clauses" here will change in size as we add resolvents
    size_t at = solver->mtrand.randInt(clauses.size()-1);
    for(size_t i = 0; i < clauses.size(); i++) {
        ClOffset offs = clauses[(at+i) % clauses.size()];
        Clause * cl = solver->cl_alloc.ptr(offs);
        *limit_to_decrease -= 10;
        if (!cl->freed()
            && !cl->getRemoved()
            && !cl->is_ternary_resolved
            && cl->size() == 3
            && !cl->red()
            && *limit_to_decrease > 0
            && ternary_res_cls_limit > 0
        ) {
            if (!perform_ternary(cl, offs, sub1_ret))
                goto end;
        }
    }

    if (!deal_with_added_long_and_bin(false)) {
        goto end;
    }
    assert(added_long_cl.empty());

    end:
    //Update global stats
    const double time_used = cpuTime() - myTime;
    const bool time_out = (*limit_to_decrease <= 0);
    const double time_remain =  float_div(*limit_to_decrease, orig_ternary_res_time_limit);
    if (solver->conf.verbosity) {
        cout
        << "c [occ-ternary-res] Ternary"
        << " res-tri: " << runStats.ternary_added_tri
        << " res-bin: " << runStats.ternary_added_bin
        << " sub: " << sub1_ret.sub
        << " str: " << sub1_ret.str
        << solver->conf.print_times(time_used, time_out, time_remain)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "ternary res"
            , time_used
            , time_out
            , time_remain
        );
    }
    runStats.triresolveTime += time_used;
    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();

    return solver->okay();
}

bool OccSimplifier::perform_ternary(Clause* cl, ClOffset offs, Sub1Ret& sub1_ret)
{
    assert(cl->size() == 3);
    assert(!cl->red());

    cl->is_ternary_resolved = 1;
    *limit_to_decrease -= 3;
    for(const Lit l: *cl) {
        seen[l.toInt()] = 1;
    }

    size_t largest = 0;
    Lit dont_check = lit_Undef;
    for(const Lit l: *cl) {
        size_t sz = n_occurs[l.toInt()] + n_occurs[(~l).toInt()];
        if (sz > largest) {
            largest = sz;
            dont_check = l;
        }
    }

    assert(cl_to_add_ternary.empty());
    for(const Lit l: *cl) {
        if (l == dont_check) {
            continue;
        }
        check_ternary_cl(cl, offs, solver->watches[l]);
        check_ternary_cl(cl, offs, solver->watches[~l]);
    }

    //clean up
    for(const Lit l: *cl) {
        seen[l.toInt()] = 0;
    }

    //Add new ternary resolvents
    for(const Tri& newcl: cl_to_add_ternary) {
        ClauseStats stats;
        stats.last_touched = solver->sumConflicts;
        stats.is_ternary_resolvent = true;
        #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
        ClauseStatsExtra stats_extra;
        stats_extra.introduced_at_conflict = solver->sumConflicts;
        stats_extra.orig_size = 3;
        #endif

        #ifdef FINAL_PREDICTOR
        stats.which_red_array = 2;
        #else
        stats.which_red_array = 1;
        #endif

        #ifdef STATS_NEEDED
        double myrnd = solver->mtrand.randDblExc();
        if (myrnd <= solver->conf.dump_individual_cldata_ratio) {
            //TODO mark clause for dumping
        }
        stats.ID = solver->clauseID++;
        #endif

        tmp_tern_res.clear();
        for(uint32_t i = 0; i < newcl.size; i++) {
            tmp_tern_res.push_back(newcl.lits[i]);
        }

        Clause* newCl = full_add_clause(tmp_tern_res, finalLits_ternary, &stats, true);
        if (newCl) {
            #ifdef STATS_NEEDED
            newCl->stats.locked_for_data_gen =
                solver->mtrand.randDblExc() < solver->conf.lock_for_data_gen_ratio;
            if (newCl->stats.locked_for_data_gen) {
                newCl->stats.which_red_array = 0;
            }
            #endif
            #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
            solver->red_stats_extra.push_back(stats_extra);
            newCl->stats.extra_pos = solver->red_stats_extra.size()-1;
            #endif
            ClOffset off = solver->cl_alloc.get_offset(newCl);
            if (!sub_str->backw_sub_str_with_long(off, sub1_ret)) {
                return false;
            }
        } else {
            //We'd need to check sub1_ret and see if it subsumed an irred
            // then fix it up. Can't be bothered.
//             if (!sub_str->backw_sub_str_with_impl(finalLits_ternary, sub1_ret)) {
//                 return false;
//             }
        }
        *limit_to_decrease-=20;
        ternary_res_cls_limit--;
    }
    cl_to_add_ternary.clear();

    return solver->okay();
}

void OccSimplifier::check_ternary_cl(Clause* cl, ClOffset offs, watch_subarray ws)
{
    *limit_to_decrease -= ws.size()*2;
    for (const Watched& w: ws) {
        if (!w.isClause() || w.get_offset() == offs)
            continue;

        ClOffset offs2 = w.get_offset();
        Clause * cl2 = solver->cl_alloc.ptr(offs2);
        *limit_to_decrease -= 10;
        if (!cl2->freed()
            && !cl2->getRemoved()
            && cl2->size() == 3
            && !cl2->red()
        ) {
            uint32_t num_lits = 0;
            uint32_t num_vars = 0;
            Lit lit_clash = lit_Undef;
            for(Lit l2: *cl2) {
                num_vars += (seen[l2.toInt()] || seen[(~l2).toInt()]);
                num_lits += seen[l2.toInt()];
                if (seen[(~l2).toInt()]) {
                    lit_clash = l2;

                    //It's symmetric so only do it one way
                    // I can resolve
                    //      "a b c" with "a -b d" OR
                    //      "a -b d" with "a b c"
                    // This ensures we only do it one way.
                    if (!lit_clash.sign()) {
                        lit_clash = lit_Error;
                        break;
                    }
                }
            }

            //Not tri resolveeable or the wrong side of the symmetry
            if (lit_clash == lit_Error) {
                continue;
            }

            //Becomes tri
            if ((num_vars == 2 && num_lits == 1) ||
                (solver->conf.allow_ternary_bin_create &&
                    num_vars == 3 && num_lits == 2)
            ) {
                *limit_to_decrease-=20;

                Tri newcl;
                for(Lit l: *cl) {
                    if (l.var() != lit_clash.var())
                        newcl.lits[newcl.size++] = l;
                }
                for(Lit l: *cl2) {
                    if (l.var() != lit_clash.var()
                        && !seen[l.toInt()]
                    ) {
                        newcl.lits[newcl.size++] = l;
                    }
                }

                assert(newcl.size < 4 && newcl.size > 1);
                if (newcl.size == 2 || newcl.size == 3) {
                    if (newcl.size == 2) {
                        runStats.ternary_added_bin++;
                    } else {
                        assert(newcl.size == 3);
                        runStats.ternary_added_tri++;
                    }
                    cl_to_add_ternary.push_back(newcl);
                }
            }
        }
    }
}



void OccSimplifier::backward_sub()
{
    auto backup = subsumption_time_limit;
    subsumption_time_limit = 0;
    limit_to_decrease = &subsumption_time_limit;
    assert(cl_to_free_later.empty());

    subsumption_time_limit += (int64_t)
        ((double)backup*solver->conf.subsumption_time_limit_ratio_sub_str_w_bin);

    assert(cl_to_free_later.empty());
    assert(solver->watches.get_smudged_list().empty());

//     if (!sub_str->backw_sub_str_long_with_bins()
//         || solver->must_interrupt_asap()
//     ) {
//         goto end;
//     }

    subsumption_time_limit += (int64_t)
        ((double)backup*solver->conf.subsumption_time_limit_ratio_sub_w_long);
    sub_str->backw_sub_long_with_long();

    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();
}

bool OccSimplifier::backward_sub_str()
{
    assert(cl_to_free_later.empty());
    assert(solver->watches.get_smudged_list().empty());

    auto backup = subsumption_time_limit;
    subsumption_time_limit = 0;
    limit_to_decrease = &subsumption_time_limit;

    //Sub-str long with bins
    subsumption_time_limit += (int64_t)
        ((double)backup*solver->conf.subsumption_time_limit_ratio_sub_str_w_bin);
    if (!sub_str->backw_sub_str_long_with_bins()
        || solver->must_interrupt_asap()
    ) {
        goto end;
    }

    //Sub long with long
    subsumption_time_limit += (int64_t)
        ((double)backup*solver->conf.subsumption_time_limit_ratio_sub_w_long);
    sub_str->backw_sub_long_with_long();
    if (solver->must_interrupt_asap())
        goto end;

    //Sub+Str long with long
    limit_to_decrease = &strengthening_time_limit;
    if (!sub_str->backw_str_long_with_long()
        || solver->must_interrupt_asap()
    ) {
        goto end;
    }

    //Deal with added long and bin
    if (!deal_with_added_long_and_bin(true)
        || solver->must_interrupt_asap()
    ) {
        goto end;
    }

    end:
    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();

    return solver->okay();
}

bool OccSimplifier::fill_occur()
{
    //Calculate binary clauses' contribution to n_occurs
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
            if (it2->isBin() && !it2->red() && lit < it2->lit2()) {
                n_occurs[lit.toInt()]++;
                n_occurs[it2->lit2().toInt()]++;
            }
        }
    }

    //Add irredundant to occur
    uint64_t memUsage = calc_mem_usage_of_occur(solver->longIrredCls);
    print_mem_usage_of_occur(memUsage);
    if (memUsage > solver->conf.maxOccurIrredMB*1000ULL*1000ULL*solver->conf.var_and_mem_out_mult) {
        if (solver->conf.verbosity) {
            cout << "c [occ] Memory usage of occur is too high, unlinking and skipping occur" << endl;
        }
        CompleteDetachReatacher detRet(solver);
        detRet.reattachLongs(true);
        return false;
    }

    link_in_data_irred = link_in_clauses(
        solver->longIrredCls
        , true //add to occur list
        , std::numeric_limits<uint32_t>::max()
        , std::numeric_limits<int64_t>::max()
    );
    solver->longIrredCls.clear();
    if (solver->conf.verbosity) {
        cout << "c [occ] Linked in IRRED BIN by default: " << solver->binTri.irredBins << endl;
        cout << "c [occ] Linked in RED   BIN by default: " << solver->binTri.redBins << endl;
    }
    print_linkin_data(link_in_data_irred);

    //Add redundant to occur
    if (solver->conf.maxRedLinkInSize > 0) {
        memUsage = calc_mem_usage_of_occur(solver->longRedCls[0]);
        print_mem_usage_of_occur(memUsage);
        bool linkin = true;
        if (memUsage > solver->conf.maxOccurRedMB*1000ULL*1000ULL*solver->conf.var_and_mem_out_mult) {
            linkin = false;
        }
        //Sort, so we get the shortest ones in at least
        uint32_t arr_to_link = 0;
        std::sort(solver->longRedCls[arr_to_link].begin(), solver->longRedCls[arr_to_link].end()
            , ClauseSizeSorter(solver->cl_alloc));

        link_in_data_red = link_in_clauses(
            solver->longRedCls[arr_to_link]
            , linkin
            , solver->conf.maxRedLinkInSize
            , solver->conf.maxOccurRedLitLinkedM*1000ULL*1000ULL*solver->conf.var_and_mem_out_mult
        );
        solver->longRedCls[arr_to_link].clear();
    }

    //Don't really link in the rest
    for(auto& lredcls: solver->longRedCls) {
        link_in_clauses(lredcls, false, 0, 0);
    }
    for(auto& lredcls: solver->longRedCls) {
        lredcls.clear();
    }

    LinkInData combined(link_in_data_irred);
    combined.combine(link_in_data_red);
    print_linkin_data(combined);

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
    uint32_t at_blocked_cls = blk_var_to_cls[var];
    if (at_blocked_cls == std::numeric_limits<uint32_t>::max())
        return solver->okay();

    //Eliminate it in practice
    //NOTE: Need to eliminate in theory first to avoid infinite loops

    //Mark for removal from blocked list
    blockedClauses[at_blocked_cls].toRemove = true;
    can_remove_blocked_clauses = true;
    assert(blockedClauses[at_blocked_cls].at(0, blkcls).var() == var);

    //Re-insert into Solver
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout
    << "Uneliminating cl ";
    for(size_t i=0; i< blockedClauses[at_blocked_cls].size(); i++){
        cout << blockedClauses[at_blocked_cls].at(i, blkcls) << " ";
    }
    cout << " on var " << var+1
    << endl;
    #endif

    vector<Lit> lits;
    size_t bat = 1;
    while(bat < blockedClauses[at_blocked_cls].size()) {
        Lit l = blockedClauses[at_blocked_cls].at(bat, blkcls);
        if (l == lit_Undef) {
            solver->add_clause_outer_copylits(lits);
            if (!solver->okay()) {
                return false;
            }
            lits.clear();
        } else {
            lits.push_back(l);
        }
        bat++;
    }

    return solver->okay();
}

void OccSimplifier::remove_by_drat_recently_blocked_clauses(size_t origBlockedSize)
{
    if (! ((*solver->drat).enabled() || solver->conf.simulate_drat) )
        return;

    if (solver->conf.verbosity >= 6) {
        cout << "c Deleting blocked clauses for DRAT" << endl;
    }

    uint32_t at_ID = 0;
    vector<Lit> lits;
    for(size_t i = origBlockedSize; i < blockedClauses.size(); i++) {
        lits.clear();
        size_t at = 1;
        while(at < blockedClauses[i].size()) {
            const Lit l = blockedClauses[i].at(at, blkcls);
            if (l == lit_Undef) {
                const uint64_t ID = newly_blocked_cls_IDs[at_ID++];
                (*solver->drat) << del << ID << lits << fin;
                lits.clear();
            } else {
                lits.push_back(solver->map_outer_to_inter(l));
            }
            at++;
        }
    }
    newly_blocked_cls_IDs.clear();
}

void OccSimplifier::buildBlockedMap()
{
    blk_var_to_cls.clear();
    blk_var_to_cls.resize(solver->nVarsOuter(), std::numeric_limits<uint32_t>::max());
    for(size_t i = 0; i < blockedClauses.size(); i++) {
        const BlockedClauses& blocked = blockedClauses[i];
        uint32_t blockedon = blocked.at(0, blkcls).var();
        assert(blockedon < blk_var_to_cls.size());
        blk_var_to_cls[blockedon] = i;
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
    remove_all_longs_from_watches();
    if (solver->ok) {
        assert(solver->prop_at_head());
        add_back_to_solver();
        if (solver->okay()) {
            solver->ok = solver->propagate<true>().isNULL();
        }
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
    sub_str->finishedRun();

    //Sanity checks
    if (solver->okay()
        #ifndef SLOW_DEBUG
         && somethingSet
        #endif
    ) {
        #ifdef SLOW_DEBUG
        solver->test_all_clause_attached();
        solver->check_wrong_attach();
        solver->check_stats();
        solver->check_implicit_propagated();
        #endif
    }

    if (solver->ok) {
        check_elimed_vars_are_unassignedAndStats();
    }

    //Let's just clean up ourselves a bit
    clauses.clear();
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
    varelim_sub_str_limit      = 1000ULL*1000ULL*solver->conf.varelim_sub_str_limit
        *solver->conf.global_timeout_multiplier;
    ternary_res_time_limit     = 1000ULL*1000ULL*solver->conf.ternary_res_time_limitM
        *solver->conf.global_timeout_multiplier;
    occ_based_lit_rem_time_limit = 1000ULL*1000ULL*solver->conf.occ_based_lit_rem_time_limitM
        *solver->conf.global_timeout_multiplier;
    ternary_res_cls_limit = link_in_data_irred.cl_linked * solver->conf.ternary_max_create;

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
    varelim_sub_str_limit *= 10;

    varelim_num_limit = ((double)solver->get_num_free_vars() * solver->conf.varElimRatioPerIter);
    varelim_linkin_limit_bytes = solver->conf.var_linkin_limit_MB *1000LL*1000LL*solver->conf.var_and_mem_out_mult;

    if (!solver->conf.do_strengthen_with_occur) {
        strengthening_time_limit = 0;
    }

    //For debugging
//     subsumption_time_limit = 0;
//     strengthening_time_limit = 0;
//     norm_varelim_time_limit = 0;
//     empty_varelim_time_limit = 0;
//     varelim_num_limit = 0;
//     subsumption_time_limit   = std::numeric_limits<int64_t>::max();
//     strengthening_time_limit = std::numeric_limits<int64_t>::max();
//     norm_varelim_time_limit  = std::numeric_limits<int64_t>::max();
//     empty_varelim_time_limit = std::numeric_limits<int64_t>::max();
//     varelim_num_limit        = std::numeric_limits<int64_t>::max();
//     varelim_sub_str_limit    = std::numeric_limits<int64_t>::max();
}

void OccSimplifier::cleanBlockedClauses()
{
    assert(solver->decisionLevel() == 0);
    vector<BlockedClauses>::iterator i = blockedClauses.begin();
    vector<BlockedClauses>::iterator j = blockedClauses.begin();

    uint64_t i_blkcls = 0;
    uint64_t j_blkcls = 0;
    for (vector<BlockedClauses>::iterator
        end = blockedClauses.end()
        ; i != end
        ; ++i
    ) {
        const uint32_t blockedOn = solver->map_outer_to_inter(i->at(0, blkcls).var());
        if (solver->varData[blockedOn].removed == Removed::elimed
            && solver->value(blockedOn) != l_Undef
        ) {
            std::cerr
            << "ERROR: var " << Lit(blockedOn, false) << " elimed,"
            << " value: " << solver->value(blockedOn)
            << endl;
            assert(false);
            std::exit(-1);
        }

        if (i->toRemove) {
            blockedMapBuilt = false;
            i_blkcls += i->size();
            assert(i_blkcls == i->end);
            i->start = std::numeric_limits<uint64_t>::max();
            i->end = std::numeric_limits<uint64_t>::max();
        } else {
            assert(solver->varData[blockedOn].removed == Removed::elimed);

            //beware we might change this
            const size_t sz = i->size();

            //don't copy if we don't need to
            if (!blockedMapBuilt) {
                for(size_t x = 0; x < sz; x++) {
                    blkcls[j_blkcls++] = blkcls[i_blkcls++];
                }
            } else {
                i_blkcls += sz;
                j_blkcls += sz;
            }
            assert(i_blkcls == i->end);
            i->start = j_blkcls-sz;
            i->end   = j_blkcls;
            *j++ = *i;
        }
    }
    blkcls.resize(j_blkcls);
    blockedClauses.resize(blockedClauses.size()-(i-j));
    can_remove_blocked_clauses = false;
}

void OccSimplifier::rem_cls_from_watch_due_to_varelim(
    const Lit lit
    , bool add_to_block
) {
    blockedMapBuilt = false;

    //Copy&clear i.e. MOVE
    solver->watches[lit].moveTo(tmp_rem_cls_copy);
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
            assert(!cl.freed());

            //Put clause into blocked status
            if (add_to_block) {
                if (!cl.red()) {
                    bvestats.clauses_elimed_long++;
                    bvestats.clauses_elimed_sumsize += cl.size();

                    lits.resize(cl.size());
                    std::copy(cl.begin(), cl.end(), lits.begin());
                    add_clause_to_blck(lits, cl.stats.ID);
                } else {
                    red = true;
                }
            }

            //Remove -- only DRAT the ones that are redundant
            //The irred will be removed thanks to 'blocked' system
            unlink_clause(offset, cl.red(), true, true);
        } else if (watch.isBin()) {
            //Update stats
            if (!watch.red()) {
                bvestats.clauses_elimed_bin++;
                bvestats.clauses_elimed_sumsize += 2;
            } else {
                red = true;
            }

            //Put clause into blocked status
            lits.resize(2);
            lits[0] = lit;
            lits[1] = watch.lit2();
            if (!watch.red()) {
                if (add_to_block) {
                    add_clause_to_blck(lits, watch.get_ID());
                }
                n_occurs[lits[0].toInt()]--;
                n_occurs[lits[1].toInt()]--;
                removed_cl_with_var.touch(lits[0]);
                removed_cl_with_var.touch(lits[1]);
                elim_calc_need_update.touch(lits[0]);
                elim_calc_need_update.touch(lits[1]);
            } else {
                //If redundant, delayed blocked-based DRAT deletion will not work
                //so delete explicitly
                (*solver->drat) << del << watch.get_ID() << lits[0] << lits[1] << fin;
            }

            //Remove
            //*limit_to_decrease -= (long)solver->watches[lits[0]].size()/4; //This is zero
            *limit_to_decrease -= (long)solver->watches[lits[1]].size()/4;
            solver->detach_bin_clause(lits[0], lits[1], red, watch.get_ID(), true, true);
        } else {
            assert(false);
        }

        if (solver->conf.verbosity >= 3 && !lits.empty()) {
            cout
            << "Eliminated clause " << lits << " (red: " << red << ")"
            << " on var " << lit.var()+1
            << endl;
        }
    }
}

void OccSimplifier::add_clause_to_blck(const vector<Lit>& lits, const uint64_t ID)
{
    for(const Lit& l: lits) {
        removed_cl_with_var.touch(l.var());
        elim_calc_need_update.touch(l.var());
    }

    vector<Lit> lits_outer = lits;
    solver->map_inter_to_outer(lits_outer);
    for(Lit l: lits_outer) {
        blkcls.push_back(l);
    }
    blkcls.push_back(lit_Undef);
    blockedClauses.back().end = blkcls.size();
    newly_blocked_cls_IDs.push_back(ID);
}

bool OccSimplifier::find_or_gate(
    Lit elim_lit
    , watch_subarray_const a
    , watch_subarray_const b
    , vec<Watched>& out_a
    , vec<Watched>& out_b
) {
    bool found = false;
    out_a.clear();
    out_b.clear();

    assert(toClear.empty());
    for(const Watched w: a) {
        if (w.isBin()) {
            #ifdef SLOW_DEBUG
            assert(!w.red());
            #endif
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
            #ifdef SLOW_DEBUG
            assert(!solver->redundant_or_removed(w));
            #endif
            Clause* cl = solver->cl_alloc.ptr(w.get_offset());

            assert(cl->size() > 2);
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
                out_b.push(w);
                for(const Lit lit: *cl) {
                    if (lit != ~elim_lit) {
                        assert(false && "FRAT ID needed, 0 is wrong");
                        out_a.push(Watched(~lit, false, 0));
                    }
                }
                found = true;
                break;
            }
        }
    }

    for(Lit l: toClear) {
        seen[l.toInt()] = 0;
    }
    toClear.clear();

    return found;
}

bool OccSimplifier::find_ite_gate(
    Lit elim_lit
    , watch_subarray_const a
    , watch_subarray_const b
    , vec<Watched>& out_a
    , vec<Watched>& out_b
    , vec<Watched>* out_a_all
) {
    int limit = solver->conf.varelim_gate_find_limit;
    bool found = false;
    out_a.clear();
    out_b.clear();

    //gate description we are looking for where "a" is being defineed
    // as  x -> (a=f)
    //    -x -> (a=g)
    // these translate to:
    // -a V  f V -x
    // -a V  g V  x
    //  a V -f V -x
    //  a V -g V  x
    //where elim_lit = -a
    //      lits[0]  =  f
    //      lits[1]  = -x
    //      lits[2]  =  g
    Lit lits[3];
    assert(toClear.empty());
    for(uint32_t i = 0; i < a.size() && limit >= 0; i++, limit--) {
        const Watched& w = a[i];
        if (w.isBin()) {
            #ifdef SLOW_DEBUG
            assert(!w.red());
            #endif
            continue;
        }

        assert(w.isClause());
        #ifdef SLOW_DEBUG
        assert(!solver->redundant_or_removed(w));
        #endif
        Clause* cl = solver->cl_alloc.ptr(w.get_offset());
        if (cl->size() != 3) {
            continue;
        }

        //Clear seen
        for(const auto& l: toClear) {
            seen[l.var()] = 0;
        }
        toClear.clear();
        out_a.push(w);

        //Set up base
        uint32_t at = 0;
        for(const auto& l: *cl) {
            if (l == elim_lit) {
                continue;
            }
            lits[at++] = l;
            seen[l.var()] = 1;
            toClear.push_back(l);
        }
        assert(at == 2);

        //Find 2nd base: -a V  g V  x
        for(uint32_t j = i+1; j < a.size(); j++, limit--) {
            const Watched& w2 = a[j];
            if (w2.isBin()) {
                continue;
            }
            Clause* cl2 = solver->cl_alloc.ptr(w2.get_offset());
            if (cl2->size() != 3) {
                continue;
            }

            uint32_t match = 0;
            bool ok = false;
            bool ok2 = false;
            for(const auto& l: *cl2) {
                match += seen[l.var()];
                if (l == ~lits[1]) {
                    ok = true;
                }
                if (l == ~lits[2]) {
                    ok2 = true;
                }
            }
            if (match != 1) {
                continue;
            }

            if (!ok && !ok2) {
                continue;
            }
            if (!ok && ok2) {
//                 cout << "Swapped" << endl;
                std::swap(lits[0], lits[1]);
            }

            //Make elim_lit 1st position
            if (elim_lit == (*cl2)[1]) {
                std::swap((*cl2)[0], (*cl2)[1]);
            }
            if (elim_lit == (*cl2)[2]) {
                std::swap((*cl2)[0], (*cl2)[2]);
            }

            //Make ~lits[1] (i.e. x) 2nd position
            if (~lits[1] == (*cl2)[2]) {
                std::swap((*cl2)[2], (*cl2)[1]);
            }
            lits[2] = (*cl2)[2];

            std::sort(cl2->begin(), cl2->end());
            seen[lits[2].var()] = 1;
            toClear.push_back(lits[2]);
            out_a.push(w2);
            break;
        }

        if (out_a.size() != 2) {
            continue;
        }

//         cout << "Start" << endl;
        out_b.clear();
        bool got_mf_v_mx = false;
        bool got_mg_v_x = false;
        for(uint32_t j = 0; j < b.size(); j++, limit--) {
            const Watched& w2 = b[j];
            if (w2.isBin()) {
                continue;
            }
            Clause* cl2 = solver->cl_alloc.ptr(w2.get_offset());
            if (cl2->size() != 3) {
                continue;
            }
            uint32_t match = 0;
            for(const auto& l: *cl2) {
                match += seen[l.var()];
            }
            if (match != 2) {
                continue;
            }
//             cout << "base: " << *cl << endl;
//             cout << "elim lit: " << elim_lit << endl;
//             cout << "lits[0]: " << lits[0] << endl;
//             cout << "lits[1]: " << lits[1] << endl;
//             cout << "lits[1]: " << lits[2] << endl;
//             cout << "check " << *cl2 << endl;


            //Make ~elim_lit 1st position
            if (~elim_lit == (*cl2)[1]) {
                std::swap((*cl2)[0], (*cl2)[1]);
            }
            if (~elim_lit == (*cl2)[2]) {
                std::swap((*cl2)[0], (*cl2)[2]);
            }

            //Make lits[1].var() (i.e. (~)x) 2nd position
            if (lits[1].var() == (*cl2)[2].var()) {
                std::swap((*cl2)[1], (*cl2)[2]);
            }

            //it's -x here, so must have -f i.e. ~lits[0]
            //  a V -f V -x
            if ((*cl2)[1] == lits[1] &&
                (*cl2)[2] == ~lits[0] &&
                !got_mf_v_mx)
            {
                std::sort(cl2->begin(), cl2->end());
                out_b.push(w2);
                got_mf_v_mx = true;
                continue;
            }

            //it's x here, so must have -g i.e. ~lits[2]
            if ((*cl2)[1] == ~lits[1] &&
                (*cl2)[2] == ~lits[2] &&
                !got_mg_v_x)
            {
                std::sort(cl2->begin(), cl2->end());
                out_b.push(w2);
                got_mg_v_x = true;
                continue;
            }
            std::sort(cl2->begin(), cl2->end());

            if (got_mf_v_mx && got_mg_v_x) {
                break;
            }
        }
        if (got_mf_v_mx && got_mg_v_x) {
            found = true;
            if (out_a_all == NULL) {
                break;
            } else {
                assert(out_a.size() == 2);
                out_a_all->push(out_a[0]);
                out_a_all->push(out_a[1]);
            }
        }
    }

    if (limit < 0) {
        //cout << "ITE Gate find timeout limit reached" << endl;
        bvestats.gatefind_timeouts++;
    }

    for(Lit l: toClear) {
        seen[l.var()] = 0;
    }
    toClear.clear();

    if (found && out_a_all == NULL) {
        assert(out_a.size() == 2);
        assert(out_b.size() == 2);
        std::sort(out_a.begin(), out_a.end(), sort_smallest_first(solver->cl_alloc));
        std::sort(out_b.begin(), out_b.end(), sort_smallest_first(solver->cl_alloc));
    }

    return found;
}


bool OccSimplifier::find_equivalence_gate(
    [[maybe_unused]] Lit elim_lit
    , watch_subarray_const a
    , watch_subarray_const b
    , vec<Watched>& out_a
    , vec<Watched>& out_b
) {
    assert(toClear.empty());

    bool found = false;
    out_a.clear();
    out_b.clear();

    for(const Watched& w: a) {
        if (w.isBin()) {
            #ifdef SLOW_DEBUG
            assert(!w.red());
            #endif
            seen[w.lit2().toInt()] = 1;
            toClear.push_back(w.lit2());
        }
    }

    for(const Watched& w: b) {
        if (w.isBin()) {
            #ifdef SLOW_DEBUG
            assert(!w.red());
            #endif
            if (seen[(~w.lit2()).toInt()]) {
                out_b.push(w);
                out_a.push(Watched(~w.lit2(), false, w.get_ID()));
                found = true;
                break;
            }
        }
    }

    for(const auto& l: toClear) {
        seen[l.toInt()] = 0;
    }
    toClear.clear();


    if (found) {
        //cout << "EQ gate" << endl;
    }

    return found;
}

bool OccSimplifier::find_xor_gate(
    [[maybe_unused]] Lit elim_lit
    , watch_subarray_const a
    , watch_subarray_const b
    , vec<Watched>& out_a
    , vec<Watched>& out_b
) {
    assert(toClear.empty());
    //cout << "Finding xor gate" << endl;

    bool found = false;
    int limit = solver->conf.varelim_gate_find_limit;
    out_a.clear();
    out_b.clear();
    assert(toclear_marked_cls.empty());
    assert(parities_found.empty());

    uint32_t maxsize = 7;
    maxsize = std::min((int)maxsize, (int)std::log2(a.size())+1);
    maxsize = std::min((int)maxsize, (int)std::log2(b.size())+1);

    bool parity;
    uint32_t size;
    uint32_t tofind;
    for(uint32_t j = 0; j < a.size() && limit >= 0; j++, limit--) {
        const Watched& w = a[j];
        if (w.isBin()) {
            continue;
        }

        assert(w.isClause());
        Clause* cl = solver->cl_alloc.ptr(w.get_offset());
        if (cl->size() > maxsize || cl->stats.marked_clause || cl->red()) {
            continue;
        }
        size = cl->size();
        tofind = 1ULL<<(size-1);

        //Clear seen
        for(const auto& l: toClear) {
            seen[l.var()] = 0;
        }
        toClear.clear();

        parity = 0;
        for(const auto& l: *cl) {
            seen [l.var()] = 1;
            toClear.push_back(l);
            parity ^= l.sign();
        }
        out_a.clear();
        out_a.push(w);
        cl->stats.marked_clause = 1;
        toclear_marked_cls.push_back(cl);
        std::sort(cl->begin(), cl->end());
        uint32_t val = 0;
        for(uint32_t i2 = 0; i2 < cl->size(); i2 ++) {
            if ((*cl)[i2].sign()) {
                val += 1 << i2;
            }
        }
        parities_found.clear();
        parities_found.insert(val);

        for(uint32_t i = j+1; i < a.size(); i++) {
            const Watched& w2 = a[i];
            if (w2.isBin()) {
                continue;
            }
            assert(w2.isClause());
            Clause* cl2 = solver->cl_alloc.ptr(w2.get_offset());
            #ifdef SLOW_DEBUG
            assert(!cl2->red());
            #endif
            if (cl2->size() != size || cl2->stats.marked_clause) {
                continue;
            }
            bool this_cl_ok = true;
            bool myparity = 0;
            for(const auto& l2: *cl2) {
                if (!seen[l2.var()]) {
                    this_cl_ok = false;
                    break;
                }
                myparity ^= l2.sign();
            }
            //cout << "Mypar: " << myparity << " real par: " << parity << " ok: " << this_cl_ok << endl;
            if (this_cl_ok && myparity == parity) {
                cl2->stats.marked_clause = 1;
                toclear_marked_cls.push_back(cl2);
                std::sort(cl2->begin(), cl2->end());
                val = 0;
                for(uint32_t i2 = 0; i2 < cl2->size(); i2 ++) {
                    if ((*cl2)[i2].sign()) {
                        val += 1 << i2;
                    }
                }
                if (parities_found.find(val) == parities_found.end()) {
                    out_a.push(w2);
                    parities_found.insert(val);
                }
            }
        }

        //Early abort, we should have found 3 by now
        //cout << "Here par find s: " << parities_found.size() << endl;
        if (parities_found.size() != tofind/2) {
            continue;
        }

        out_b.clear();
        for(uint32_t i = 0; i < b.size(); i++, limit--) {
            const Watched& w2 = b[i];
            if (w2.isBin()) {
                continue;
            }
            assert(w2.isClause());
            Clause* cl2 = solver->cl_alloc.ptr(w2.get_offset());
            #ifdef SLOW_DEBUG
            assert(!cl2->red());
            #endif
            if (cl2->size() != size || cl2->stats.marked_clause) {
                continue;
            }
            bool this_cl_ok = true;
            bool myparity = 0;
            for(const auto& l2: *cl2) {
                if (!seen[l2.var()]) {
                    this_cl_ok = false;
                    break;
                }
                myparity ^= l2.sign();
            }
            //cout << "Mypar: " << myparity << " real par: " << parity << " ok: " << this_cl_ok << endl;
            if (this_cl_ok && myparity == parity) {
                cl2->stats.marked_clause = 1;
                toclear_marked_cls.push_back(cl2);
                std::sort(cl2->begin(), cl2->end());
                val = 0;
                for(uint32_t i2 = 0; i2 < cl2->size(); i2 ++) {
                    if ((*cl2)[i2].sign()) {
                        val += 1 << i2;
                    }
                }
                if (parities_found.find(val) == parities_found.end()) {
                    out_b.push(w2);
                    parities_found.insert(val);
                }
            }
            uint32_t so_far = parities_found.size();

            if (so_far == tofind) {
                found = true;
                break;
            }

            //Early abort -- there is not enough left in "b" to be able to do work
            if (tofind-so_far > (b.size() - i - 1)) {
                break;
            }
        }
        if (found) {
            break;
        }
    }

    if (limit < 0) {
//         cout << "XOR Gate find limit reached" << endl;
        bvestats.gatefind_timeouts++;
    }

    //Clear seen
    for(const auto& l: toClear) {
        seen[l.var()] = 0;
    }
    toClear.clear();

    //Cleark cl markings
    for(const auto& cl: toclear_marked_cls) {
        cl->stats.marked_clause = 0;
    }
    toclear_marked_cls.clear();
    parities_found.clear();


    if (found) {
        //cout << "XOR gate" << endl;
        assert(out_a.size() == tofind/2);
        assert(out_b.size() == tofind/2);
        std::sort(out_a.begin(), out_a.end(), sort_smallest_first(solver->cl_alloc));
        std::sort(out_b.begin(), out_b.end(), sort_smallest_first(solver->cl_alloc));
    } else {
        out_a.clear();
        out_b.clear();
    }

    return found;
}

bool OccSimplifier::try_remove_lit_via_occurrence_simpl(
    const OccurClause& occ_cl)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->prop_at_head());
    if (occ_cl.ws.isBin()) {
        return false;
    }

    solver->new_decision_level();
    Clause* cl = solver->cl_alloc.ptr(occ_cl.ws.get_offset());
    assert(!cl->getRemoved());
    assert(!cl->freed());

    bool conflicted = false;
    bool found_it = false;
    bool can_remove_cl = false;
    for(Lit l: *cl) {
        if (l != occ_cl.lit) {
            l = ~l;
        } else {
            found_it = true;
        }

        const lbool val = solver->value(l);
        if (val == l_False) {
            //either  it's the ORIG lit -- then the lit can be removed
            if (l == occ_cl.lit) {
                conflicted = true;
            } else { //it's an INVERTED lit -- then the whole clause can be removed.
                assert(false && "Not possible, we cleaned up the clauses from satisfied");
            }
            break;
        }
        if (val == l_Undef) {
            solver->enqueue<true>(l);
        }
    }

    //No conflict at decision level 0, let's propagate
    if (!conflicted && !can_remove_cl) {
        assert(found_it);
        conflicted = !solver->propagate_occur<true>();
    }
    solver->cancelUntil<false, true>(0);

    assert(solver->decisionLevel() == 0);
    return conflicted;
}

bool OccSimplifier::forward_subsume_irred(
    const Lit lit,
    cl_abst_type abs,
    const uint32_t size)
{
    for(const auto& w: solver->watches[lit]) {
        if (w.isBin()) {
            if (!w.red() && seen[w.lit2().toInt()]) {
                return true;
            }
        } else {
            assert(w.isClause());
            Clause* cl = solver->cl_alloc.ptr(w.get_offset());
            if (cl->freed() || cl->getRemoved() || cl->red()) {
                continue;
            }
            if (cl->size() >= size || !subsetAbst(cl->abst, abs)) {
                continue;
            }

            bool ok = true;
            for(const auto& l: *cl) {
                if (!seen[l.toInt()]) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                return true;
            }

        }
    }
    return false;
}

bool OccSimplifier::generate_resolvents(
    vec<Watched>& tmp_poss,
    vec<Watched>& tmp_negs,
    Lit lit,
    uint32_t limit)
{
    size_t at_poss = 0;
    for (const Watched* it = tmp_poss.begin(), *end = tmp_poss.end()
        ; it != end
        ; ++it, at_poss++
    ) {
        *limit_to_decrease -= 3;
        #ifdef SLOW_DEBUG
        assert(!solver->redundant_or_removed(*it));
        #endif

        size_t at_negs = 0;
        for (const Watched *it2 = tmp_negs.begin(), *end2 = tmp_negs.end()
            ; it2 != end2
            ; it2++, at_negs++
        ) {
            *limit_to_decrease -= 3;
            assert(!solver->redundant_or_removed(*it2));

            //Resolve the two clauses
            bool tautological = resolve_clauses(*it, *it2, lit);
            if (tautological) {
                continue;
            }

            if (solver->satisfied(dummy)) {
                continue;
            }

            #ifdef VERBOSE_DEBUG_VARELIM
            cout << "Adding new clause due to varelim: " << dummy << endl;
            #endif

            //Early-abort or over time
            if (resolvents.size()+1 > limit
                //Too long resolvent
                || (solver->conf.velim_resolvent_too_large != -1
                    && ((int)dummy.size() > solver->conf.velim_resolvent_too_large))
                //Over-time
                || *limit_to_decrease < -10LL*1000LL

            ) {
                return false;
            }

            //Calculate new clause stats
            ClauseStats stats;
            bool is_xor = false;
            if (it->isBin() && it2->isClause()) {
                Clause* c = solver->cl_alloc.ptr(it2->get_offset());
                stats = c->stats;
                is_xor |= c->used_in_xor();
            } else if (it2->isBin() && it->isClause()) {
                Clause* c = solver->cl_alloc.ptr(it->get_offset());
                stats = c->stats;
                is_xor |= c->used_in_xor();
            } else if (it2->isClause() && it->isClause()) {
                Clause* c1 = solver->cl_alloc.ptr(it->get_offset());
                Clause* c2 = solver->cl_alloc.ptr(it2->get_offset());
                //Neither are redundant, this works.
                stats = ClauseStats::combineStats(c1->stats, c2->stats);
                is_xor |= c1->used_in_xor();
                is_xor |= c2->used_in_xor();
            }
            //must clear marking that has been set due to gate
            resolvents.add_resolvent(dummy, stats, is_xor);
        }
    }

    return true;
}

void OccSimplifier::get_antecedents(
    const vec<Watched>& gates,
    const vec<Watched>& full_set,
    vec<Watched>& output)
{
    //both of gates and full_set are strictly sorted and cleaned from REDundant
    output.clear();
    uint32_t j = 0;
    for(uint32_t i = 0; i < full_set.size(); i++) {
        const Watched& w = full_set[i];
        if (solver->redundant_or_removed(w))
            continue;
        if (j < gates.size()) {
            if (gates[j] == w) {
                j++;
                continue;
            }
        }
        output.push(w);
    }

    assert(output.size() == full_set.size() - gates.size());
}

void OccSimplifier::clean_from_red_or_removed(
    const vec<Watched>& in,
    vec<Watched>& out)
{
    out.clear();
    for(const auto& w: in) {
        if (!solver->redundant_or_removed(w)) {
            out.push(w);
        }
    }
}

void OccSimplifier::clean_from_satisfied(vec<Watched>& in)
{
    uint32_t j = 0;
    uint32_t i = 0;
    for(; i < in.size(); i++) {
        const Watched& w = in[i];
        if (w.isBin()) {
            if (solver->value(w.lit2()) == l_Undef) {
                in[j++] = in[i];
            }
            continue;
        }

        assert(w.isClause());
        if (!solver->satisfied(w.get_offset())) {
            in[j++] = in[i];
        }
        continue;
    }
    in.shrink(i-j);
}

//Return true if it worked
bool OccSimplifier::test_elim_and_fill_resolvents(const uint32_t var)
{
    assert(solver->ok);
    assert(solver->varData[var].removed == Removed::none);
    assert(solver->value(var) == l_Undef);
    resolvents.clear();
    const Lit lit = Lit(var, false);

    //Gather data
    #ifdef CHECK_N_OCCUR
    if (n_occurs[Lit(var, false).toInt()] != calc_data_for_heuristic(Lit(var, false))) {
        cout << "lit " << Lit(var, false) << endl;
        cout << "n_occ is: " << n_occurs[Lit(var, false).toInt()] << endl;
        cout << "calc is: " << calc_data_for_heuristic(Lit(var, false)) << endl;
        assert(false);
    }

    if (n_occurs[Lit(var, true).toInt()] != calc_data_for_heuristic(Lit(var, true))) {
        cout << "lit " << Lit(var, true) << endl;
        cout << "n_occ is: " << n_occurs[Lit(var, true).toInt()] << endl;
        cout << "calc is: " << calc_data_for_heuristic(Lit(var, true)) << endl;
    }
    #endif
    uint32_t pos = n_occurs[Lit(var, false).toInt()];
    uint32_t neg = n_occurs[Lit(var, true).toInt()];

    //set-up
    clean_from_red_or_removed(solver->watches[lit], poss);
    clean_from_red_or_removed(solver->watches[~lit], negs);
    assert(poss.size() == pos);
    assert(negs.size() == neg);
    clean_from_satisfied(poss);
    clean_from_satisfied(negs);
    pos = poss.size();
    neg = negs.size();

    //Pure literal, no resolvents
    //we look at "pos" and "neg" (and not poss&negs) because we don't care about redundant clauses
    if (pos == 0 || neg == 0) {
        return true;
    }


    // A smaller OR gate will lead to less BIN (and 1 long) clause.
    //The total size of the resolvent is
    // |G_a|*|R_NOTa| + |G_NOTa|*|R_a|.
    // Let's say it's [50,50], gate is short, [1,2]
    // 1 * 48 + 2* 49 = 146
    // Let's say it's [50,50], gate is lomg, [1,6]
    // 1 * 44 + 6* 49 =  338
    // So must sort smallest first to find the short gate first!

    std::sort(poss.begin(), poss.end(), sort_smallest_first(solver->cl_alloc));
    std::sort(negs.begin(), negs.end(), sort_smallest_first(solver->cl_alloc));

    //Too expensive to check, it's futile
    if ((uint64_t)neg * (uint64_t)pos
        >= solver->conf.varelim_cutoff_too_many_clauses
    ) {
        return false;
    }

    // see:  http://baldur.iti.kit.edu/sat/files/ex04.pdf
    bool gates = false;
    if (find_equivalence_gate(lit, poss, negs, gates_poss, gates_negs)) {
        gates = true;
    } else if (find_or_gate(lit, poss, negs, gates_poss, gates_negs)) {
        gates = true;
    } else if (find_or_gate(~lit, negs, poss, gates_negs, gates_poss)) {
        gates = true;
    } else if (find_ite_gate(lit, poss, negs, gates_poss, gates_negs)) {
        gates = true;
    } else if (find_ite_gate(~lit, negs, poss, gates_negs, gates_poss)) {
        gates = true;
    } else if (find_xor_gate(lit, poss, negs, gates_poss, gates_negs)) {
        gates = true;
    }

    if (gates && solver->conf.verbosity > 5) {
        cout << "Elim on gate, lit: " << lit << " g poss: ";
        for(const auto& w: gates_poss) {
            if (w.isClause()) {
                cout << " [" << *solver->cl_alloc.ptr(w.get_offset()) << "], ";
            } else {
                cout << w << ", ";
            }
        }
        cout << " -- g negs: ";
        for(const auto& w: gates_negs) {
            cout << w << ", ";
        }
        cout << endl;
    }

    std::sort(gates_poss.begin(), gates_poss.end(), sort_smallest_first(solver->cl_alloc));
    std::sort(gates_negs.begin(), gates_negs.end(), sort_smallest_first(solver->cl_alloc));
    //TODO We could just filter negs, poss below
    get_antecedents(gates_negs, negs, antec_negs);
    get_antecedents(gates_poss, poss, antec_poss);
    const uint32_t limit = pos+neg+grow;

    if (gates) {
        if (!generate_resolvents(gates_poss, antec_negs, lit, limit)) {
            return false;
        } else if (!generate_resolvents(gates_negs, antec_poss, ~lit, limit)) {
            return false;
//         } else if (!generate_resolvents(gates_poss, gates_negs, lit, limit)) {
//             return false;
        }
    } else {
        if (!generate_resolvents(antec_poss, antec_negs, lit, limit)) {
            return false;
        }
    }

    if (solver->conf.varelim_check_resolvent_subs) {
        tmp_subs.clear();
        int64_t total_lits = 0;
        for(uint32_t i = 0; i < resolvents.at; i++) {
            const auto& lits = resolvents.resolvents_lits[i];
            total_lits += lits.size();
            sub_str->find_subsumed(
                CL_OFFSET_MAX,
                lits,
                calcAbstraction(lits),
                tmp_subs,
                true //only irred
            );
        }
        std::sort(tmp_subs.begin(), tmp_subs.end());
        tmp_subs.erase(unique(tmp_subs.begin(),tmp_subs.end()),tmp_subs.end());
        for(const auto& cl_off: tmp_subs) {
            if (cl_off.ws.isBin()) {
                total_lits -= 2;
                continue;
            }
            assert(cl_off.ws.isClause());
            Clause* cl = solver->cl_alloc.ptr(cl_off.ws.get_offset());
            total_lits -= cl->size();
        }
        for(const auto& w: poss) {
            if (w.isBin()) {
                total_lits -= 2;
                continue;
            }
            assert(w.isClause());
            Clause* cl = solver->cl_alloc.ptr(w.get_offset());
            total_lits -= cl->size();
        }
        for(const auto& w: negs) {
            if (w.isBin()) {
                total_lits -= 2;
                continue;
            }
            assert(w.isClause());
            Clause* cl = solver->cl_alloc.ptr(w.get_offset());
            total_lits -= cl->size();
        }

        //TODO some of the subsumed clauses could be the clauses in POSS and NEGS. Have to check.
        if ((int)resolvents.size() - (int)tmp_subs.size() < (int)limit-15
            || (int)total_lits < std::min<int>(grow-20, -10)
        ) {
            return true;
        } else {
            return false;
        }
    }

    return true;
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
            const Clause& cl = *solver->cl_alloc.ptr(w.get_offset());
            if (cl.getRemoved())
                continue;
            cout
            << "Clause--> "
            << cl
            << "(red: " << cl.red()
            << ")"
            << "(rem: " << cl.getRemoved()
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
    , const bool is_xor
) {
    assert(solver->okay());
    assert(solver->prop_at_head());

    bvestats.newClauses++;
    Clause* newCl = NULL;

    if (solver->conf.verbosity >= 5) {
        cout
        << "adding v-elim resolvent: "
        << finalLits
        << endl;
    }

    ClauseStats backup_stats(stats);
    newCl = solver->add_clause_int(
        finalLits //Literals in new clause
        , false //Is the new clause redundant?
        , &backup_stats//Statistics for this new clause (usage, etc.)
        , false //Should clause be attached?
        , &finalLits //Return final set of literals here
    );

    if (solver->okay()) {
        solver->ok = solver->propagate_occur<false>();
    }
    if (!solver->okay()) {
        return false;
    }

    if (newCl != NULL) {
        newCl->set_used_in_xor(is_xor);
        link_in_clause(*newCl);
        ClOffset offset = solver->cl_alloc.get_offset(newCl);
        clauses.push_back(offset);
        added_long_cl.push_back(offset);

        // 4 = clause itself
        // 8 = watch (=occur) space
        varelim_linkin_limit_bytes -= (int64_t)finalLits.size()*(4+8);
        varelim_linkin_limit_bytes -= (int64_t)sizeof(Clause);
    } else if (finalLits.size() == 2) {
        n_occurs[finalLits[0].toInt()]++;
        n_occurs[finalLits[1].toInt()]++;
        added_irred_bin.push_back(std::make_pair(finalLits[0], finalLits[1]));

        // 8 = watch space
        varelim_linkin_limit_bytes -= (int64_t)finalLits.size()*(8);
    }

    //Touch every var of the new clause, so we re-estimate
    //elimination complexity for this var
    for(Lit lit: finalLits) {
        #ifdef CHECK_N_OCCUR
        if(n_occurs[lit.toInt()] != calc_data_for_heuristic(lit)) {
            cout << "n_occurs[lit.toInt()]:" << n_occurs[lit.toInt()] << endl;
            cout << "calc_data_for_heuristic(lit): " << calc_data_for_heuristic(lit) << endl;
            cout << "cl: " << finalLits << endl;
            cout << "lit: " << lit << endl;
            assert(false);
        }
        #endif
        elim_calc_need_update.touch(lit.var());
        added_cl_to_var.touch(lit.var());
    }

    return true;
}

void OccSimplifier::check_n_occur()
{
    for (size_t i=0; i < solver->nVars(); i++) {
        Lit lit(i, false);
        const uint32_t pos = calc_occ_data(lit);
        if (pos != n_occurs[lit.toInt()]) {
            cout << "for lit: " << lit << endl;
            cout << "pos is: " << pos
            << " n_occurs is:" << n_occurs[lit.toInt()] << endl;
            assert(false);
        }

        const uint32_t neg = calc_occ_data(~lit);
        if (neg != n_occurs[(~lit).toInt()]) {
            cout << "for lit: " << lit << endl;
            cout << "neg is: " << neg
            << " n_occurs is:" << n_occurs[(~lit).toInt()] << endl;
            assert(false);
        }
    }
}

void OccSimplifier::update_varelim_complexity_heap()
{
    num_otf_update_until_now++;
    for(uint32_t var: elim_calc_need_update.getTouchedList()) {
        //No point in updating the score of this var
        if (!can_eliminate_var(var) || !velim_order.inHeap(var)) {
            continue;
        }

        auto prev = varElimComplexity[var];
        varElimComplexity[var] = heuristicCalcVarElimScore(var);

        //If different, PUT IT BACK IN, and update the heap
        if (prev != varElimComplexity[var]) {
            velim_order.update(var);
        }
    }
    elim_calc_need_update.clear();

    #ifdef CHECK_N_OCCUR
    for(uint32_t var = 0; var < solver->nVars(); var++) {
        if (!can_eliminate_var(var) || !velim_order.inHeap(var)) {
            continue;
        }

        auto prev = varElimComplexity[var];
        varElimComplexity[var] = heuristicCalcVarElimScore(var);
        if (prev != varElimComplexity[var]) {
            cout << "prev: " << prev << " now: " << varElimComplexity[var] << endl;
        }
        assert(prev == varElimComplexity[var]);
    }
    #endif
}

void OccSimplifier::print_var_elim_complexity_stats(const uint32_t var) const
{
    if (solver->conf.verbosity >= 5) {
        cout << "var " << var +1 << " trying complexity: " << varElimComplexity[var] << endl;
    }
}

void OccSimplifier::set_var_as_eliminated(const uint32_t var)
{
    const Lit lit = Lit(var, false);
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
    blkcls.push_back(solver->map_inter_to_outer(lit));
    blockedClauses.push_back(
        BlockedClauses(blkcls.size()-1, blkcls.size())
    );
    blockedMapBuilt = false;
}

bool OccSimplifier::occ_based_lit_rem(uint32_t var, uint32_t& removed) {
    assert(solver->decisionLevel() == 0);

    removed = 0;
    for(int i = 0; i < 2; i++) {
        Lit lit(var, i);
        solver->watches[lit].copyTo(poss);

        for(const auto& w: poss) {
            if (!w.isClause()) {
                continue;
            }

            const ClOffset offset = w.get_offset();
            Clause* cl = solver->cl_alloc.ptr(offset);
            if (cl->getRemoved() || cl->red()) {
                continue;
            }
            assert(!cl->freed());

            if (solver->satisfied(*cl)) {
                unlink_clause(offset, true, true, true);
                continue;
            }

            if (try_remove_lit_via_occurrence_simpl(OccurClause(lit, w))) {
// TODO Not sure if this is needed.... I think it is?
//                 occ_based_lit_rem_tmp.clear();
//                 for(const auto& l: *cl) {
//                     if (l == lit) {
//                         occ_based_lit_rem_tmp.push_back(~l);
//                     } else {
//                         occ_based_lit_rem_tmp.push_back(l);
//                     }
//                 }
//                 (*solver->drat) << add << occ_based_lit_rem_tmp << fin;
                remove_literal(offset, lit, true);
//                 (*solver->drat) << del << occ_based_lit_rem_tmp << fin;
                if (!solver->okay()) {
                    return false;
                }

                removed++;
            }
        }
    }
    return solver->okay();
}

bool OccSimplifier::all_occ_based_lit_rem()
{
    assert(solver->okay());
    assert(solver->prop_at_head());

    double myTime = cpuTime();
    //TODO this is not being used, bogoprops is not checked here
    limit_to_decrease = &occ_based_lit_rem_time_limit;

    //Order them for removal
    vector<uint32_t> vars;
    for(uint32_t v = 0; v < solver->nVars(); v++) {
        if (solver->varData[v].removed == Removed::none &&
            solver->value(v) == l_Undef)
        {
            vars.push_back(v);
        }
    }
    std::sort(vars.begin(), vars.end(), OrderByDecreasingIncidence(n_occurs));

    //Try to remove
    uint32_t removed_all = 0;
    for(const auto& v: vars) {
        uint32_t all = n_occurs[Lit(v, false).toInt()] + n_occurs[Lit(v, true).toInt()];
        if (all == 0) {
            continue;
        }
        uint32_t removed = 0;
        if (!occ_based_lit_rem(v, removed)) {
            goto end;
        }
        removed_all += removed;
        if (solver->conf.verbosity >= 5) {
            cout << "occ-lit-rem finished var " << v
            << " occ_p: " << n_occurs[Lit(v, false).toInt()]
            << " occ_n: " << n_occurs[Lit(v, true).toInt()]
            << " rem: " << removed
            << endl;
        }
    }

    if (!deal_with_added_long_and_bin(false)) {
        goto end;
    }

    end:
    solver->clean_occur_from_removed_clauses_only_smudged();
    free_clauses_to_free();

    if (solver->okay()) {
        assert(solver->prop_at_head());
        solver->check_implicit_propagated();
    }

    double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout
        << "c [occ-lit-rem] Occ Lit Rem: " << removed_all
        << solver->conf.print_times(time_used)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "occ based lit rem"
            , time_used
        );
    }

    return solver->okay();
}

bool OccSimplifier::maybe_eliminate(const uint32_t var)
{
    assert(solver->ok);
    assert(solver->prop_at_head());

    print_var_elim_complexity_stats(var);
    bvestats.testedToElimVars++;
    const Lit lit = Lit(var, false);


    if (solver->conf.varelim_check_resolvent_subs &&
        !solver->varData[var].occ_simp_tried &&
        (n_occurs[lit.toInt()] + n_occurs[(~lit).toInt()] < 20))
    {
        solver->varData[var].occ_simp_tried = 1;
        uint32_t rem = 0;
        occ_based_lit_rem(var, rem);
    }
    if (solver->value(var) != l_Undef ||
        !solver->okay()
    ) {
        return false;
    }

    if (!test_elim_and_fill_resolvents(var)
        || *limit_to_decrease < 0
    ) {
        return false;  //didn't eliminate :(
    }
    bvestats.triedToElimVars++;

    print_var_eliminate_stat(lit);

    //Remove clauses
    create_dummy_blocked_clause(lit);
    rem_cls_from_watch_due_to_varelim(lit);
    rem_cls_from_watch_due_to_varelim(~lit);

    //Add resolvents
    while(!resolvents.empty()) {
        if (!add_varelim_resolvent(resolvents.back_lits(),
            resolvents.back_stats(), resolvents.back_xor())
        ) {
            goto end;
        }
        resolvents.pop();
    }
    limit_to_decrease = &norm_varelim_time_limit;

end:
    set_var_as_eliminated(var);

    return true; //eliminated!
}

void OccSimplifier::add_pos_lits_to_dummy_and_seen(
    const Watched& ps
    , const Lit& posLit
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
    const Watched& qs
    , const Lit& posLit
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

bool OccSimplifier::resolve_clauses(
    const Watched& ps
    , const Watched& qs
    , const Lit& posLit
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

    dummy.clear();
    add_pos_lits_to_dummy_and_seen(ps, posLit);
    bool tautological = add_neg_lits_to_dummy_and_seen(qs, posLit);

    *limit_to_decrease -= (long)dummy.size()/2 + 1;
    for (const Lit lit: dummy) {
        seen[lit.toInt()] = 0;
    }

    return tautological;
}

uint32_t OccSimplifier::calc_data_for_heuristic(const Lit lit)
{
    uint32_t ret = 0;
    watch_subarray_const ws_list = solver->watches[lit];

    //BUT WATCHES ARE SMUDGED!!
    //THIS IS WRONG!!
    /*if (link_in_data_red.cl_linked < 100) {
        ret = ws_list.size();
        return ret;
    }*/

    *limit_to_decrease -= (long)ws_list.size()*3 + 100;
    for (const Watched ws: ws_list) {
        //Skip redundant clauses
        if (solver->redundant(ws))
            continue;

        switch(ws.getType()) {
            case watch_binary_t:
                ret++;
                break;

            case watch_clause_t: {
                const Clause* cl = solver->cl_alloc.ptr(ws.get_offset());
                if (!cl->getRemoved()) {
                    assert(!cl->freed() && "Inside occur, so cannot be freed");
                    ret++;
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

uint32_t OccSimplifier::calc_occ_data(const Lit lit)
{
    uint32_t ret = 0;
    watch_subarray_const ws_list = solver->watches[lit];
    for (const Watched ws: ws_list) {
        //Skip redundant clauses
        if (solver->redundant(ws))
            continue;

        switch(ws.getType()) {
            case watch_binary_t:
                ret++;
                break;

            case watch_clause_t: {
                const Clause* cl = solver->cl_alloc.ptr(ws.get_offset());
                if (!cl->getRemoved()) {
                    assert(!cl->freed() && "Inside occur, so cannot be freed");
                    ret++;
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
                        int num = __builtin_popcountll(seen[(~ws.lit2()).toInt()]);
                        assert(num <= otherSize);
                        count += otherSize - num;
                        break;
                }

                //this "if" is only here to avoid undefined shift-up error by
                //clang sanitizer
                if (action == ResolvCount::set && numCls < 15) {
                    at = at << 1U;
                }
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
                //this "if" is only here to avoid undefined shift-up error by
                //clang sanitizer
                if (action == ResolvCount::set && numCls < 15) {
                    at = at << 1U;
                }
                numCls++;

                //Count using tmp
                if (action == ResolvCount::count) {
                    int num = __builtin_popcountll(tmp);
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

uint64_t OccSimplifier::heuristicCalcVarElimScore(const uint32_t var)
{
    const Lit lit(var, false);
    #ifdef CHECK_N_OCCUR
    const uint32_t pos = calc_data_for_heuristic(lit);
    if (pos != n_occurs[lit.toInt()]) {
        cout << "for lit: " << lit << endl;
        cout << "pos is: " << pos
        << " n_occurs is:" << n_occurs[lit.toInt()] << endl;
        assert(false);
    }

    const uint32_t neg = calc_data_for_heuristic(~lit);
    if (neg != n_occurs[(~lit).toInt()]) {
        cout << "for lit: " << lit << endl;
        cout << "neg is: " << neg
        << " n_occurs is:" << n_occurs[(~lit).toInt()] << endl;
        assert(false);
    }
    #endif

    return  (uint64_t)n_occurs[lit.toInt()] * (uint64_t)n_occurs[(~lit).toInt()];
}

void OccSimplifier::order_vars_for_elim()
{
    velim_order.clear();
    varElimComplexity.clear();
    varElimComplexity.resize(solver->nVars(), 0);
    elim_calc_need_update.clear();

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
        varElimComplexity[var] = heuristicCalcVarElimScore(var);
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
    b += added_long_cl.capacity()*sizeof(ClOffset);
    b += sub_str->mem_used();
    b += blockedClauses.capacity()*sizeof(BlockedClauses);
    b += blkcls.capacity()*sizeof(Lit);
    b += blk_var_to_cls.size()*sizeof(uint32_t);
    b += velim_order.mem_used();
    b += varElimComplexity.capacity()*sizeof(int)*2;
    b += elim_calc_need_update.mem_used();
    b += clauses.capacity()*sizeof(ClOffset);
    b += sampling_vars_occsimp.capacity();

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

void OccSimplifier::link_in_clause(Clause& cl)
{
    assert(!cl.stats.marked_clause);
    assert(cl.size() > 2);
    ClOffset offset = solver->cl_alloc.get_offset(&cl);
    cl.recalc_abst_if_needed();
    if (!cl.red()) {
        for(const Lit l: cl){
            n_occurs[l.toInt()]++;
            added_cl_to_var.touch(l.var());
        }
    }
    assert(cl.stats.marked_clause == 0 && "marks must always be zero at linkin");

    std::sort(cl.begin(), cl.end());
    for (const Lit lit: cl) {
        watch_subarray ws = solver->watches[lit];
        ws.push(Watched(offset, cl.abst));
    }
    cl.setOccurLinked(true);
}

double OccSimplifier::Stats::total_time(OccSimplifier* occs) const
{
    return linkInTime + varElimTime + xorTime + triresolveTime
        + finalCleanupTime
        + occs->sub_str->get_stats().subsumeTime
        + occs->sub_str->get_stats().strengthenTime
        + occs->bvestats_global.timeUsed
        + occs->bva->get_stats().time_used;
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
    varElimTime += other.varElimTime;
    xorTime += other.xorTime;
    triresolveTime += other.triresolveTime;
    finalCleanupTime += other.finalCleanupTime;
    zeroDepthAssings += other.zeroDepthAssings;
    ternary_added_tri += other.ternary_added_tri;
    ternary_added_bin += other.ternary_added_bin;

    return *this;
}

BVEStats& BVEStats::operator+=(const BVEStats& other)
{
    numVarsElimed += other.numVarsElimed;
    varElimTimeOut += other.varElimTimeOut;
    clauses_elimed_long += other.clauses_elimed_long;
    clauses_elimed_bin += other.clauses_elimed_bin;
    clauses_elimed_sumsize += other.clauses_elimed_sumsize;
    testedToElimVars += other.testedToElimVars;
    triedToElimVars += other.triedToElimVars;
    newClauses += other.newClauses;
    subsumedByVE  += other.subsumedByVE;
    gatefind_timeouts += other.gatefind_timeouts;

    return *this;
}

void OccSimplifier::Stats::print_extra_times() const
{

    cout
    << "c [occur] " << linkInTime+finalCleanupTime << " is overhead"
    << endl;

    cout
    << "c [occur] link-in T: " << linkInTime
    << " cleanup T: " << finalCleanupTime
    << endl;
}

void OccSimplifier::Stats::print(const size_t nVars, OccSimplifier* occs) const
{
    cout << "c -------- OccSimplifier STATS ----------" << endl;
    print_stats_line("c time"
        , total_time(occs)
        , stats_line_percent(varElimTime, total_time(occs))
        , "% var-elim"
    );

    print_stats_line("c called"
        ,  numCalls
        , float_div(total_time(occs), numCalls)
        , "s per call"
    );

    print_stats_line("c 0-depth assigns"
        , zeroDepthAssings
        , stats_line_percent(zeroDepthAssings, nVars)
        , "% vars"
    );

    cout << "c -------- OccSimplifier STATS END ----------" << endl;
}

/*void OccSimplifier::save_state(SimpleOutFile& f)
{
    assert(solver->decisionLevel() == 0);
    cleanBlockedClauses();
    f.put_uint64_t(blockedClauses.size());
    for(const BlockedClauses& c: blockedClauses) {
        c.save_to_file(f);
    }
    f.put_vector(blkcls);
    f.put_struct(globalStats);
    f.put_uint32_t(anythingHasBeenBlocked);
}*/

Clause* OccSimplifier::full_add_clause(
    const vector<Lit>& tmp_cl,
    vector<Lit>& finalLits,
    ClauseStats* cl_stats,
    bool red)
{
    Clause* newCl = solver->add_clause_int(
        tmp_cl//Literals in new clause
        , red //Is the new clause redundant?
        , cl_stats
        , false //Should clause be attached if long?
        , &finalLits
    );

    if (solver->okay()) {
        solver->ok = solver->propagate_occur<false>();
    }
    if (!solver->okay()) {
        return NULL;
    }

    if (!newCl) {
        if (finalLits.size() == 2 && !red) {
            n_occurs[finalLits[0].toInt()]++;
            n_occurs[finalLits[1].toInt()]++;
            added_irred_bin.push_back(std::make_pair(finalLits[0], finalLits[1]));
        }
    } else {
        link_in_clause(*newCl);
        ClOffset offset = solver->cl_alloc.get_offset(newCl);
        clauses.push_back(offset);
    }

    return newCl;
}

bool OccSimplifier::remove_literal(
    ClOffset offset,
    const Lit toRemoveLit,
    bool only_set_is_removed)
{
    Clause& cl = *solver->cl_alloc.ptr(offset);
    #ifdef VERBOSE_DEBUG
    cout << "-> Strenghtening clause :" << cl;
    cout << " with lit: " << toRemoveLit << endl;
    #endif

    *limit_to_decrease -= 5;

    (*solver->drat) << deldelay << cl << fin;
    cl.strengthen(toRemoveLit);
    added_cl_to_var.touch(toRemoveLit.var());
    cl.recalc_abst_if_needed();

    cl.stats.ID = solver->clauseID++;
    (*solver->drat) << add << cl << fin << findelay;
    if (!cl.red()) {
        n_occurs[toRemoveLit.toInt()]--;
        elim_calc_need_update.touch(toRemoveLit.var());
        removed_cl_with_var.touch(toRemoveLit.var());
    }

    removeWCl(solver->watches[toRemoveLit], offset);
    if (cl.red())
        solver->litStats.redLits--;
    else
        solver->litStats.irredLits--;

    return clean_clause(offset, only_set_is_removed);
}
