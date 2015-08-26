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

#include "constants.h"
#include "cryptominisat4/cryptominisat.h"
#include "solver.h"
#include "drup.h"
#include "shareddata.h"
#include <stdexcept>
#include <thread>
#include <mutex>
#include <fstream>
#include <cstdlib>
using std::thread;
using std::mutex;

#define CACHE_SIZE 10ULL*1000ULL*1000UL

using namespace CMSat;

static const bool print_thread_start_and_finish = false;

namespace CMSat {
    struct CMSatPrivateData {
        explicit CMSatPrivateData(bool* _must_interrupt) {
            cls = 0;
            vars_to_add = 0;
            must_interrupt = _must_interrupt;
            which_solved = 0;
            shared_data = NULL;
            okay = true;
        }
        ~CMSatPrivateData()
        {
            delete log; //this will also close the file
            delete shared_data;
        }
        CMSatPrivateData(CMSatPrivateData&) //copy should fail
        {
            std::exit(-1);
        }
        CMSatPrivateData(const CMSatPrivateData&) //copy should fail
        {
            std::exit(-1);
        }

        vector<Solver*> solvers;
        SharedData *shared_data;
        int which_solved;
        bool* must_interrupt;
        bool must_interrupt_needs_free = false;
        unsigned cls;
        unsigned vars_to_add;
        vector<Lit> cls_lits;
        bool okay;
        std::ofstream* log = NULL;
    };
}

struct DataForThread
{
    explicit DataForThread(CMSatPrivateData* data, const vector<Lit>* _assumptions = NULL) :
        solvers(data->solvers)
        , lits_to_add(&(data->cls_lits))
        , vars_to_add(data->vars_to_add)
        , assumptions(_assumptions)
        , update_mutex(new mutex)
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
    vector<Lit> *lits_to_add;
    uint32_t vars_to_add;
    const vector<Lit> *assumptions;
    mutex* update_mutex;
    int *which_solved;
    lbool* ret;
};

DLL_PUBLIC SATSolver::SATSolver(void* config, bool* interrupt_asap)
{
    if (interrupt_asap == NULL) {
        data = new CMSatPrivateData(new bool);
        data->must_interrupt_needs_free = true;
    } else {
        data = new CMSatPrivateData(interrupt_asap);
    }

    data->solvers.push_back(new Solver((SolverConf*) config, data->must_interrupt));
}

DLL_PUBLIC SATSolver::~SATSolver()
{
    for(Solver* this_s: data->solvers) {
        delete this_s;
    }
    if (data->must_interrupt_needs_free) {
        delete data->must_interrupt;
    }
    delete data;
}

void update_config(SolverConf& conf, unsigned thread_num)
{
    thread_num = thread_num % 20;

    //Don't accidentally reconfigure everything to a specific value!
    if (thread_num > 0) {
        conf.reconfigure_val = 0;
    }

    switch(thread_num) {
        case 1: {
            //Minisat-like
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;
            conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;

            conf.inc_max_temp_red_cls = 1.02;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::size)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.5;
            break;
        }
        case 2: {
            //Similar to old CMS except we look at learnt DB size insteead
            //of conflicts to see if we need to clean.
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::size)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.5;
            conf.glue_must_keep_clause_if_below_or_eq = 0;
            conf.inc_max_temp_red_cls = 1.03;
            break;
        }
        case 3: {
            conf.max_temporary_learnt_clauses = 40000;
            conf.var_decay_max = 0.80;
            break;
        }
        case 4: {
            conf.never_stop_search = true;
            break;
        }
        case 5: {
            conf.max_temporary_learnt_clauses = 30000;
            break;
        }
        case 6: {
            conf.do_bva = false;
            conf.glue_must_keep_clause_if_below_or_eq = 2;
            conf.varElimRatioPerIter = 1;
            conf.inc_max_temp_red_cls = 1.04;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.1;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::size)] = 0.1;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.3;
            conf.var_decay_max = 0.90; //more 'slow' in adjusting activities
            break;
        }

        case 7: {
            conf.global_timeout_multiplier = 5;
            conf.num_conflicts_of_search_inc = 1.15;
            conf.more_red_minim_limit_cache = 1200;
            conf.more_red_minim_limit_binary = 600;
            conf.max_num_lits_more_red_min = 20;
            conf.max_temporary_learnt_clauses = 10000;
            conf.var_decay_max = 0.99; //more 'fast' in adjusting activities
            break;
        }
        case 8: {
            //Different glue limit
            conf.glue_must_keep_clause_if_below_or_eq = 4;
            conf.max_num_lits_more_red_min = 3;
            conf.max_glue_more_minim = 4;
            break;
        }
        case 9: {
            //Different glue limit
            conf.glue_must_keep_clause_if_below_or_eq = 7;
            break;
        }
        case 10: {
            //Luby
            conf.restart_inc = 1.5;
            conf.restart_first = 100;
            conf.restartType = CMSat::Restart::luby;
            break;
        }
        case 11: {
            conf.glue_must_keep_clause_if_below_or_eq = 3;
            conf.var_decay_max = 0.97;
            break;
        }
        case 12: {
            conf.var_decay_max = 0.998;
            break;
        }
        case 13: {
            conf.polarity_mode = CMSat::PolarityMode::polarmode_pos;
            break;
        }
        case 14: {
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;

            conf.inc_max_temp_red_cls = 1.01;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::size)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.3;
        }
        case 15: {
            conf.inc_max_temp_red_cls = 1.001;
            break;
        }

        default: {
            break;
        }
    }
}

