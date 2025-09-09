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

#include "constants.h"
#include "cryptominisat.h"
#include "solver.h"
#include "frat.h"
#include "shareddata.h"
#include "solvertypesmini.h"

#include <fstream>
#include <cstdint>
#include <iomanip>
#include <thread>
#include <mutex>
#include <atomic>
#include <cassert>
using std::thread;
using std::vector;

#define CACHE_SIZE 10ULL*1000ULL*1000UL
#ifndef LIMITMEM
#define MAX_VARS (1ULL<<28)
#else
#define MAX_VARS 3000
#endif

using namespace CMSat;

using std::complex;

static bool print_thread_start_and_finish = false;

namespace CMSat {
    struct CMSatPrivateData {
        explicit CMSatPrivateData(std::atomic<bool>* _must_interrupt)
        {
            must_interrupt = _must_interrupt;
            if (must_interrupt == nullptr) {
                must_interrupt = new std::atomic<bool>(false);
                must_interrupt_needs_delete = true;
            }
        }
        ~CMSatPrivateData()
        {
            for(Solver* this_s: solvers) {
                delete this_s;
            }
            if (must_interrupt_needs_delete) {
                delete must_interrupt;
            }

            delete log; //this will also close the file
            delete shared_data;
        }
        CMSatPrivateData(const CMSatPrivateData&) = delete;
        CMSatPrivateData& operator=(const CMSatPrivateData&) = delete;

        //Mult-threaded data
        vector<Solver*> solvers;
        SharedData *shared_data = nullptr;
        int which_solved = 0;
        std::atomic<bool>* must_interrupt;
        bool must_interrupt_needs_delete = false;

        bool okay = true;

        ///????
        std::ofstream* log = nullptr;
        int sql = 0;
        double timeout = numeric_limits<double>::max();
        bool interrupted = false;

        //variables and clauses added/to add.
        //This is to make it faster to add variables/clasues
        //   by adding them in one go
        unsigned cls = 0;
        unsigned vars_to_add = 0;
        unsigned total_num_vars = 0;
        vector<Lit> cls_lits;

        //For single call setup
        uint32_t num_solve_simplify_calls = 0;
        bool promised_single_call = false;

        //stats
        uint64_t previous_sum_conflicts = 0;
        uint64_t previous_sum_propagations = 0;
        uint64_t previous_sum_decisions = 0;
        vector<double> cpu_times;
    };
}

struct DataForThread
{
    explicit DataForThread(CMSatPrivateData* data, const vector<Lit>* _assumptions = nullptr) :
        solvers(data->solvers)
        , cpu_times(data->cpu_times)
        , lits_to_add(&(data->cls_lits))
        , vars_to_add(data->vars_to_add)
        , assumptions(_assumptions)
        , update_mutex(new std::mutex)
        , which_solved(&(data->which_solved))
        , ret(new lbool(l_Undef))
    {
    }

    ~DataForThread()
    {
        delete update_mutex;
        delete ret;
    }
    vector<Solver*>& solvers;
    vector<double>& cpu_times;
    vector<Lit> *lits_to_add;
    uint32_t vars_to_add;
    const vector<Lit> *assumptions;
    std::mutex* update_mutex;
    int *which_solved;
    lbool* ret;
};

DLL_PUBLIC SATSolver::SATSolver(
    void* config
    , std::atomic<bool>* interrupt_asap
    )
{
    data = new CMSatPrivateData(interrupt_asap);

    if (config && ((SolverConf*) config)->verbosity) {
        //NOT SAFE
        //yes -- this system will use a lock, but the solver itself won't(!)
        //so things will get mangled and printed wrongly
        //print_thread_start_and_finish = true;
    }

    data->solvers.push_back(new Solver((SolverConf*) config, data->must_interrupt));
    data->cpu_times.push_back(0.0);
}

DLL_PUBLIC SATSolver::~SATSolver()
{
    delete data;
}

void update_config(SolverConf& conf, unsigned thread_num)
{
    //Don't accidentally reconfigure everything to a specific value!
    conf.origSeed += thread_num;
    conf.thread_num = thread_num;

    switch(thread_num % 23) {
        case 0: {
            //default setup
            break;
        }

        case 1: {
            //Minisat-like
            conf.branch_strategy_setup = "vsids";
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;
            conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;

            conf.inc_max_temp_lev2_red_cls = 1.02;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.5;
            break;
        }
        case 2: {
            conf.branch_strategy_setup = "vsids";
//             conf.polar_best_inv_every_n = 100;
            break;
        }
        case 3: {
            //Similar to CMS 2.9 except we look at learnt DB size insteead
            //of conflicts to see if we need to clean.
            conf.branch_strategy_setup = "vsids";
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.5;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0;
            conf.glue_put_lev0_if_below_or_eq = 0;
            conf.inc_max_temp_lev2_red_cls = 1.03;
            break;
        }
        case 4: {
            //Similar to CMS 5.0
            conf.branch_strategy_setup = "vsids";
            conf.varElimRatioPerIter = 0.4;
            conf.every_lev1_reduce = 0;
            conf.every_lev2_reduce = 0;
            conf.do_bva = false;
            conf.max_temp_lev2_learnt_clauses = 30000;
            conf.glue_put_lev0_if_below_or_eq = 4;

            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.5;
            break;
        }
        case 5: {
            conf.branch_strategy_setup = "vsids";
            conf.never_stop_search = true;
            break;
        }
        case 6: {
            //Maple with backtrack
            conf.branch_strategy_setup = "vsids";
//             conf.polar_stable_every_n = 10000;
            break;
        }
        case 7: {
            conf.branch_strategy_setup = "vsids";
            conf.do_bva = false;
            conf.glue_put_lev0_if_below_or_eq = 2;
            conf.varElimRatioPerIter = 1;
            conf.inc_max_temp_lev2_red_cls = 1.04;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.1;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.3;
            break;
        }
        case 8: {
            //Different glue limit
            conf.branch_strategy_setup = "vmtf";
            conf.glue_put_lev0_if_below_or_eq = 2;
            conf.glue_put_lev1_if_below_or_eq = 2;
            break;
        }
        case 9: {
            conf.branch_strategy_setup = "vsids";
//             conf.polar_stable_every_n = 1;
            break;
        }
        case 10: {
            conf.branch_strategy_setup = "vsids";
            conf.polarity_mode = CMSat::PolarityMode::polarmode_pos;
//             conf.polar_stable_every_n = 100000;
            break;
        }
        case 11: {
            conf.branch_strategy_setup = "vsids";
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;

            conf.inc_max_temp_lev2_red_cls = 1.01;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.3;
            break;
        }
        case 12: {
            conf.branch_strategy_setup = "vmtf";
            conf.inc_max_temp_lev2_red_cls = 1.001;
//             conf.polar_stable_every_n = 7;
//             conf.polar_best_inv_every_n = 6;
            break;
        }

        case 13: {
            //Minisat-like
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;
            conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;

            conf.inc_max_temp_lev2_red_cls = 1.02;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.5;
            break;
        }
        case 14: {
            //Different glue limit
            conf.branch_strategy_setup = "vsids";
            conf.do_bva = false;
            conf.doMinimRedMoreMore = 1;
            conf.glue_put_lev0_if_below_or_eq = 4;
            //conf.glue_put_lev2_if_below_or_eq = 8;
            conf.max_num_lits_more_more_red_min = 3;
            conf.max_glue_more_minim = 4;
            break;
        }
        case 15: {
            //Similar to CMS 2.9 except we look at learnt DB size insteead
            //of conflicts to see if we need to clean.
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.5;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0;
            conf.glue_put_lev0_if_below_or_eq = 0;
            conf.inc_max_temp_lev2_red_cls = 1.03;
//             conf.polar_stable_every_n = 2;
            break;
        }
        case 16: {
            //Similar to CMS 5.0
            conf.varElimRatioPerIter = 0.4;
            conf.every_lev1_reduce = 0;
            conf.every_lev2_reduce = 0;
            conf.max_temp_lev2_learnt_clauses = 30000;
            conf.glue_put_lev0_if_below_or_eq = 4;

            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.5;
            break;
        }
        case 17: {
            //conf.max_temporary_learnt_clauses = 10000;
            conf.do_bva = true;
            break;
        }
        case 18: {
            conf.branch_strategy_setup = "vsids";
            conf.every_lev1_reduce = 0;
            conf.every_lev2_reduce = 0;
            conf.glue_put_lev1_if_below_or_eq = 0;
            conf.max_temp_lev2_learnt_clauses = 10000;
            break;
        }

        case 19: {
            conf.do_bva = false;
            conf.doMinimRedMoreMore = 0;
            conf.orig_global_timeout_multiplier = 5;
            conf.num_conflicts_of_search_inc = 1.15;
            conf.more_red_minim_limit_binary = 600;
            conf.max_num_lits_more_more_red_min = 20;
//             conf.polar_stable_every_n = 4;
            //conf.max_temporary_learnt_clauses = 10000;
            break;
        }

        case 20: {
            //Luby
            conf.branch_strategy_setup = "vmtf";
            conf.restart_inc = 1.5;
            conf.restart_first = 100;
            conf.restartType = Restart::luby;
//             conf.polar_stable_every_n = 2;
            break;
        }

        case 21: {
            conf.branch_strategy_setup = "vsids";
            conf.glue_put_lev0_if_below_or_eq = 3;
            conf.glue_put_lev1_if_below_or_eq = 5;
            break;
        }

        case 22: {
            conf.branch_strategy_setup = "vmtf";
            conf.doMinimRedMoreMore = 0;
            conf.orig_global_timeout_multiplier = 5;
            conf.num_conflicts_of_search_inc = 1.15;
            conf.more_red_minim_limit_binary = 600;
            conf.max_num_lits_more_more_red_min = 20;
            //conf.max_temporary_learnt_clauses = 10000;
            break;
        }

        default: {
            conf.varElimRatioPerIter = 0.1*(thread_num % 9);
            if (thread_num % 4 == 0) {
                conf.restartType = Restart::glue;
            }
            if (thread_num % 5 == 0) {
                conf.restartType = Restart::geom;
            }
            conf.restart_first = 100 * (0.5*(thread_num % 5));
            conf.doMinimRedMoreMore = ((thread_num % 5) == 1);
            break;
        }
    }
}

