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

#include "constants.h"
#include "cryptominisat5/cryptominisat.h"
#include "solver.h"
#include "drat.h"
#include "shareddata.h"
#include <fstream>

#include <thread>
#include <mutex>
#include <atomic>
#include <cassert>
using std::thread;

#define CACHE_SIZE 10ULL*1000ULL*1000UL
#ifndef LIMITMEM
#define MAX_VARS (1ULL<<28)
#else
#define MAX_VARS 3000
#endif

using namespace CMSat;

static bool print_thread_start_and_finish = false;

namespace CMSat {
    struct CMSatPrivateData {
        explicit CMSatPrivateData(std::atomic<bool>* _must_interrupt)
        {
            must_interrupt = _must_interrupt;
            if (must_interrupt == NULL) {
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

        vector<Solver*> solvers;
        SharedData *shared_data = NULL;
        int which_solved = 0;
        std::atomic<bool>* must_interrupt;
        bool must_interrupt_needs_delete = false;
        bool okay = true;
        std::ofstream* log = NULL;
        int sql = 0;
        double timeout = std::numeric_limits<double>::max();
        bool interrupted = false;

        //variables and clauses added/to add
        unsigned cls = 0;
        unsigned vars_to_add = 0;
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
    explicit DataForThread(CMSatPrivateData* data, const vector<Lit>* _assumptions = NULL) :
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
    if (thread_num > 0) {
        conf.reconfigure_val = 0;
    }
    conf.origSeed += thread_num;
    conf.thread_num = thread_num;

    switch(thread_num % 23) {
        case 0: {
            //default setup
            break;
        }

        case 1: {
            //Minisat-like
            conf.branch_strategy_setup = "vsids1";
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;
            conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;

            conf.inc_max_temp_lev2_red_cls = 1.02;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.5;
            break;
        }
        case 2: {
            conf.branch_strategy_setup = "vsids1";
            break;
        }
        case 3: {
            //Similar to CMS 2.9 except we look at learnt DB size insteead
            //of conflicts to see if we need to clean.
            conf.branch_strategy_setup = "vsids1";
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.5;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0;
            conf.glue_put_lev0_if_below_or_eq = 0;
            conf.inc_max_temp_lev2_red_cls = 1.03;
            break;
        }
        case 4: {
            //Similar to CMS 5.0
            conf.branch_strategy_setup = "vsids1";
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
            conf.branch_strategy_setup = "vsids1";
            conf.never_stop_search = true;
            break;
        }
        case 6: {
            //Maple with backtrack
            conf.branch_strategy_setup = "vsids1";
            break;
        }
        case 7: {
            conf.branch_strategy_setup = "vsids1";
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
            conf.branch_strategy_setup = "vsids1";
            conf.glue_put_lev0_if_below_or_eq = 2;
            conf.glue_put_lev1_if_below_or_eq = 2;
            break;
        }
        case 9: {
            conf.branch_strategy_setup = "vsids1";
            break;
        }
        case 10: {
            conf.branch_strategy_setup = "vsids1";
            conf.polarity_mode = CMSat::PolarityMode::polarmode_pos;
            break;
        }
        case 11: {
            conf.branch_strategy_setup = "vsids1";
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;

            conf.inc_max_temp_lev2_red_cls = 1.01;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.3;
            break;
        }
        case 12: {
            conf.branch_strategy_setup = "vsids1";
            conf.inc_max_temp_lev2_red_cls = 1.001;
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
            conf.branch_strategy_setup = "vsids1";
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
            conf.branch_strategy_setup = "vsids1+vsids2";
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
            //conf.max_temporary_learnt_clauses = 10000;
            break;
        }

        case 20: {
            //Luby
            conf.branch_strategy_setup = "vsids1";
            conf.restart_inc = 1.5;
            conf.restart_first = 100;
            conf.restartType = Restart::luby;
            break;
        }

        case 21: {
            conf.branch_strategy_setup = "vsids2";
            conf.glue_put_lev0_if_below_or_eq = 3;
            conf.glue_put_lev1_if_below_or_eq = 5;
            break;
        }

        case 22: {
            conf.branch_strategy_setup = "vsids1";
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

    if (data->solvers[0]->drat->enabled() ||
        data->solvers[0]->conf.simulate_drat
    ) {
        const char err[] = "ERROR: DRAT cannot be used in multi-threaded mode";
        std::cerr << err << endl;
        throw std::runtime_error(err);
    }

    if (data->cls > 0 || nVars() > 0) {
        const char err[] = "ERROR: You must first call set_num_threads() and only then add clauses and variables";
        std::cerr << err << endl;
        throw std::runtime_error(err);
    }

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
    {
    }

    void operator()()
    {
        Solver& solver = *data_for_thread.solvers[tid];
        solver.new_external_vars(data_for_thread.vars_to_add);

        vector<Lit> lits;
        vector<uint32_t> vars;
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
                ret = solver.add_clause_outer(lits);
            } else {
                vars.clear();
                at++;
                bool rhs = orig_lits[at].sign();
                at++;
                for(; at < size
                    && orig_lits[at] != lit_Undef
                    && orig_lits[at] != lit_Error
                    ; at++
                ) {
                    vars.push_back(orig_lits[at].var());
                }
                ret = solver.add_xor_clause_outer(vars, rhs);
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

static bool actually_add_clauses_to_threads(CMSatPrivateData* data)
{
    DataForThread data_for_thread(data);
    if (data->solvers.size() == 1) {
        OneThreadAddCls t(data_for_thread, 0);
        t.operator()();
    } else {
        std::vector<std::thread> thds;
        for(size_t i = 0; i < data->solvers.size(); i++) {
            thds.push_back(thread(OneThreadAddCls(data_for_thread, i)));
        }
        for(std::thread& thread : thds){
            thread.join();
        }
    }
    bool ret = (*data_for_thread.ret == l_True);

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

DLL_PUBLIC void SATSolver::set_max_confl(int64_t max_confl)
{
  assert(max_confl >= 0 && "Cannot set negative limit on conflicts");

  for (Solver* s : data->solvers) {
    uint64_t new_max = s->get_stats().conflStats.numConflicts + static_cast<uint64_t>(max_confl);
    bool would_overflow = std::numeric_limits<long>::max() < new_max
                       || new_max < s->get_stats().conflStats.numConflicts;

    // TBD: It is highly unlikely that an int64_t could overflow in practice,
    // meaning that this test is unlikely to ever fire. However, the conflict
    // limit inside the solver is stored as a long, which can be 32 bits
    // on some platforms. In practice that is also unlikely to be overflown,
    // but it needs some extra checks.
    s->conf.max_confl = would_overflow? std::numeric_limits<long>::max()
                                      : static_cast<long>(new_max);
  }
}

DLL_PUBLIC void SATSolver::set_default_polarity(bool polarity)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.polarity_mode = polarity ? PolarityMode::polarmode_pos : PolarityMode::polarmode_neg;
    }
}

DLL_PUBLIC void SATSolver::set_no_simplify()
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
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
    #ifndef USE_GAUSS
    std::cerr << "ERROR: CryptoMiniSat was not compiled with GAUSS" << endl;
    exit(-1);
    #else
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        //s.conf.reconfigure_at = 0;
        //s.conf.reconfigure_val = 15;
        s.conf.gaussconf.max_num_matrices = 10;
        s.conf.gaussconf.autodisable = false;
        s.conf.xor_detach_reattach = true;
        s.conf.allow_elim_xor_vars = false;
    }
    #endif
}

DLL_PUBLIC void SATSolver::set_no_simplify_at_startup()
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.simplify_at_startup = false;
    }
}

DLL_PUBLIC void SATSolver::set_no_equivalent_lit_replacement()
{
    set_scc(0);
}

DLL_PUBLIC void SATSolver::set_no_bva()
{
    set_bva(0);
}

DLL_PUBLIC void SATSolver::set_no_bve()
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.doVarElim = false;
    }
}