DLL_PUBLIC void SATSolver::set_num_threads(const unsigned num)
{
    if (num <= 0) {
        std::cerr << "ERROR: Number of threads must be at least 1" << endl;
        exit(-1);
    }
    if (num == 1) {
        return;
    }

    if (data->solvers[0]->drup->enabled()) {
        std::cerr << "ERROR: DRUP cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }

    if (data->cls > 0 || nVars() > 0) {
        std::cerr << "ERROR: You must first call set_num_threads() and only then add clauses and variables" << endl;
        exit(-1);
    }

    data->cls_lits.reserve(CACHE_SIZE);
    for(unsigned i = 1; i < num; i++) {
        SolverConf conf = data->solvers[0]->getConf();
        conf.doSQL = 0;
        update_config(conf, i);
        data->solvers.push_back(new Solver(&conf, data->must_interrupt));
    }

    //set shared data
    data->shared_data = new SharedData(data->solvers.size());
    for(unsigned i = 0; i < num; i++) {
        SolverConf conf = data->solvers[i]->getConf();
        if (i >= 1) {
            conf.verbosity = 0;
            conf.doSQL = 0;
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
        vector<Var> vars;
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
    std::vector<std::thread> thds;
    for(size_t i = 0; i < data->solvers.size(); i++) {
        thds.push_back(thread(OneThreadAddCls(data_for_thread, i)));
    }
    for(std::thread& thread : thds){
        thread.join();
    }
    bool ret = (*data_for_thread.ret == l_True);

    //clear what has been added
    data->cls_lits.clear();
    data->vars_to_add = 0;

    return ret;
}

DLL_PUBLIC void SATSolver::set_max_confl(int64_t max_confl)
{
  for (size_t i = 0; i < data->solvers.size(); ++i) {
    Solver& s = *data->solvers[i];
    if (max_confl >= 0) {
      s.conf.maxConfl = max_confl;
    }
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
        s.conf.simplify_at_startup = false;
        s.conf.simplify_at_every_startup = false;
        s.conf.full_simplify_at_startup = false;
        s.conf.perform_occur_based_simp = false;
        s.conf.do_simplify_problem = false;
    }
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
    for (size_t i = 0; i < data->solvers.size(); ++i) {
        Solver& s = *data->solvers[i];
        s.conf.doFindAndReplaceEqLits = false;
    }
}

DLL_PUBLIC void SATSolver::set_verbosity(unsigned verbosity)
{
  for (size_t i = 0; i < data->solvers.size(); ++i) {
    Solver& s = *data->solvers[i];
    s.conf.verbosity = verbosity;
  }
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
            (*file) << "0" << endl;;
        }
    } else {
        if (!rhs) {
            (*file) << "-";
        }
        for(unsigned var: vars) {
            (*file) << (var+1) << " ";
        }
        (*file) << " 0" << endl;;
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
        for(Var var: vars) {
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

struct OneThreadSolve
{
    OneThreadSolve(DataForThread& _data_for_thread, size_t _tid) :
        data_for_thread(_data_for_thread)
        , tid(_tid)
    {}

    void operator()()
    {
        if (print_thread_start_and_finish) {
            data_for_thread.update_mutex->lock();
            cout << "Starting thread" << tid << endl;
            data_for_thread.update_mutex->unlock();
        }

        OneThreadAddCls cls_adder(data_for_thread, tid);
        cls_adder();
        lbool ret = data_for_thread.solvers[tid]->solve_with_assumptions(data_for_thread.assumptions);

        if (print_thread_start_and_finish) {
            data_for_thread.update_mutex->lock();
            cout << "Finished tread " << tid << " with result: " << ret << endl;
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
        data_for_thread.solvers[tid]->unset_must_interrupt_asap();
    }

    DataForThread& data_for_thread;
    const size_t tid;
};

DLL_PUBLIC lbool SATSolver::solve(const vector< Lit >* assumptions)
{
    //Reset the interrupt signal if it was set
    *(data->must_interrupt) = false;

    if (data->log) {
        (*data->log) << "c Solver::solve( ";
        if (assumptions) {
            (*data->log) << *assumptions;
        }
        (*data->log) << " )" << endl;
    }

    if (data->solvers.size() == 1) {
        data->solvers[0]->new_vars(data->vars_to_add);
        data->vars_to_add = 0;

        lbool ret = data->solvers[0]->solve_with_assumptions(assumptions);
        data->okay = data->solvers[0]->okay();
        return ret;
    }

    DataForThread data_for_thread(data, assumptions);
    std::vector<std::thread> thds;
    for(size_t i = 0; i < data->solvers.size(); i++) {
        thds.push_back(thread(OneThreadSolve(data_for_thread, i)));
    }
    for(std::thread& thread : thds){
        thread.join();
    }
    lbool real_ret = *data_for_thread.ret;

    //clear what has been added
    data->cls_lits.clear();
    data->vars_to_add = 0;
    data->okay = data->solvers[*data_for_thread.which_solved]->okay();

    return real_ret;
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
    if (data->log) {
        (*data->log) << "c Solver::new_var()" << endl;
    }
    data->vars_to_add += 1;
}

DLL_PUBLIC void SATSolver::new_vars(const size_t n)
{
    if (data->log) {
        (*data->log) << "c Solver::new_vars( " << n << " )" << endl;
    }

    data->vars_to_add += n;
}

DLL_PUBLIC void SATSolver::add_sql_tag(const std::string& tagname, const std::string& tag)
{
    for(Solver* solver: data->solvers) {
        solver->add_sql_tag(tagname, tag);
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

DLL_PUBLIC void SATSolver::print_stats() const
{
    data->solvers[data->which_solved]->print_stats();
}

DLL_PUBLIC void SATSolver::set_drup(std::ostream* os)
{
    if (data->solvers.size() > 1) {
        std::cerr << "ERROR: DRUP cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }
    DrupFile* drup = new DrupFile();
    drup->setFile(os);
    if (data->solvers[0]->drup)
        delete data->solvers[0]->drup;

    data->solvers[0]->drup = drup;
}

DLL_PUBLIC void SATSolver::interrupt_asap()
{
    *(data->must_interrupt) = true;
}

DLL_PUBLIC void SATSolver::open_file_and_dump_irred_clauses(std::string fname) const
{
    data->solvers[data->which_solved]->open_file_and_dump_irred_clauses(fname);
}

void DLL_PUBLIC SATSolver::open_file_and_dump_red_clauses(std::string fname) const
{
    data->solvers[data->which_solved]->open_file_and_dump_red_clauses(fname);
}

void DLL_PUBLIC SATSolver::add_in_partial_solving_stats()
{
    data->solvers[data->which_solved]->add_in_partial_solving_stats();
}

DLL_PUBLIC std::vector<Lit> SATSolver::get_zero_assigned_lits() const
{
    return data->solvers[data->which_solved]->get_zero_assigned_lits();
}

DLL_PUBLIC unsigned long SATSolver::get_sql_id() const
{
    return data->solvers[0]->get_sql_id();
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