DLL_PUBLIC void SATSolver::set_num_threads(unsigned num)
{
    if (num <= 0) {
        const char err[] = "ERROR: Number of threads must be at least 1";
        std::cerr << err << endl;
        throw std::runtime_error(err);
    }
    if (num == 1) {
        return;
    }
    if (data->solvers.size() > 1) {
        const char err[] = "ERROR: You must call set_num_threads() at most once";
        std::cerr << err << endl;
        throw std::runtime_error(err);
    }

    if (data->solvers[0]->frat->enabled()) {
        const char err[] = "ERROR: FRAT cannot be used in multi-threaded mode";
        std::cerr << err << endl;
        throw std::runtime_error(err);
    }

    if (data->cls > 0 || nVars() > 0) {
        const char err[] = "ERROR: You must first call set_num_threads() and only then add clauses and variables";
        std::cerr << err << endl;
        throw std::runtime_error(err);
    }

    #ifdef USE_BREAKID
    if (num > 1) {
        cout << "ERROR: BreakID cannot work with multiple threads. Something is off in the memory allocation of the library that's likely 'static'. Perhaps in 'bliss'. Exiting." << endl;
        exit(-1);
    }
    #endif

    data->cls_lits.reserve(CACHE_SIZE);
    for(unsigned i = 1; i < num; i++) {
        SolverConf conf = data->solvers[0]->getConf();
        update_config(conf, i);
        data->solvers.push_back(new Solver(&conf, data->must_interrupt));
        data->cpu_times.push_back(0.0);
    }

    //set shared data
    data->shared_data = new SharedData(data->solvers.size());
    for(unsigned i = 0; i < num; i++) {
        SolverConf conf = data->solvers[i]->getConf();
        if (i >= 1) {
            conf.verbosity = 0;
            conf.doFindXors = 0;
        }
        data->solvers[i]->setConf(conf);
        data->solvers[i]->set_shared_data((SharedData*)data->shared_data);
    }
}

struct OneThreadAddCls
{
    OneThreadAddCls(DataForThread& _data_for_thread, size_t _tid) :
        data_for_thread(_data_for_thread)
        , tid(_tid)
    { }

    void operator()() {
        Solver& solver = *data_for_thread.solvers[tid];
        solver.new_external_vars(data_for_thread.vars_to_add);

        vector<Lit> lits;
        bool ret = true;
        size_t at = 0;
        const vector<Lit>& orig_lits = (*data_for_thread.lits_to_add);
        const size_t size = orig_lits.size();
        while(at < size && ret) {
            if (orig_lits[at] == lit_Undef) {
                lits.clear();
                at++;
                for(; at < size
                    && orig_lits[at] != lit_Undef
                    && orig_lits[at] != lit_Error
                    ; at++
                ) {
                    lits.push_back(orig_lits[at]);
                }
                ret = solver.add_clause_outside(lits);
            } else {
                lits.clear();
                at++;
                bool rhs = orig_lits[at].sign();
                at++;
                for(; at < size
                    && orig_lits[at] != lit_Undef
                    && orig_lits[at] != lit_Error
                    ; at++
                ) {
                    lits.push_back(orig_lits[at]);
                }
                ret = solver.add_xor_clause_outside(lits, rhs);
            }
        }

        if (!ret) {
            data_for_thread.update_mutex->lock();
            *data_for_thread.ret = l_False;
            data_for_thread.update_mutex->unlock();
        }
    }

    DataForThread& data_for_thread;
    const size_t tid;
};

//Add the cached clauses and variables to the threads
static bool actually_add_clauses_to_threads(CMSatPrivateData* data)
{
    DataForThread data_for_thread(data);
    if (data->solvers.size() == 1) {
        OneThreadAddCls t(data_for_thread, 0);
        t.operator()();
    } else {
        vector<thread> thds;
        for(size_t i = 0; i < data->solvers.size(); i++) {
            thds.push_back(thread(OneThreadAddCls(data_for_thread, i)));
        }
        for(std::thread& t: thds){
            t.join();
        }
    }
    bool ret = (*data_for_thread.ret != l_False);

    //clear what has been added
    data->cls_lits.clear();
    data->vars_to_add = 0;

    return ret;
}