DLL_PUBLIC void SATSolver::set_greedy_undef()
{
    assert(false && "ERROR: Unfortunately, greedy undef is broken, please don't use it");
    std::cerr << "ERROR: Unfortunately, greedy undef is broken, please don't use it" << endl;
    exit(-1);
}

DLL_PUBLIC void SATSolver::set_sampling_vars(vector<uint32_t>* sampl_vars)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.sampling_vars = sampl_vars;
    }
}


DLL_PUBLIC void SATSolver::set_verbosity(unsigned verbosity)
{
    if (data->solvers.empty())
        return;

    Solver& s = *data->solvers[0];
    s.conf.verbosity = verbosity;
}

DLL_PUBLIC void SATSolver::set_timeout_all_calls(double timeout)
{
    data->timeout = timeout;
}

DLL_PUBLIC bool SATSolver::add_clause(const vector< Lit >& lits)
{
    if (data->log) {
        (*data->log) << lits << " 0" << endl;
    }

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

        ret = data->solvers[0]->add_clause_outer(lits);
        data->cls++;
    }

    return ret;
}

void add_xor_clause_to_log(const std::vector<unsigned>& vars, bool rhs, std::ofstream* file)
{
    if (vars.size() == 0) {
        if (rhs) {
            (*file) << "0" << endl;
        }
    } else {
        if (!rhs) {
            (*file) << "-";
        }
        for(unsigned var: vars) {
            (*file) << (var+1) << " ";
        }
        (*file) << " 0" << endl;
    }
}

