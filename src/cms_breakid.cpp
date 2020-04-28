/******************************************
Copyright (c) 2019, Mate Soos

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

#include "cms_breakid.h"
#include "solver.h"
#include "clausecleaner.h"
#include "breakid/breakid.hpp"
#include "varupdatehelper.h"
#include "varreplacer.h"
#include "occsimplifier.h"
#include "subsumeimplicit.h"
#include "sqlstats.h"
#include "completedetachreattacher.h"

using namespace CMSat;

BreakID::BreakID(Solver* _solver):
    solver(_solver)
{
}

void BreakID::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& /*interToOuter*/)
{
    if (symm_var != var_Undef) {
        symm_var = getUpdatedVar(symm_var, outerToInter);
    }
}

template<class T>
BreakID::add_cl_ret BreakID::add_this_clause(const T& cl)
{
    uint32_t sz = 0;
    bool sat = false;
    brkid_lits.clear();
    for(size_t i3 = 0; i3 < cl.size(); i3++) {
        Lit lit = cl[i3];
        assert(solver->varData[lit.var()].removed == Removed::none);
        lbool val = l_Undef;
        if (solver->value(lit) != l_Undef) {
            val = solver->value(lit);
        } else {
            val = solver->lit_inside_assumptions(lit);
        }

        if (val == l_True) {
            //clause is SAT, skip!
            sat = true;
            continue;
        } else if (val == l_False) {
            continue;
        }
        brkid_lits.push_back(lit);
        sz++;
    }
    if (sat) {
        return add_cl_ret::skipped_cl;
    }
    if (sz == 0) {
        //it's unsat because of assumptions
        if (solver->conf.verbosity) {
            cout << "c [breakid] UNSAT because of assumptions in clause: " << cl << endl;
        }
        return add_cl_ret::unsat;
    }

    num_lits_in_graph += brkid_lits.size();
    breakid->add_clause((BID::BLit*)brkid_lits.data(), brkid_lits.size());
    brkid_lits.clear();

    return add_cl_ret::added_cl;
}

struct EqCls {
    EqCls(ClauseAllocator& _alloc) :
        cl_alloc(_alloc)
    {}

    bool operator()(ClOffset off1, ClOffset off2) {
        Clause* cl1 = cl_alloc.ptr(off1);
        Clause* cl2 = cl_alloc.ptr(off2);

        if (cl1->stats.hash_val != cl2->stats.hash_val) {
            return cl1->stats.hash_val < cl2->stats.hash_val;
        }

        if (cl1->size() != cl2->size()) {
            return cl1->size() < cl2->size();
        }

        //same hash, same size
        for(uint32_t i = 0; i < cl1->size(); i++) {
            if (cl1->getData()[i] != cl2->getData()[i]) {
                return (cl1->getData()[i] < cl2->getData()[i]);
            }
        }

        //they are equivalent
        return false;
    }

    ClauseAllocator& cl_alloc;
};

static bool equiv(Clause* cl1, Clause* cl2) {
    if (cl1->stats.hash_val != cl2->stats.hash_val) {
        return false;
    }

    if (cl1->size() != cl2->size()) {
        return false;
    }

    for(uint32_t i = 0; i < cl1->size(); i++) {
        if (cl1->getData()[i] != cl2->getData()[i]) {
            return false;
        }
    }

    return true;
}

void BreakID::set_up_time_lim()
{
    set_time_lim = solver->conf.breakid_time_limit_K;
    if (solver->nVars() < 5000) {
        set_time_lim*=2;
    }
    if (num_lits_in_graph < 100000) {
        set_time_lim*=2;
    }

    set_time_lim *= 1000LL;
    if (solver->conf.verbosity) {
        cout << "c [breakid] set time lim: " << set_time_lim << endl;
    }

    breakid->set_steps_lim(set_time_lim);
}

bool BreakID::add_clauses()
{
    //Add binary clauses
    vector<Lit> this_clause;
    for(size_t i2 = 0; i2 < solver->nVars()*2; i2++) {
        Lit lit = Lit::toLit(i2);
        for(const Watched& w: solver->watches[lit]) {
            if (w.isBin() && !w.red() && lit < w.lit2()) {
                this_clause.clear();
                this_clause.push_back(lit);
                this_clause.push_back(w.lit2());

                if (add_this_clause(this_clause) == add_cl_ret::unsat) {
                    return false;
                }
            }
        }
    }

    //Add long clauses
    for(ClOffset offs: dedup_cls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
        assert(!cl->getRemoved());

        if (add_this_clause(*cl) == add_cl_ret::unsat) {
            return false;
        }
    }

    return true;
}