DLL_PUBLIC void SATSolver::set_max_time(double max_time)
{
  assert(max_time >= 0 && "Cannot set negative limit on running time");

  const auto target_time = cpuTime() + max_time;
  for (Solver* s : data->solvers) {
    s->conf.maxTime = target_time;
  }
}

DLL_PUBLIC void SATSolver::set_max_confl(uint64_t max_confl)
{
  for (Solver* s : data->solvers) {
      s->set_max_confl(max_confl);
  }
}

DLL_PUBLIC void SATSolver::set_do_subs_with_resolvent_clauses(int subs) {
  for (Solver* s : data->solvers) {
      s->conf.do_subs_with_resolvent_clauses = subs;
  }
}

DLL_PUBLIC void SATSolver::set_default_polarity(bool polarity)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.polarity_mode = polarity ? PolarityMode::polarmode_pos : PolarityMode::polarmode_neg;
    }
}

DLL_PUBLIC void SATSolver::set_no_simplify()
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.doRenumberVars = false;
        s.conf.simplify_at_startup = false;
        s.conf.simplify_at_every_startup = false;
        s.conf.full_simplify_at_startup = false;
        s.conf.perform_occur_based_simp = false;
        s.conf.do_simplify_problem = false;
    }
}

DLL_PUBLIC void SATSolver::set_allow_otf_gauss()
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        //s.conf.reconfigure_at = 0;
        //s.conf.reconfigure_val = 15;
        s.conf.doFindXors = true;
        s.conf.gaussconf.max_num_matrices = 10;
        s.conf.gaussconf.max_matrix_columns = 10000000;
        s.conf.gaussconf.max_matrix_rows = 10000;
        s.conf.gaussconf.autodisable = false;
        s.conf.allow_elim_xor_vars = true;
    }
}

DLL_PUBLIC void SATSolver::set_no_simplify_at_startup()
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.simplify_at_startup = false;
    }
}

DLL_PUBLIC void SATSolver::set_no_equivalent_lit_replacement()
{
    set_scc(0);
}

DLL_PUBLIC void SATSolver::set_no_bva()
{
}

DLL_PUBLIC void SATSolver::set_simplify(const bool simp)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.do_simplify_problem = simp;
    }
}

DLL_PUBLIC void SATSolver::set_gates(const bool gates)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.doGateFind = gates;
    }
}

DLL_PUBLIC void SATSolver::set_picosat_gate_limitK(const uint32_t lim)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.picosat_gate_limitK = lim;
    }
}
DLL_PUBLIC void SATSolver::set_picosat_confl_limit(const uint32_t lim)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.picosat_confl_limit = lim;
    }
}

DLL_PUBLIC void SATSolver::set_weaken_time_limitM(const uint32_t lim)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.weaken_time_limitM = lim;
    }
}

DLL_PUBLIC void SATSolver::set_occ_based_lit_rem_time_limitM(const uint32_t lim)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.occ_based_lit_rem_time_limitM = lim;
    }
}

DLL_PUBLIC double SATSolver::get_orig_global_timeout_multiplier()
{
    return data->solvers[0]->conf.orig_global_timeout_multiplier;
}


DLL_PUBLIC void SATSolver::set_orig_global_timeout_multiplier(const double mult)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.orig_global_timeout_multiplier = mult;
    }
}


DLL_PUBLIC void SATSolver::set_no_bve()
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.doVarElim = false;
    }
}

DLL_PUBLIC void SATSolver::set_bve(int bve)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.doVarElim = bve;
    }
}


DLL_PUBLIC void SATSolver::set_bve_too_large_resolvent(int too_large_resolvent)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.velim_resolvent_too_large = too_large_resolvent;
    }
}

DLL_PUBLIC void SATSolver::set_greedy_undef()
{
    assert(false && "ERROR: Unfortunately, greedy undef is broken, please don't use it");
    std::cerr << "ERROR: Unfortunately, greedy undef is broken, please don't use it" << endl;
    exit(-1);
}

DLL_PUBLIC void SATSolver::set_verbosity(unsigned verbosity)
{
    if (data->solvers.empty()) return;
    Solver& s = *data->solvers[0];
    s.conf.verbosity = verbosity;
}

DLL_PUBLIC void SATSolver::set_timeout_all_calls(double timeout)
{
    data->timeout = timeout;
}

DLL_PUBLIC bool SATSolver::add_red_clause(const vector< Lit >& lits) {
    // TODO: use the bulk-adding maybe... but lit_Undef is used for clauses
    //       and lit_Error for XOR clauses, so... it's a bit crowded
    if (data->log) { (*data->log) << "c red " << lits << " 0" << endl; }
    bool ret = actually_add_clauses_to_threads(data);

    if (!ret) return false;
    for(auto& s: data->solvers) {
        ret &= s->add_clause_outside(lits, true);
    }
    return ret;
}

DLL_PUBLIC bool SATSolver::add_clause(const vector< Lit >& lits)
{
    if (data->log) { (*data->log) << lits << " 0" << endl; }

    bool ret = true;
    if (data->solvers.size() > 1) {
        if (data->cls_lits.size() + lits.size() + 1 > CACHE_SIZE) {
            ret = actually_add_clauses_to_threads(data);
        }

        data->cls_lits.push_back(lit_Undef);
        for(Lit lit: lits) {
            data->cls_lits.push_back(lit);
        }
    } else {
        data->solvers[0]->new_vars(data->vars_to_add);
        data->vars_to_add = 0;

        ret = data->solvers[0]->add_clause_outside(lits);
        data->cls++;
    }

    return ret;
}

void add_xor_clause_to_log(const std::vector<unsigned>& vars, bool rhs, std::ofstream* file)
{
    if (vars.empty()) {
        if (rhs) (*file) << "0" << endl;
    } else {
        if (!rhs) (*file) << "-";
        for(unsigned var: vars) (*file) << (var+1) << " ";
        (*file) << " 0" << endl;
    }
}

DLL_PUBLIC bool SATSolver::add_xor_clause(const std::vector<Lit>& lits, bool rhs)
{
    if (data->log) { (*data->log) << "x" << lits << " 0" << endl; }

    bool ret = true;
    if (data->solvers.size() > 1) {
        if (data->cls_lits.size() + lits.size() + 1 > CACHE_SIZE) {
            ret = actually_add_clauses_to_threads(data);
        }

        data->cls_lits.push_back(lit_Error);
        data->cls_lits.push_back(Lit(0, rhs));
        for(const auto& l: lits) data->cls_lits.push_back(l);
    } else {
        data->solvers[0]->new_vars(data->vars_to_add);
        data->vars_to_add = 0;

        ret = data->solvers[0]->add_xor_clause_outside(lits, rhs);
        data->cls++;
    }

    return ret;
}

