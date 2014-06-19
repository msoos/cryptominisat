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

#include "cryptominisat.h"
#include "solver.h"
#include "drup.h"
#include "shareddata.h"
#include <stdexcept>
#include <thread>
#include <mutex>
#include <fstream>
using std::thread;
using std::mutex;

#define CACHE_SIZE 10ULL*1000ULL*1000UL

using namespace CMSat;

static const bool print_thread_start_and_finish = false;

namespace CMSat {
    struct CMSatPrivateData {
        CMSatPrivateData(bool* _interrupt_asap) {
            cls = 0;
            vars_to_add = 0;
            inter = _interrupt_asap;
            which_solved = 0;
            shared_data = NULL;
            okay = true;
        }
        vector<Solver*> solvers;
        SharedData *shared_data;
        int which_solved;
        bool* inter;
        unsigned cls;
        unsigned vars_to_add;
        vector<Lit> cls_lits;
        bool okay;
        std::ofstream* log = NULL;
    };
}

struct DataForThread
{
    explicit DataForThread(CMSatPrivateData* data, vector<Lit>* _assumptions = NULL) :
        solvers(data->solvers)
        , lits_to_add(&(data->cls_lits))
        , vars_to_add(data->vars_to_add)
        , assumptions(_assumptions)
        , update_mutex(new mutex)
        , which_solved(&(data->which_solved))
        , ret(new lbool(l_True))
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
    vector<Lit> *assumptions;
    mutex* update_mutex;
    int *which_solved;
    lbool* ret;
};

SATSolver::SATSolver(const SolverConf conf, bool* interrupt_asap)
{
    data = new CMSatPrivateData(interrupt_asap);
    data->solvers.push_back(new Solver(conf, data->inter));
}

SATSolver::~SATSolver()
{
    for(Solver* this_s: data->solvers) {
        delete this_s;
    }
    delete data->shared_data;
    if (data->log) {
        data->log->close();
        delete data->log;
    }
}

void update_config(SolverConf& conf, unsigned thread_num)
{
    switch(thread_num) {
        case 1: {
            conf.restartType = restart_type_geom;
            conf.polarity_mode = CMSat::polarmode_neg;
            conf.varElimRatioPerIter = 1;
            break;
        }
        case 2: {
            conf.shortTermHistorySize = 80;
            conf.clauseCleaningType = CMSat::clean_glue_based;
            conf.restartType = CMSat::restart_type_glue;
            conf.increaseClean = 1.08;
            conf.ratioRemoveClauses = 0.55;
            break;
        }
        case 3: {
            conf.doVarElim = 0;
            conf.doGateFind = 0;
            conf.more_red_minim_limit_cache = 400;
            conf.more_red_minim_limit_binary = 200;
            conf.probe_bogoprops_timeoutM = 3500;
            conf.restartType = CMSat::restart_type_agility;
            conf.ratioRemoveClauses = 0.6;
            break;
        }
        case 4: {
            conf.simplify_at_startup = 1;
            conf.regularly_simplify_problem = 0;
            conf.varElimRatioPerIter = 1;
            conf.restartType = restart_type_geom;
            conf.clauseCleaningType = CMSat::clean_sum_activity_based;
            conf.polarity_mode = CMSat::polarmode_neg;
            conf.ratioRemoveClauses = 0.65;
            break;
        }
        case 5: {
            conf.doGateFind = 0;
            conf.more_red_minim_limit_cache = 100;
            conf.more_red_minim_limit_binary = 100;
            conf.probe_bogoprops_timeoutM = 4000;
            conf.ratioRemoveClauses = 0.6;
            break;
        }
        case 6: {
            conf.numCleanBetweenSimplify = 1;
            conf.skip_some_bve_resolvents = 1;
            conf.ratioRemoveClauses = 0.7;
            break;
        }
        case 7: {
            conf.clauseCleaningType = CMSat::clean_sum_confl_depth_based;
            conf.ratioRemoveClauses = 0.55;
            break;
        }
        case 8: {
            conf.polarity_mode = CMSat::polarmode_pos;
            conf.ratioRemoveClauses = 0.6;
            break;
        }
        case 9: {
            conf.do_bva = 0;
            conf.doGateFind = 0;
            conf.more_red_minim_limit_cache = 800;
            conf.more_red_minim_limit_binary = 400;
            conf.polarity_mode = CMSat::polarmode_neg;
            conf.ratioRemoveClauses = 0.6;
            break;
        }
        case 10: {
            conf.do_bva = 0;
            conf.doGateFind = 0;
            conf.restartType = CMSat::restart_type_agility;
            conf.clauseCleaningType = CMSat::clean_glue_based;
            conf.ratioRemoveClauses = 0.6;
            break;
        }
        case 11: {
            conf.simplify_at_startup = 1;
            conf.propBinFirst = 1;
            conf.doLHBR = 1;
            conf.increaseClean = 1.12;
            conf.ratioRemoveClauses = 0.7;
            break;
        }
        default: {
            conf.clauseCleaningType = CMSat::clean_glue_based;
            conf.ratioRemoveClauses = 0.7;
            break;
        }
    }
}