///returns whether we actually ran
bool BreakID::doit()
{
    assert(solver->okay());
    assert(solver->decisionLevel() == 0);
    num_lits_in_graph = 0;

    if (!solver->conf.doStrSubImplicit) {
        if (solver->conf.verbosity) {
            cout
            << "c [breakid] cannot run BreakID without implicit submsumption, "
            << "it would find too many (bad) symmetries"
            << endl;
        }
        return false;
    }

    if (solver->check_assumptions_contradict_foced_assignment()) {
        if (solver->conf.verbosity) {
            cout
            << "c [breakid] forced assignements contradicted by assumptions, cannot run"
            << endl;
        }
        return false;
    }

    if (!check_limits()) {
        return false;
    }

    solver->clauseCleaner->remove_and_clean_all();
    solver->subsumeImplicit->subsume_implicit(false, "-breakid");

    CompleteDetachReatacher reattacher(solver);
    reattacher.detach_nonbins_nontris();

    if (!remove_duplicates()) {
        return false;
    }

    double myTime = cpuTime();
    assert(breakid == NULL);
    breakid = new BID::BreakID;
    breakid->set_verbosity(0);
    breakid->set_useMatrixDetection(solver->conf.breakid_matrix_detect);
    breakid->set_symBreakingFormLength(solver->conf.breakid_max_constr_per_permut);
    breakid->start_dynamic_cnf(solver->nVars());
    if (solver->conf.verbosity) {
        cout << "c [breakid] version " << breakid->get_sha1_version() << endl;
    }

    if (!add_clauses()) {
        delete breakid;
        breakid = NULL;

        bool ok = reattacher.reattachLongs();
        assert(ok);
        return false;
    }
    set_up_time_lim();

    // Detect symmetries, detect subgroups
    breakid->end_dynamic_cnf();
    if (solver->conf.verbosity) {
        cout << "c [breakid] Generators: " << breakid->get_num_generators() << endl;
    }
    if (solver->conf.verbosity > 3) {
        breakid->print_generators(std::cout);
    }

    if (solver->conf.verbosity >= 2) {
        cout << "c [breakid] Detecting subgroups..." << endl;
    }
    breakid->detect_subgroups();

    if (solver->conf.verbosity > 3) {
        breakid->print_subgroups(cout);
    }

    // Break symmetries
    breakid->break_symm();

    //reattach clauses
    bool ok = reattacher.reattachLongs();
    assert(ok);

    if (breakid->get_num_break_cls() != 0) {
        break_symms_in_cms();
    }

    get_outer_permutations();


    // Finish up
    double time_used = cpuTime() - myTime;
    int64_t remain = breakid->get_steps_remain();
    bool time_out = remain <= 0;
    double time_remain = float_div(remain, set_time_lim);
    if (solver->conf.verbosity) {
        cout << "c [breakid] finished "
        << solver->conf.print_times(time_used, time_out, time_remain)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "breakid"
            , time_used
            , time_out
            , time_remain
        );
    }

    delete breakid;
    breakid = NULL;

    return false;
}

void BreakID::get_outer_permutations()
{
    vector<unordered_map<BID::BLit, BID::BLit>> perms_inter;
    breakid->get_perms(&perms_inter);
    for(const auto& p: perms_inter) {
        unordered_map<Lit, Lit> outer;
        for(const auto& mymap: p) {
            Lit from = Lit::toLit(mymap.first.toInt());
            Lit to = Lit::toLit(mymap.second.toInt());

            from = solver->map_inter_to_outer(from);
            to = solver->map_inter_to_outer(to);

            outer[from] = to;
        }
        perms_outer.push_back(outer);
    }
}

bool BreakID::check_limits()
{
    uint64_t tot_num_cls = solver->longIrredCls.size()+solver->binTri.irredBins;
    uint64_t tot_num_lits = solver->litStats.irredLits + solver->binTri.irredBins*2;
    if (solver->nVars() > solver->conf.breakid_vars_limit_K*1000ULL) {
        if (solver->conf.verbosity) {
            cout
            << "c [breakid] max var limit exceeded, not running."
            << " Num vars: " << print_value_kilo_mega(solver->nVars(), false)
            << endl;
        }
        return false;
    }

    if (tot_num_cls > solver->conf.breakid_cls_limit_K*1000ULL) {
        if (solver->conf.verbosity) {
            cout
            << "c [breakid] max clause limit exceeded, not running."
            << " Num clauses: " << print_value_kilo_mega(tot_num_cls, false)
            << endl;
        }
        return false;
    }
    if (tot_num_lits > solver->conf.breakid_lits_limit_K*1000ULL) {
        if (solver->conf.verbosity) {
            cout
            << "c [breakid] max literals limit exceeded, not running."
            << " Num lits: " << print_value_kilo_mega(tot_num_lits, false)
            << endl;
        }
        return false;
    }

    return true;
}