DLL_PUBLIC bool SATSolver::add_xor_clause(const std::vector<unsigned>& vars, bool rhs)
{
    if (data->log) add_xor_clause_to_log(vars, rhs, data->log);

    bool ret = true;
    if (data->solvers.size() > 1) {
        if (data->cls_lits.size() + vars.size() + 1 > CACHE_SIZE) {
            ret = actually_add_clauses_to_threads(data);
        }

        data->cls_lits.push_back(lit_Error);
        data->cls_lits.push_back(Lit(0, rhs));
        for(uint32_t var: vars) {
            data->cls_lits.push_back(Lit(var, false));
        }
    } else {
        data->solvers[0]->new_vars(data->vars_to_add);
        data->vars_to_add = 0;

        ret = data->solvers[0]->add_xor_clause_outside(vars, rhs);
        data->cls++;
    }

    return ret;
}

DLL_PUBLIC bool SATSolver::add_bnn_clause(
    const std::vector<Lit>& lits,
    signed cutoff,
    Lit out)
{
    if (data->log) {
       assert(false && "No logs for BNN yet");
    }

    //lit_Undef is == TRUE, but lit_Error is not accepted
    assert(out != lit_Error);

    bool ret = true;
    if (data->solvers.size() > 1) {
        assert(false && "No multithreading for BNN yet");
    } else {
        data->solvers[0]->new_vars(data->vars_to_add);
        data->vars_to_add = 0;

        ret = data->solvers[0]->add_bnn_clause_outside(
            lits,
            cutoff,
            out);
        data->cls++;
    }

    return ret;
}

enum class Todo {todo_solve, todo_simplify};

struct OneThreadCalc
{
    OneThreadCalc(
        DataForThread& _data_for_thread,
        size_t _tid,
        Todo _todo,
        bool _only_sampling_solution
    ) :
        data_for_thread(_data_for_thread)
        , tid(_tid)
        , todo(_todo)
        , only_sampling_solution(_only_sampling_solution)
    {
        assert(data_for_thread.cpu_times.size() > tid);
        assert(data_for_thread.solvers.size() > tid);
    }

    void operator()()
    {
        if (print_thread_start_and_finish) {
            start_time = cpuTime();
            //data_for_thread.update_mutex->lock();
            //cout << "c Starting thread " << tid << endl;
            //data_for_thread.update_mutex->unlock();
        }

        //Add clauses and variables
        OneThreadAddCls cls_adder(data_for_thread, tid);
        cls_adder();

        //Solve or simplify
        lbool ret;
        if (todo == Todo::todo_solve) {
            ret = data_for_thread.solvers[tid]->solve_with_assumptions(data_for_thread.assumptions, only_sampling_solution);
        } else if (todo == Todo::todo_simplify) {
            ret = data_for_thread.solvers[tid]->simplify_with_assumptions(data_for_thread.assumptions);
        } else {
            assert(false);
        }

        assert(data_for_thread.cpu_times.size() > tid);
        data_for_thread.cpu_times[tid] = cpuTime();
        if (print_thread_start_and_finish) {
            data_for_thread.update_mutex->lock();
            std::ios::fmtflags f(cout.flags());
            cout << "c Finished thread " << tid << " with result: " << ret
            << " T-diff: " << std::fixed << std::setprecision(2)
            << (data_for_thread.cpu_times[tid]-start_time)
            << endl;
            cout.flags(f);
            data_for_thread.update_mutex->unlock();
        }


        if (ret != l_Undef) {
            data_for_thread.update_mutex->lock();
            *data_for_thread.which_solved = tid;
            *data_for_thread.ret = ret;
            //will interrupt all of them
            data_for_thread.solvers[0]->set_must_interrupt_asap();
            data_for_thread.update_mutex->unlock();
        }
    }

    DataForThread& data_for_thread;
    const size_t tid;
    double start_time;
    Todo todo;
    bool only_sampling_solution;
};

lbool calc(
    const vector< Lit >* assumptions,
    Todo todo,
    CMSatPrivateData *data,
    bool only_sampling_solution = false,
    const string* strategy = nullptr
) {
    //Sanity check
    if (data->solvers.size() > 1 && data->sql > 0) {
        std::cerr
        << "Multithreaded solving and SQL cannot be specified at the same time"
        << endl;
        exit(-1);
    }

    //Reset the interrupt signal if it was set
    data->must_interrupt->store(false, std::memory_order_relaxed);

    //Set timeout information
    if (data->timeout != numeric_limits<double>::max()) {
        for (size_t i = 0; i < data->solvers.size(); ++i) {
            Solver& s = *data->solvers[i];
            s.conf.maxTime = cpuTime() + data->timeout;
        }
    }

    if (data->log) {
        (*data->log) << "c Solver::";
        if (todo == Todo::todo_solve) {
            (*data->log) << "solve";
        } else if (todo == Todo::todo_simplify) {
            (*data->log) << "simplify";
        } else {
            assert(false);
        }
        (*data->log) << "( ";
        if (assumptions) {
            (*data->log) << *assumptions;
        }
        (*data->log) << " )" << endl;
    }

    //Deal with the single-thread case
    if (data->solvers.size() == 1) {
        data->solvers[0]->new_vars(data->vars_to_add);
        data->vars_to_add = 0;

        lbool ret ;
        if (todo == Todo::todo_solve) {
            ret = data->solvers[0]->solve_with_assumptions(assumptions, only_sampling_solution);
        } else if (todo == Todo::todo_simplify) {
            ret = data->solvers[0]->simplify_with_assumptions(assumptions, strategy);
        }
        data->okay = data->solvers[0]->okay();
        data->cpu_times[0] = cpuTime();
        return ret;
    }

    //Multi-threaded case
    DataForThread data_for_thread(data, assumptions);
    vector<thread> thds;
    for(size_t i = 0 ; i < data->solvers.size() ; i++) {
        thds.push_back(thread(OneThreadCalc( data_for_thread, i, todo, only_sampling_solution)));
    }

    for(std::thread& t: thds){
        t.join();
    }
    lbool real_ret = *data_for_thread.ret;

    //This does it for all of them, there is only one must-interrupt
    data_for_thread.solvers[0]->unset_must_interrupt_asap();

    //clear what has been added
    data->cls_lits.clear();
    data->vars_to_add = 0;
    data->okay = data->solvers[*data_for_thread.which_solved]->okay();
    return real_ret;
}

DLL_PUBLIC lbool SATSolver::solve(const vector< Lit >* assumptions, bool only_sampling_solution)
{
    if (data->promised_single_call
        && data->num_solve_simplify_calls > 0
    ) {
        cout
        << "ERROR: You promised to only call solve/simplify() once"
        << "       by calling set_single_run(), but you violated it. Exiting."
        << endl;
        exit(-1);
    }
    data->num_solve_simplify_calls++;

    //set information data (props, confl, dec)
    data->previous_sum_conflicts = get_sum_conflicts();
    data->previous_sum_propagations = get_sum_propagations();
    data->previous_sum_decisions = get_sum_decisions();

    return calc(assumptions, Todo::todo_solve, data, only_sampling_solution);
}