void SATSolver::set_num_threads(const unsigned num)
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
        update_config(conf, i);
        data->solvers.push_back(new Solver(conf, data->inter));
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
        data->solvers[i]->set_shared_data((SharedData*)data->shared_data, i);
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

bool SATSolver::add_clause(const vector< Lit >& lits)
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

bool SATSolver::add_xor_clause(const std::vector<unsigned>& vars, bool rhs)
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
            for(size_t i = 0; i < data_for_thread.solvers.size(); i++) {
                if (i == tid) {
                    continue;
                }

                data_for_thread.solvers[i]->set_must_interrupt_asap();
            }
            data_for_thread.update_mutex->unlock();
        }
        data_for_thread.solvers[tid]->unset_must_interrupt_asap();
    }

    DataForThread& data_for_thread;
    const size_t tid;
};

lbool SATSolver::solve(vector< Lit >* assumptions)
{
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

const vector< lbool >& SATSolver::get_model() const
{
    return data->solvers[data->which_solved]->get_model();
}

const std::vector<Lit>& SATSolver::get_conflict() const
{

    return data->solvers[data->which_solved]->get_final_conflict();
}

uint32_t SATSolver::nVars() const
{
    return data->solvers[0]->nVarsOutside() + data->vars_to_add;
}

void SATSolver::new_var()
{
    if (data->log) {
        (*data->log) << "c Solver::new_var()" << endl;
    }
    data->vars_to_add += 1;
}

void SATSolver::new_vars(const size_t n)
{
    if (data->log) {
        (*data->log) << "c Solver::new_vars( " << n << " )" << endl;
    }

    data->vars_to_add += n;
}

void SATSolver::add_sql_tag(const std::string& tagname, const std::string& tag)
{
    for(size_t i = 0; i < data->solvers.size(); i++) {
        data->solvers[i]->add_sql_tag(tagname, tag);
    }
}

const char* SATSolver::get_version()
{
    return Solver::getVersion();
}

void SATSolver::print_stats() const
{
    data->solvers[data->which_solved]->printStats();
}

void SATSolver::set_drup(std::ostream* os)
{
    if (data->solvers.size() > 1) {
        std::cerr << "ERROR: DRUP cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }
    DrupFile* drup = new DrupFile();
    drup->setFile(os);
    data->solvers[0]->drup = drup;
}

void SATSolver::interrupt_asap()
{
    for(Solver* solver: data->solvers) {
        solver->set_must_interrupt_asap();
    }
}

void SATSolver::open_file_and_dump_irred_clauses(std::string fname) const
{
    data->solvers[data->which_solved]->open_file_and_dump_irred_clauses(fname);
}

void SATSolver::open_file_and_dump_red_clauses(std::string fname) const
{
    data->solvers[data->which_solved]->open_file_and_dump_red_clauses(fname);
}

void SATSolver::add_in_partial_solving_stats()
{
    data->solvers[data->which_solved]->add_in_partial_solving_stats();
}

std::vector<Lit> SATSolver::get_zero_assigned_lits() const
{
    return data->solvers[data->which_solved]->get_zero_assigned_lits();
}

unsigned long SATSolver::get_sql_id() const
{
    return data->solvers[0]->get_sql_id();
}

SolverConf SATSolver::get_conf() const
{
    return data->solvers[0]->getConf();
}

bool SATSolver::okay() const
{
    return data->okay;
}

void SATSolver::log_to_file(std::string filename)
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

std::vector<std::pair<Lit, Lit> > SATSolver::get_all_binary_xors() const
{
    return data->solvers[0]->get_all_binary_xors();
}