bool BreakID::remove_duplicates()
{
    double myTime = cpuTime();
    dedup_cls.clear();

    for(ClOffset offs: solver->longIrredCls) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
        assert(!cl->getRemoved());
        assert(!cl->red());
        std::sort(cl->begin(), cl->end());
        cl->stats.hash_val = hash_clause(cl->getData(), cl->size());
        dedup_cls.push_back(offs);
    }

    std::sort(dedup_cls.begin(), dedup_cls.end(), EqCls(solver->cl_alloc));

    size_t old_size = dedup_cls.size();
    if (dedup_cls.size() > 1 && true) {
        vector<ClOffset>::iterator prev = dedup_cls.begin();
        vector<ClOffset>::iterator i = dedup_cls.begin();
        i++;
        Clause* prevcl = solver->cl_alloc.ptr(*prev);
        for(vector<ClOffset>::iterator end = dedup_cls.end(); i != end; i++) {
            Clause* cl = solver->cl_alloc.ptr(*i);
            if (!equiv(cl, prevcl)) {
                prev++;
                *prev = *i;
                prevcl = cl;
            }
        }
        prev++;
        dedup_cls.resize(prev-dedup_cls.begin());
    }

    double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity >= 1) {
        cout << "c [breakid] tmp-rem-dup cls"
        << " dupl: " << print_value_kilo_mega(old_size-dedup_cls.size(), false)
        << solver->conf.print_times(time_used)
        <<  endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "breakid-rem-dup"
            , time_used
        );
    }

    return true;
}

void BreakID::break_symms_in_cms()
{
    if (solver->conf.verbosity) {
        cout << "c [breakid] Breaking cls: "<< breakid->get_num_break_cls() << endl;
        cout << "c [breakid] Aux vars: "<< breakid->get_num_aux_vars() << endl;
    }
    for(uint32_t i = 0; i < breakid->get_num_aux_vars(); i++) {
        solver->new_var(true);
    }
    if (solver->conf.breakid_use_assump) {
        if (symm_var == var_Undef) {
            solver->new_var(true);
            symm_var = solver->nVars()-1;
            solver->add_assumption(Lit(symm_var, true));
        }
        assert(solver->varData[symm_var].removed == Removed::none);
    }

    auto brk = breakid->get_brk_cls();
    for (const auto& cl: brk) {
        vector<Lit>* cl2 = (vector<Lit>*)&cl;
        if (solver->conf.breakid_use_assump) {
            cl2->push_back(Lit(symm_var, false));
        }
        for(const Lit& l: *cl2) {
            assert(l.var() < solver->nVars());
            if (solver->conf.breakid_use_assump) {
                assert(solver->value(l) == l_Undef);
            }
        }
        Clause* newcl = solver->add_clause_int(*cl2
            , false //redundant
            , ClauseStats() //stats
            , true //attach
            , NULL //return simplified
            , false //DRAT... oops does not work right now
            , lit_Undef
        );
        if (newcl != NULL) {
            ClOffset offset = solver->cl_alloc.get_offset(newcl);
            solver->longIrredCls.push_back(offset);
        }
    }
}

void BreakID::finished_solving()
{
    //Nothing actually
}

void BreakID::start_new_solving()
{
    assert(solver->decisionLevel() == 0);
    assert(solver->okay());
    if (symm_var == var_Undef) {
        return;
    }

    assert(solver->varData[symm_var].removed == Removed::none);
    assert(solver->value(symm_var) != l_False
        && "The symm var can never be foreced to FALSE, logic error");

    //In certain conditions, in particular when the problem is UNSAT
    //the symmetry assumption var can be forced to TRUE at level 0
    if (solver->value(symm_var) == l_True) {
        symm_var = var_Undef;
        return;
    }

    assert(solver->value(symm_var) == l_Undef);
    solver->enqueue(Lit(symm_var, false));
    PropBy ret = solver->propagate<false>();
    assert(ret == PropBy() && "Must not fail on resetting symmetry var");
    symm_var = var_Undef;
}


void BreakID::update_var_after_varreplace()
{
    if (symm_var != var_Undef) {
        symm_var = solver->varReplacer->get_var_replaced_with(symm_var);
    }
}