DLL_PUBLIC lbool SATSolver::simplify(const vector< Lit >* assumptions, const string* strategy)
{
    if (data->promised_single_call
        && data->num_solve_simplify_calls > 0
    ) {
        cout
        << "ERROR: You promised to only call solve/simplify() once"
        << "       by calling set_single_run(), but you violated it. Exiting."
        << endl;
        exit(-1);
    }
    data->num_solve_simplify_calls++;

    //set information data (props, confl, dec)
    data->previous_sum_conflicts = get_sum_conflicts();
    data->previous_sum_propagations = get_sum_propagations();
    data->previous_sum_decisions = get_sum_decisions();

    return calc(assumptions, Todo::todo_simplify, data, false, strategy);
}

DLL_PUBLIC const vector< lbool >& SATSolver::get_model() const
{
    return data->solvers[data->which_solved]->get_model();
}

DLL_PUBLIC const std::vector<Lit>& SATSolver::get_conflict() const
{

    return data->solvers[data->which_solved]->get_final_conflict();
}

DLL_PUBLIC uint32_t SATSolver::nVars() const
{
    return data->solvers[0]->nVarsOuter() + data->vars_to_add;
}

DLL_PUBLIC void SATSolver::new_var()
{
    new_vars(1);
}

DLL_PUBLIC void SATSolver::new_vars(const size_t n)
{
    if (n >= MAX_VARS ||
        data->total_num_vars + n >= MAX_VARS)
    {
        throw CMSat::TooManyVarsError();
    }

    if (data->log) {
        (*data->log) << "c Solver::new_vars( " << n << " )" << endl;
    }

    data->vars_to_add += n;
    data->total_num_vars += n;
}

DLL_PUBLIC void SATSolver::add_sql_tag(const std::string& name, const std::string& val)
{
    for(Solver* solver: data->solvers) {
        solver->add_sql_tag(name, val);
    }
}

DLL_PUBLIC const char* SATSolver::get_version_sha1()
{
    return Solver::get_version_sha1();
}

DLL_PUBLIC const char* SATSolver::get_version()
{
    return Solver::get_version_tag();
}

DLL_PUBLIC const char* SATSolver::get_compilation_env()
{
    return Solver::get_compilation_env();
}

DLL_PUBLIC std::string SATSolver::get_thanks_info(const char* prefix)
{
    std::stringstream ss;
    ss << prefix << "Using VMTF, picosat, CaDiCaL, and CadiBack code by Armin Biere" << endl;
    #ifdef USE_BREAKID
    ss << prefix << "Using BreakID by Devriendt, Bogaerts, Bruynooghe and Denecker" << endl;
    ss << prefix << "Using Bliss graph automorphism library (under LGPL) by Tommi Junttila" << endl;
    ss << prefix << "CMS is GPL licensed due to Bliss being linked. Build without Bliss to get MIT version" << endl;
    #else
    ss << prefix << "CMS is MIT licensed" << endl;
    #endif
    ss << prefix << "Using code from 'When Boolean Satisfiability Meets Gauss-E. in a Simplex Way'" << endl;
    ss << prefix << "      by C.-S. Han and J.-H. Roland Jiang in CAV 2012. Fixes by M. Soos" << endl;
    ss << prefix << "Using CCAnr from 'CCAnr: A Conf. Checking Based Local Search Solver [...]'" << endl;
    ss << prefix << "      by Shaowei Cai, Chuan Luo, and Kaile Su, SAT 2015" << endl;
    ss << prefix << "Using Oracle code from 'Integrating Tree Decompositions [...]'" << endl;
    ss << prefix << "      by Tuukka Korhonen and Matti Jarvisalo, CP 2021";
    return ss.str();
}

DLL_PUBLIC void SATSolver::print_stats(double wallclock_time_started) const
{
    double cpu_time_total = cpuTimeTotal();

    double cpu_time;
    if (data->interrupted) {
        //cannot know, we can only print one, so we print the 1st
        cpu_time = data->cpu_times[0];
    } else {
        //We print the winning solver's thread time
        cpu_time = data->cpu_times[data->which_solved];
    }

    //If only one thread, then it's easy
    if (data->solvers.size() == 1) {
        cpu_time = cpu_time_total;
    }

    data->solvers[data->which_solved]->print_stats(cpu_time, cpu_time_total, wallclock_time_started);
}

DLL_PUBLIC void SATSolver::set_find_xors(bool do_find_xors)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.doFindXors = do_find_xors;
    }
}

DLL_PUBLIC void SATSolver::set_frat(FILE* os)
{
    if (data->solvers.size() > 1) {
        std::cerr << "ERROR: FRAT cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }
    if (nVars() > 0) {
        std::cerr << "ERROR: FRAT cannot be set after variables have been added" << endl;
        exit(-1);
    }

    data->solvers[0]->conf.doBreakid = false;
    data->solvers[0]->add_frat(os);
    data->solvers[0]->conf.do_hyperbin_and_transred = true;
}

DLL_PUBLIC void SATSolver::set_idrup(FILE* os)
{
    if (data->solvers.size() > 1) {
        std::cerr << "ERROR: IDRUP cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }
    if (nVars() > 0) {
        std::cerr << "ERROR: IDRUP cannot be set after variables have been added" << endl;
        exit(-1);
    }
    data->solvers[0]->conf.doBreakid = false;
    data->solvers[0]->conf.doFindXors = false;
    data->solvers[0]->add_idrup(os);
    data->solvers[0]->conf.gaussconf.max_matrix_rows = 0;
    data->solvers[0]->conf.gaussconf.max_matrix_columns = 0;
    data->solvers[0]->conf.do_hyperbin_and_transred = true;
}

DLL_PUBLIC void SATSolver::interrupt_asap()
{
    data->must_interrupt->store(true, std::memory_order_relaxed);
}

void DLL_PUBLIC SATSolver::add_in_partial_solving_stats()
{
    data->solvers[data->which_solved]->add_in_partial_solving_stats();
    data->interrupted = true;
}

DLL_PUBLIC std::vector<Lit> SATSolver::get_zero_assigned_lits() const
{
    return data->solvers[data->which_solved]->get_zero_assigned_lits();
}

DLL_PUBLIC unsigned long SATSolver::get_sql_id() const
{
    return 0;
}

DLL_PUBLIC bool SATSolver::okay() const
{
    return data->okay && data->solvers[0]->okay();
}

DLL_PUBLIC void SATSolver::log_to_file(std::string filename)
{
    if (data->log) {
        std::cerr
        << "ERROR: A file has already been designated for logging!"
        << endl;
        exit(-1);
    }

    data->log = new std::ofstream();
    data->log->exceptions( std::ofstream::failbit | std::ofstream::badbit );
    data->log->open(filename.c_str(), std::ios::out);
    if (!data->log->is_open()) {
        std::cerr
        << "ERROR: Cannot open record file '" << filename << "'"
        << " for writing."
        << endl;
        exit(-1);
    }
}

DLL_PUBLIC std::vector<std::pair<Lit, Lit> > SATSolver::get_all_binary_xors() const
{
    return data->solvers[0]->get_all_binary_xors();
}

DLL_PUBLIC vector<std::pair<vector<uint32_t>, bool> >
SATSolver::get_recovered_xors(bool) const
{
    vector<std::pair<vector<uint32_t>, bool> > ret;
    Solver& s = *data->solvers[0];

    std::pair<vector<uint32_t>, bool> tmp;
    vector<Xor> xors = s.get_recovered_xors();
    for(const auto& x: xors) {
        tmp.first = x.get_vars();
        tmp.second = x.rhs;
        ret.push_back(tmp);
    }
    return ret;
}

DLL_PUBLIC void SATSolver::set_sqlite(std::string filename)
{
    if (data->solvers.size() > 1) {
        std::cerr
        << "Multithreaded solving and SQL cannot be specified at the same time"
        << endl;
        exit(-1);
    }
    data->sql = 1;
    data->solvers[0]->set_sqlite(filename);
}

DLL_PUBLIC uint64_t SATSolver::get_sum_conflicts()
{
    uint64_t conlf = 0;
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        conlf += s.sumConflicts;
    }
    return conlf;
}

DLL_PUBLIC uint64_t SATSolver::get_sum_conflicts() const
{
    uint64_t total_conflicts = 0;
    for (Solver const* s : data->solvers) {
        total_conflicts += s->sumConflicts;
    }
    return total_conflicts;
}

DLL_PUBLIC uint64_t SATSolver::get_sum_propagations()
{
    uint64_t props = 0;
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        props += s.sumPropStats.propagations;
    }
    return props;
}