DLL_PUBLIC bool SATSolver::add_xor_clause(const std::vector<unsigned>& vars, bool rhs)
{
    if (data->log) {
       add_xor_clause_to_log(vars, rhs, data->log);
    }

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

        ret = data->solvers[0]->add_xor_clause_outer(vars, rhs);
        data->cls++;
    }

    return ret;
}

struct OneThreadCalc
{
    OneThreadCalc(
        DataForThread& _data_for_thread,
        size_t _tid,
        bool _solve,
        bool _only_sampling_solution
    ) :
        data_for_thread(_data_for_thread)
        , tid(_tid)
        , solve(_solve)
        , only_sampling_solution(_only_sampling_solution)
    {}

    void operator()()
    {
        if (print_thread_start_and_finish) {
            start_time = cpuTime();
            //data_for_thread.update_mutex->lock();
            //cout << "c Starting thread " << tid << endl;
            //data_for_thread.update_mutex->unlock();
        }

        OneThreadAddCls cls_adder(data_for_thread, tid);
        cls_adder();
        lbool ret;
        if (solve) {
            ret = data_for_thread.solvers[tid]->solve_with_assumptions(data_for_thread.assumptions, only_sampling_solution);
        } else {
            ret = data_for_thread.solvers[tid]->simplify_with_assumptions(data_for_thread.assumptions);
        }

        data_for_thread.cpu_times[tid] = cpuTime();
        if (print_thread_start_and_finish) {
            data_for_thread.update_mutex->lock();
            ios::fmtflags f(cout.flags());
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
    bool solve;
    bool only_sampling_solution;
};

lbool calc(
    const vector< Lit >* assumptions,
    bool solve, CMSatPrivateData *data,
    bool only_sampling_solution = false
) {
    //Reset the interrupt signal if it was set
    data->must_interrupt->store(false, std::memory_order_relaxed);

    //Set timeout information
    if (data->timeout != std::numeric_limits<double>::max()) {
        for (size_t i = 0; i < data->solvers.size(); ++i) {
            Solver& s = *data->solvers[i];
            s.conf.maxTime = cpuTime() + data->timeout;
        }
    }

    if (data->log) {
        (*data->log) << "c Solver::"
        << (solve ? "solve" : "simplify")
        << "( ";
        if (assumptions) {
            (*data->log) << *assumptions;
        }
        (*data->log) << " )" << endl;
    }

    if (data->solvers.size() > 1 && data->sql > 0) {
        std::cerr
        << "Multithreaded solving and SQL cannot be specified at the same time"
        << endl;
        exit(-1);
    }

    if (data->solvers.size() == 1) {
        data->solvers[0]->new_vars(data->vars_to_add);
        data->vars_to_add = 0;

        lbool ret ;
        if (solve) {
            ret = data->solvers[0]->solve_with_assumptions(assumptions, only_sampling_solution);
        } else {
            ret = data->solvers[0]->simplify_with_assumptions(assumptions);
        }
        data->okay = data->solvers[0]->okay();
        data->cpu_times[0] = cpuTime();
        return ret;
    }

    //Multi-thread from now on.
    DataForThread data_for_thread(data, assumptions);
    std::vector<std::thread> thds;
    for(size_t i = 0
        ; i < data->solvers.size()
        ; i++
    ) {
        thds.push_back(thread(OneThreadCalc(data_for_thread, i, solve, only_sampling_solution)));
    }
    for(std::thread& thread : thds){
        thread.join();
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

    return calc(assumptions, true, data, only_sampling_solution);
}

DLL_PUBLIC lbool SATSolver::simplify(const vector< Lit >* assumptions)
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

    return calc(assumptions, false, data);
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
    return data->solvers[0]->nVarsOutside() + data->vars_to_add;
}

DLL_PUBLIC void SATSolver::new_var()
{
    new_vars(1);
}

DLL_PUBLIC void SATSolver::new_vars(const size_t n)
{
    if (n >= MAX_VARS
        || (data->vars_to_add + n) >= MAX_VARS
    ) {
        throw CMSat::TooManyVarsError();
    }

    if (data->log) {
        (*data->log) << "c Solver::new_vars( " << n << " )" << endl;
    }

    data->vars_to_add += n;
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

DLL_PUBLIC std::string SATSolver::get_text_version_info()
{
    std::stringstream ss;
    ss << "c CryptoMiniSat version " << get_version() << endl;
    ss << "c CMS Copyright Mate Soos (soos.mate@gmail.com)" << endl;
    ss << "c CMS SHA revision " << get_version_sha1() << endl;
    #ifdef USE_M4RI
    ss << "c CMS is GPL licensed due to M4RI being linked. Build without M4RI to get MIT version" << endl;
    #else
    ss << "c CMS is MIT licensed" << endl;
    #endif
    ss << "c Using VMTF code by Armin Biere from CaDiCaL" << endl;
    ss << "c Using Yalsat by Armin Biere, see Balint et al. Improving implementation of SLS solvers [...], SAT'14" << endl;
    ss << "c Using WalkSAT by Henry Kautz, see Kautz and Selman Pushing the envelope: planning, propositional logic, and stochastic search, AAAI'96," << endl;
    #ifdef USE_BREAKID
    ss << "c Using BreakID by Devriendt, Bogaerts, Bruynooghe and Denecker" << endl;
    ss << "c Using Bliss graph automorphism library (under LGPL) by Tommi Junttila" << endl;
    #endif

    #ifdef USE_GAUSS
    ss << "c Using code from 'When Boolean Satisfiability Meets Gauss-E. in a Simplex Way'" << endl;
    ss << "c       by C.-S. Han and J.-H. Roland Jiang in CAV 2012. Fixes by M. Soos" << endl;
    #endif
    ss << "c Using CCAnr from 'CCAnr: A Conf. Checking Based Local Search Solver [...]'" << endl;
    ss << "c       by Shaowei Cai, Chuan Luo, and Kaile Su, SAT 2015" << endl;
    ss << "c CMS compilation env " << get_compilation_env() << endl;
    #ifdef __GNUC__
    ss << "c CMS compiled with gcc version " << __VERSION__ << endl;
    #else
    ss << "c CMS compiled with non-gcc compiler" << endl;
    #endif

    return ss.str();
}

DLL_PUBLIC void SATSolver::print_stats() const
{
    double cpu_time_total = cpuTimeTotal();

    double cpu_time;
    if (data->interrupted) {
        //cannot know, we have in fact no idea how much time passed...
        //we have to guess. Shitty guess comes here... :S
        cpu_time = cpuTimeTotal()/(double)data->solvers.size();
    } else {
        cpu_time = data->cpu_times[data->which_solved];
    }

    //If only one thread, then don't confuse the user. The difference
    //is minimal.
    if (data->solvers.size() == 1) {
        cpu_time = cpu_time_total;
    }

    data->solvers[data->which_solved]->print_stats(cpu_time, cpu_time_total);
}

DLL_PUBLIC void SATSolver::set_drat(std::ostream* os, bool add_ID)
{
    if (data->solvers.size() > 1) {
        std::cerr << "ERROR: DRAT cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }
    if (nVars() > 0) {
        std::cerr << "ERROR: DRAT cannot be set after variables have been added" << endl;
        exit(-1);
    }

    data->solvers[0]->conf.gaussconf.doMatrixFind = false;
    data->solvers[0]->conf.doBreakid = false;
    data->solvers[0]->add_drat(os, add_ID);
    data->solvers[0]->conf.do_hyperbin_and_transred = true;
    data->solvers[0]->conf.doFindXors = false;
    data->solvers[0]->conf.doCompHandler = false;

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
    return data->okay;
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
SATSolver::get_recovered_xors(bool xor_together_xors) const
{
    vector<std::pair<vector<uint32_t>, bool> > ret;
    Solver& s = *data->solvers[0];

    std::pair<vector<uint32_t>, bool> tmp;
    vector<Xor> xors = s.get_recovered_xors(xor_together_xors);
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
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
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
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
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

DLL_PUBLIC void SATSolver::dump_irred_clauses(std::ostream *out) const
{
    data->solvers[data->which_solved]->dump_irred_clauses(out);
}

void DLL_PUBLIC SATSolver::dump_red_clauses(std::ostream *out) const
{
    data->solvers[data->which_solved]->dump_red_clauses(out);
}

DLL_PUBLIC void SATSolver::open_file_and_dump_irred_clauses(std::string fname) const
{
    data->solvers[data->which_solved]->open_file_and_dump_irred_clauses(fname);
}

void DLL_PUBLIC SATSolver::open_file_and_dump_red_clauses(std::string fname) const
{
    data->solvers[data->which_solved]->open_file_and_dump_red_clauses(fname);
}

void DLL_PUBLIC SATSolver::start_getting_small_clauses(uint32_t max_len, uint32_t max_glue)
{
    assert(data->solvers.size() >= 1);
    data->solvers[0]->start_getting_small_clauses(max_len, max_glue);
}

bool DLL_PUBLIC SATSolver::get_next_small_clause(std::vector<Lit>& out)
{
    assert(data->solvers.size() >= 1);
    return data->solvers[0]->get_next_small_clause(out);
}

void DLL_PUBLIC SATSolver::end_getting_small_clauses()
{
    assert(data->solvers.size() >= 1);
    data->solvers[0]->end_getting_small_clauses();
}

void DLL_PUBLIC SATSolver::set_up_for_scalmc()
{
    for (size_t i = 0; i < data->solvers.size(); i++) {
        SolverConf conf = data->solvers[i]->getConf();
        conf.doBreakid = false;
        conf.gaussconf.max_num_matrices = 2;
        conf.gaussconf.autodisable = false;
        conf.xor_detach_reattach = true;
        conf.global_multiplier_multiplier_max = 1;
        conf.orig_global_timeout_multiplier = 1.5;
        conf.min_bva_gain = 2;
        conf.xor_finder_time_limitM = 400;
        conf.polar_stable_every_n = 100000; //i.e. never
        uint32_t xor_cut = 4;
        assert(xor_cut >= 3);
        conf.xor_var_per_cut = xor_cut-2;

        conf.simplify_at_startup = 1;
        conf.varElimRatioPerIter = 1;
        conf.restartType = Restart::geom;
        conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;
        conf.branch_strategy_setup = "vsids1";
        conf.bva_every_n = 1;
        conf.do_simplify_problem = true;
        conf.force_preserve_xors = true;
        conf.diff_declev_for_chrono = -1;
        data->solvers[i]->setConf(conf);
    }
}

DLL_PUBLIC void SATSolver::set_verbosity_detach_warning(bool verb)
{
    for (size_t i = 0; i < data->solvers.size(); i++) {
        SolverConf conf = data->solvers[i]->getConf();
        conf.xor_detach_verb = verb;
        data->solvers[i]->setConf(conf);
    }
}

DLL_PUBLIC void SATSolver::add_empty_cl_to_drat()
{
    data->solvers[data->which_solved]->add_empty_cl_to_drat();
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

DLL_PUBLIC void SATSolver::set_var_weight(Lit lit, double weight)
{
    actually_add_clauses_to_threads(data);
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.set_var_weight(lit, weight);
    }
}

DLL_PUBLIC vector<uint32_t> SATSolver::get_var_incidence()
{
    return data->solvers[data->which_solved]->get_outside_var_incidence();
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
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.do_bva = val;
    }
}

DLL_PUBLIC void SATSolver::set_full_bve_iter_ratio(double val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.varElimRatioPerIter = val;
    }
}

DLL_PUBLIC void SATSolver::set_xor_detach(bool val)
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.xor_detach_reattach = val;
    }
}


DLL_PUBLIC void SATSolver::set_yes_comphandler()
{
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.doCompHandler = true;
        s.enable_comphandler();
    }
}