DLL_PUBLIC uint64_t SATSolver::get_sum_propagations() const
{
    uint64_t total_propagations = 0;
    for (Solver const* s : data->solvers) {
        total_propagations += s->sumPropStats.propagations;
    }
    return total_propagations;
}

DLL_PUBLIC uint64_t SATSolver::get_sum_decisions()
{
    uint64_t dec = 0;
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        dec += s.sumSearchStats.decisions;
    }
    return dec;
}

DLL_PUBLIC uint64_t SATSolver::get_sum_decisions() const
{
    uint64_t total_decisions = 0;
    for (Solver const* s : data->solvers) {
        total_decisions += s->sumSearchStats.decisions;
    }
    return total_decisions;
}

DLL_PUBLIC uint64_t SATSolver::get_last_conflicts()
{
    return get_sum_conflicts() - data->previous_sum_conflicts;
}

DLL_PUBLIC uint64_t SATSolver::get_last_propagations()
{
    return get_sum_propagations() - data->previous_sum_propagations;
}

DLL_PUBLIC uint64_t SATSolver::get_last_decisions()
{
    return get_sum_decisions() - data->previous_sum_decisions;
}


void DLL_PUBLIC SATSolver::start_getting_constraints(bool red, bool simplified,
        uint32_t max_len, uint32_t max_glue) {
    actually_add_clauses_to_threads(data);
    assert(!data->solvers.empty());
    data->solvers[0]->start_getting_constraints(red, simplified, max_len, max_glue);
}

bool DLL_PUBLIC SATSolver::get_next_constraint(std::vector<Lit>& out, bool& is_xor, bool& rhs)
{
    assert(!data->solvers.empty());
    return data->solvers[0]->get_next_constraint(out, is_xor, rhs);
}

void DLL_PUBLIC SATSolver::end_getting_constraints()
{
    assert(!data->solvers.empty());
    data->solvers[0]->end_getting_constraints();
}

DLL_PUBLIC set<uint32_t> SATSolver::translate_sampl_set(
    const set<uint32_t>& sampl_set, bool also_removed)
{
    return data->solvers[0]->translate_sampl_set(sampl_set, also_removed);
}

DLL_PUBLIC std::vector<Lit> SATSolver::get_weight_translation() const
{
    return data->solvers[0]->get_weight_translation();
}

DLL_PUBLIC map<uint32_t, VarMap> SATSolver::update_var_mapping(
        const std::map<uint32_t, VarMap>& vmap)
{
    return data->solvers[0]->update_var_mapping(vmap);
}

DLL_PUBLIC vector<uint32_t> SATSolver::get_elimed_vars() const {
    return data->solvers[0]->get_elimed_vars();
}

DLL_PUBLIC std::vector<std::vector<Lit>> SATSolver::get_cls_defining_var(uint32_t outer_v) const {
    return data->solvers[0]->get_cls_defining_var(outer_v);
}

DLL_PUBLIC void SATSolver::reverse_bce() {
    return data->solvers[0]->reverse_bce();

}

void DLL_PUBLIC SATSolver::set_min_bva_gain(uint32_t min_bva_gain)
{
    for (auto & solver : data->solvers) {
        solver->conf.min_bva_gain = min_bva_gain;
    }
}

void DLL_PUBLIC SATSolver::set_lit_weight_internal(const Lit lit, const double val)
{
    for (auto & solver : data->solvers) {
        solver->set_outer_lit_weight(lit, val);
    }
}


void DLL_PUBLIC SATSolver::set_up_for_sample_counter(const uint32_t fixed_restart)
{
    for (auto & solver : data->solvers) {
        SolverConf conf = solver->getConf();
        conf.doSLS = false;
        conf.doBreakid = false;
        conf.restartType = Restart::fixed;
        conf.never_stop_search = true;
        conf.branch_strategy_setup = "rand";
        conf.simplify_at_startup = false;
        conf.doFindAndReplaceEqLits = false;
        conf.do_distill_clauses = false;
        conf.doFindXors = false;
        conf.fixed_restart_num_confl = fixed_restart;
        conf.polarity_mode = CMSat::PolarityMode::polarmode_weighted;

        solver->setConf(conf);
    }
}

void DLL_PUBLIC SATSolver::set_up_for_scalmc()
{
    for (auto & solver : data->solvers) {
        SolverConf conf = solver->getConf();
        conf.doBreakid = false;
        conf.gaussconf.max_matrix_columns = 10000000;
        conf.gaussconf.max_matrix_rows = 10000;
        conf.gaussconf.max_num_matrices = 2;
        conf.gaussconf.autodisable = false;
        conf.global_multiplier_multiplier_max = 1;
        conf.orig_global_timeout_multiplier = 1.5;
        conf.min_bva_gain = 1;
        conf.doSLS = false;
        conf.xor_finder_time_limitM = 400;
//         conf.polar_stable_every_n = 100000; //i.e. never
        uint32_t xor_cut = 4;
        assert(xor_cut >= 3);

        //Distill
        conf.distill_sort = 4;
        conf.distill_long_cls_time_limitM = 10ULL;
        conf.distill_red_tier0_ratio = 0.7;
        conf.distill_red_tier1_ratio = 0.07;

        conf.simplify_at_startup = 1;
        conf.varElimRatioPerIter = 1;
        /* conf.restartType = Restart::geom; */
//         conf.branch_strategy_setup = "vsids1";
        conf.bva_every_n = 1;
        conf.do_simplify_problem = true;
        conf.diff_declev_for_chrono = -1;
        /* conf.polarity_mode = CMSat::PolarityMode::polarmode_saved; */
        solver->setConf(conf);
    }
}

void DLL_PUBLIC SATSolver::set_up_for_arjun()
{
    for (size_t i = 0; i < data->solvers.size(); i++) {
        SolverConf conf = data->solvers[i]->getConf();
        conf.doBreakid = false;
        //conf.gaussconf.max_num_matrices = 0;
        //conf.xor_finder_time_limitM = 0;
        //conf.xor_detach_reattach = true;
        conf.global_multiplier_multiplier_max = 1;
        conf.orig_global_timeout_multiplier = 2.5;
        conf.do_bva = false;
//         conf.polar_stable_every_n = 100000; //i.e. never use stable polarities
        conf.do_hyperbin_and_transred = false;
        conf.doTransRed = false;

        //conf.do_simplify_problem = false; //no simplification without explicity calling it
//         conf.varElimRatioPerIter = 1;
        conf.restartType = Restart::geom;
        conf.polarity_mode = CMSat::PolarityMode::polarmode_best;
        conf.branch_strategy_setup = "vsids1";
        conf.diff_declev_for_chrono = -1;

        //Distill
        conf.distill_sort = 4;
        conf.distill_long_cls_time_limitM = 10ULL;
        conf.distill_red_tier0_ratio = 0.7;
        conf.distill_red_tier1_ratio = 0.07;

        data->solvers[i]->setConf(conf);
    }
}

DLL_PUBLIC uint32_t SATSolver::get_verbosity() const
{
   const SolverConf& conf = data->solvers[0]->getConf();
   return conf.verbosity;
}

DLL_PUBLIC void SATSolver::set_single_run()
{
    if (data->num_solve_simplify_calls > 0) {
        cout << "ERROR: You must call set_single_run() before solving" << endl;
        exit(-1);
    }
    data->promised_single_call = true;

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.breakid_use_assump = false;
    }
}

DLL_PUBLIC std::vector<uint32_t> SATSolver::get_lit_incidence()
{
    actually_add_clauses_to_threads(data);
    return data->solvers[data->which_solved]->get_outside_lit_incidence();
}

DLL_PUBLIC vector<uint32_t> SATSolver::get_var_incidence()
{
    actually_add_clauses_to_threads(data);
    return data->solvers[data->which_solved]->get_outside_var_incidence();
}

DLL_PUBLIC vector<OrGate> SATSolver::get_recovered_or_gates()
{
    actually_add_clauses_to_threads(data);
    return data->solvers[0]->get_recovered_or_gates();
}

DLL_PUBLIC vector<ITEGate> SATSolver::get_recovered_ite_gates()
{
    actually_add_clauses_to_threads(data);
    return data->solvers[0]->get_recovered_ite_gates();
}

DLL_PUBLIC void SATSolver::set_renumber(const bool renumber)
{
    for(auto& s: data->solvers) {
        s->conf.doRenumberVars = renumber;
    }
}

DLL_PUBLIC std::vector<uint32_t> SATSolver::remove_definable_by_irreg_gate(const vector<uint32_t>& vars)
{
    return data->solvers[0]->remove_definable_by_irreg_gate(vars);
}

DLL_PUBLIC std::vector<uint32_t> SATSolver::extend_definable_by_irreg_gate(const vector<uint32_t>& vars)
{
    return data->solvers[0]->extend_definable_by_irreg_gate(vars);
}

DLL_PUBLIC void SATSolver::clean_sampl_get_empties(
    std::vector<uint32_t>& sampl_vars, std::set<uint32_t>& empty_vars)
{
    return data->solvers[0]->clean_sampl_get_empties(sampl_vars, empty_vars);
}

DLL_PUBLIC lbool SATSolver::find_fast_backw(FastBackwData fast_backw)
{
    assert(data->solvers.size() == 1);
    data->solvers[0]->fast_backw = fast_backw;
    bool backup_doVarElim = data->solvers[0]->conf.doVarElim;
    data->solvers[0]->conf.doVarElim = true;
    const auto ret = solve(nullptr, true);
    data->solvers[0]->fast_backw = FastBackwData();
    data->solvers[0]->conf.doVarElim = backup_doVarElim;

    return ret;
}

DLL_PUBLIC void SATSolver::remove_and_clean_all()
{
    for(auto& s: data->solvers) {
        if (!s->okay()) return;
        s->remove_and_clean_all();
    }
}

DLL_PUBLIC vector<uint32_t> SATSolver::get_var_incidence_also_red()
{
    return data->solvers[data->which_solved]->get_outside_var_incidence_also_red();
}

DLL_PUBLIC void SATSolver::set_intree_probe(int val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.doIntreeProbe = val;
    }
}

DLL_PUBLIC void SATSolver::set_no_confl_needed()
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.conf_needed = false;
    }
}


DLL_PUBLIC bool SATSolver::implied_by(
    const std::vector<Lit>& lits,
    std::vector<Lit>& out_implied
)
{
    return data->solvers[data->which_solved]->implied_by(lits, out_implied);
}

DLL_PUBLIC void SATSolver::set_full_bve(int val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.do_full_varelim = val;
    }
}

DLL_PUBLIC void SATSolver::set_sls(int val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.doSLS = val;
    }
}

DLL_PUBLIC void SATSolver::reset_vsids()
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.reset_vsids();
    }
}

DLL_PUBLIC void SATSolver::set_scc(int val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.doFindAndReplaceEqLits = val;
    }
}

DLL_PUBLIC void SATSolver::set_distill(int val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.do_distill_clauses = val;
    }
}

DLL_PUBLIC void SATSolver::set_bva(int val)
{
    assert(val == 0 && "BVA no longer supported");
}

DLL_PUBLIC void SATSolver::set_polarity_mode(CMSat::PolarityMode mode)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.polarity_mode = mode;
    }
}

DLL_PUBLIC CMSat::PolarityMode SATSolver::get_polarity_mode() const
{
    return data->solvers[0]->conf.polarity_mode;
}

DLL_PUBLIC void SATSolver::set_full_bve_iter_ratio(double val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.varElimRatioPerIter = val;
    }
}

DLL_PUBLIC void SATSolver::set_prefix(const char* prefix)
{
    for (auto & solver : data->solvers) {
        Solver& s = *solver;
        s.conf.prefix = prefix;
    }
}

DLL_PUBLIC void SATSolver::set_max_red_linkin_size(uint32_t sz)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.maxRedLinkInSize = sz;
    }
}

DLL_PUBLIC lbool SATSolver::probe(Lit l, uint32_t& min_props)
{
    assert(data->solvers.size() >= 1);
    Solver& s = *data->solvers[0];
    return s.probe_outside(l, min_props);
}

DLL_PUBLIC uint32_t SATSolver::simplified_nvars()
{
    assert(data->solvers.size() >= 1);
    Solver& s = *data->solvers[0];
    return s.nVars();
}

DLL_PUBLIC void SATSolver::set_varelim_check_resolvent_subs(bool varelim_check_resolvent_subs)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.varelim_check_resolvent_subs = varelim_check_resolvent_subs;
    }
}

void into_rhs(vector<Lit>& lits, bool rhs) {
    assert(!(lits.empty() && rhs == false));
    if (!rhs) lits[0] ^= true;
}

DLL_PUBLIC void SATSolver::open_file_and_dump_irred_clauses(const char* fname)
{
    start_getting_constraints(false);
    uint32_t num_cls = 0;
    int32_t max_vars = -1;
    vector<Lit> lits; bool is_xor; bool rhs;
    while (true) {
        bool ret = get_next_constraint(lits, is_xor, rhs);
        if (!ret) break;
        num_cls++;
        for(const Lit l: lits) {
            if ((int32_t)l.var() > max_vars) max_vars = (int32_t)l.var();
        }
    }

    std::ofstream f(fname);
    f << "p cnf " << max_vars << " " << num_cls << endl;
    while (true) {
        bool ret = get_next_constraint(lits, is_xor, rhs);
        if (!ret) break;
        if (is_xor) {into_rhs(lits, rhs); f << "x " << lits << " 0\n";}
        else f << lits << " 0\n";
    }
}

DLL_PUBLIC void SATSolver::set_pred_short_size(int32_t sz)
{
    if (sz == -1) {
        //set to default
        SolverConf conf2;
        sz = conf2.pred_short_size;
    } else if (sz < 0) {
        cout << "ERROR: only 'sz' parameters accepted are -1 for resetting to default, and >=0" << endl;
        assert(false);
        exit(-1);
    }

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.pred_short_size = sz;
    }
}

DLL_PUBLIC void SATSolver::set_pred_long_size(int32_t sz)
{
    if (sz == -1) {
        //set to default
        SolverConf conf2;
        sz = conf2.pred_long_size;
    } else if (sz < 0) {
        cout << "ERROR: only 'sz' parameters accepted are -1 for resetting to default, and >=0" << endl;
        assert(false);
        exit(-1);
    }

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.pred_long_size = sz;
    }
}

DLL_PUBLIC void SATSolver::set_pred_forever_size(int32_t sz)
{
    if (sz == -1) {
        //set to default
        SolverConf conf2;
        sz = conf2.pred_forever_size;
    } else if (sz < 0) {
        cout << "ERROR: only 'sz' parameters accepted are -1 for resetting to default, and >=0" << endl;
        assert(false);
        exit(-1);
    }

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.pred_forever_size = sz;
    }
}

DLL_PUBLIC void SATSolver::set_pred_long_chunk(int32_t sz)
{
    if (sz == -1) {
        //set to default
        SolverConf conf2;
        sz = conf2.pred_long_chunk;
    } else if (sz < 0) {
        cout << "ERROR: only 'sz' parameters accepted are -1 for resetting to default, and >=0" << endl;
        assert(false);
        exit(-1);
    }

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.pred_long_chunk = sz;
    }
}

DLL_PUBLIC void SATSolver::set_pred_forever_chunk(int32_t sz)
{
    if (sz == -1) {
        //set to default
        SolverConf conf2;
        sz = conf2.pred_forever_chunk;
    } else if (sz < 0) {
        cout << "ERROR: only 'sz' parameters accepted are -1 for resetting to default, and >=0" << endl;
        assert(false);
        exit(-1);
    }

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.pred_forever_chunk = sz;
    }
}

DLL_PUBLIC void SATSolver::set_pred_forever_cutoff(int32_t sz)
{
    if (sz == -1) {
        //set to default
        SolverConf conf2;
        sz = conf2.pred_forever_cutoff;
    } else if (sz < 0) {
        cout << "ERROR: only 'sz' parameters accepted are -1 for resetting to default, and >=0" << endl;
        assert(false);
        exit(-1);
    }

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.pred_forever_cutoff = sz;
    }
}

DLL_PUBLIC void SATSolver::set_every_pred_reduce(int32_t sz)
{
        if (sz == -1) {
        //set to default
        SolverConf conf2;
        sz = conf2.every_pred_reduce;
    } else if (sz < 0) {
        cout << "ERROR: only 'sz' parameters accepted are -1 for resetting to default, and >=0" << endl;
        assert(false);
        exit(-1);
    }

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.every_pred_reduce = sz;
    }
}

DLL_PUBLIC void SATSolver::set_seed(const uint32_t seed)
{

    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.set_seed(seed);
    }
}

DLL_PUBLIC bool SATSolver::minimize_clause(std::vector<Lit>& cl)
{
    Solver& s = *data->solvers[0];
    actually_add_clauses_to_threads(data);
    return s.minimize_clause(cl);
}


DLL_PUBLIC std::vector<std::vector<uint8_t>> SATSolver::many_sls(int64_t mems, uint32_t num) {
    Solver& s = *data->solvers[0];
    actually_add_clauses_to_threads(data);
    return s.many_sls(mems, num);
}

DLL_PUBLIC bool SATSolver::backbone_simpl(int64_t max_confl, bool& finished)
{
    Solver& s = *data->solvers[0];
    actually_add_clauses_to_threads(data);
    return s.backbone_simpl(max_confl, true, finished);
}

DLL_PUBLIC bool SATSolver::removed_var(uint32_t var) const{
    Solver& s = *data->solvers[0];
    actually_add_clauses_to_threads(data);
    return s.removed_var_ext(var);
}


DLL_PUBLIC void SATSolver::set_oracle_get_learnts(bool val) {
    Solver& s = *data->solvers[0];
    s.conf.oracle_get_learnts = val;
}

DLL_PUBLIC void SATSolver::set_oracle_find_bins(int val) {
    Solver& s = *data->solvers[0];
    s.conf.oracle_find_bins = val;
}

DLL_PUBLIC void SATSolver::set_oracle_removed_is_learnt(bool val) {
    Solver& s = *data->solvers[0];
    s.conf.oracle_removed_is_learnt = val;
}

DLL_PUBLIC const std::vector<uint32_t>& SATSolver::get_sampl_vars() const {
    Solver& s = *data->solvers[0];
    if (!s.conf.sampling_vars_set) throw std::runtime_error("Sampling vars not set");
    return s.conf.sampling_vars;
}

DLL_PUBLIC void SATSolver::set_sampl_vars(const std::vector<uint32_t>& vars) {
    Solver& s = *data->solvers[0];
    if (s.conf.sampling_vars_set) throw std::runtime_error("Sampling vars already set");
    s.conf.sampling_vars_set = true;
    s.conf.sampling_vars = vars;
}

DLL_PUBLIC bool SATSolver::get_sampl_vars_set() const {
    Solver& s = *data->solvers[0];
    return s.conf.sampling_vars_set;
}

DLL_PUBLIC void SATSolver::set_opt_sampl_vars(const std::vector<uint32_t>& vars) {
    Solver& s = *data->solvers[0];
    if (s.conf.opt_sampling_vars_set) throw std::runtime_error("Opt sampling vars already set");
    s.conf.opt_sampling_vars_set = true;
    s.conf.opt_sampling_vars = vars;
}

DLL_PUBLIC const std::vector<uint32_t>& SATSolver::get_opt_sampl_vars() const {
    Solver& s = *data->solvers[0];
    if (!s.conf.opt_sampling_vars_set) throw std::runtime_error("Sampling vars not set");
    return s.conf.opt_sampling_vars;
}

DLL_PUBLIC bool SATSolver::get_opt_sampl_vars_set() const {
    Solver& s = *data->solvers[0];
    return s.conf.opt_sampling_vars_set;
}
