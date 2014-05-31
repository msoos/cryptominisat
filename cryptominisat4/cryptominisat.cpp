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
using std::thread;
using std::mutex;

#define CACHE_SIZE 10ULL*1000ULL*1000UL
#define MY_SOLVERS \
    Data& data = *((Data*)s);

using namespace CMSat;

static const bool print_thread_start_and_finish = false;

struct Data {
    Data(bool* _interrupt_asap) {
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
};

struct DataForThread
{
    explicit DataForThread(Data& data, vector<Lit>* _assumptions = NULL) :
        solvers(data.solvers)
        , lits_to_add(&data.cls_lits)
        , vars_to_add(data.vars_to_add)
        , assumptions(_assumptions)
        , update_mutex(new mutex)
        , which_solved(&data.which_solved)
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
    s = (void*)(new Data(interrupt_asap));
    MY_SOLVERS
    data.solvers.push_back(new Solver(conf, data.inter));
}

SATSolver::~SATSolver()
{
    MY_SOLVERS
    for(Solver* this_s: data.solvers) {
        delete this_s;
    }
    delete data.shared_data;
}

void SATSolver::set_num_threads(const unsigned num)
{
    MY_SOLVERS
    if (num <= 0) {
        std::cerr << "ERROR: Number of threads must be at least 1" << endl;
        exit(-1);
    }
    if (num == 1) {
        return;
    }

    if (data.solvers[0]->drup->enabled()) {
        std::cerr << "ERROR: DRUP cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }

    if (data.cls > 0 || nVars() > 0) {
        std::cerr << "ERROR: You must first call set_num_threads() and only then add clauses and variables" << endl;
        exit(-1);
    }

    data.cls_lits.reserve(CACHE_SIZE);
    for(unsigned i = 1; i < num; i++) {
        SolverConf conf = data.solvers[0]->getConf();
        switch(i) {
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
        data.solvers.push_back(new Solver(conf, data.inter));
    }

    //set shared data
    data.shared_data = new SharedData(data.solvers.size());
    for(unsigned i = 0; i < num; i++) {
        SolverConf conf = data.solvers[i]->getConf();
        if (i >= 1) {
            conf.verbosity = 0;
            conf.doSQL = 0;
            conf.doFindXors = 0;
        }
        data.solvers[i]->setConf(conf);
        data.solvers[i]->set_shared_data((SharedData*)data.shared_data, i);
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

static bool actually_add_clauses_to_threads(Data& data)
{
    DataForThread data_for_thread(data);
    std::vector<std::thread> thds;
    for(size_t i = 0; i < data.solvers.size(); i++) {
        thds.push_back(thread(OneThreadAddCls(data_for_thread, i)));
    }
    for(std::thread& thread : thds){
        thread.join();
    }
    bool ret = (*data_for_thread.ret == l_True);

    //clear what has been added
    data.cls_lits.clear();
    data.vars_to_add = 0;

    return ret;
}

bool SATSolver::add_clause(const vector< Lit >& lits)
{
    MY_SOLVERS
    bool ret = true;
    if (data.solvers.size() > 1) {
        if (data.cls_lits.size() + lits.size() + 1 > CACHE_SIZE) {
            ret = actually_add_clauses_to_threads(data);
        }

        data.cls_lits.push_back(lit_Undef);
        for(Lit lit: lits) {
            data.cls_lits.push_back(lit);
        }
    } else {
        assert(data.solvers.size() == 1);
        ret = data.solvers[0]->add_clause_outer(lits);
        data.cls++;
    }

    return ret;
}

bool SATSolver::add_xor_clause(const std::vector<unsigned>& vars, bool rhs)
{
    MY_SOLVERS
    bool ret = true;
    if (data.solvers.size() > 1) {
        if (data.cls_lits.size() + vars.size() + 1 > CACHE_SIZE) {
            ret = actually_add_clauses_to_threads(data);
        }

        data.cls_lits.push_back(lit_Error);
        data.cls_lits.push_back(Lit(0, rhs));
        for(Var var: vars) {
            data.cls_lits.push_back(Lit(var, false));
        }
    } else {
        assert(data.solvers.size() == 1);
        ret = data.solvers[0]->add_xor_clause_outer(vars, rhs);
        data.cls++;
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
    MY_SOLVERS
    if (data.solvers.size() == 1) {
        lbool ret = data.solvers[0]->solve_with_assumptions(assumptions);
        data.okay = data.solvers[0]->okay();
        return ret;
    }

    DataForThread data_for_thread(data, assumptions);
    std::vector<std::thread> thds;
    for(size_t i = 0; i < data.solvers.size(); i++) {
        thds.push_back(thread(OneThreadSolve(data_for_thread, i)));
    }
    for(std::thread& thread : thds){
        thread.join();
    }
    lbool real_ret = *data_for_thread.ret;

    //clear what has been added
    data.cls_lits.clear();
    data.vars_to_add = 0;
    data.okay = data.solvers[*data_for_thread.which_solved]->okay();

    return real_ret;
}

const vector< lbool >& SATSolver::get_model() const
{
    MY_SOLVERS
    return data.solvers[data.which_solved]->get_model();
}

const std::vector<Lit>& SATSolver::get_conflict() const
{
    MY_SOLVERS
    return data.solvers[data.which_solved]->get_final_conflict();
}

uint32_t SATSolver::nVars() const
{
    MY_SOLVERS
    if (data.solvers.size() == 0) {
        assert(data.vars_to_add == 0);
        return data.solvers[0]->nVarsOutside();
    } else {
        return data.solvers[0]->nVarsOutside() + data.vars_to_add;
    }
}

void SATSolver::new_var()
{
    MY_SOLVERS
    if (data.solvers.size() == 1) {
        data.solvers[0]->new_external_var();
    } else {
        data.vars_to_add += 1;
    }
}

void SATSolver::new_vars(const size_t n)
{
    MY_SOLVERS
    if (data.solvers.size() == 1) {
        data.solvers[0]->new_external_vars(n);
    } else {
        data.vars_to_add += n;
    }
}

void SATSolver::add_sql_tag(const std::string& tagname, const std::string& tag)
{
    MY_SOLVERS
    for(size_t i = 0; i < data.solvers.size(); i++) {
        data.solvers[i]->add_sql_tag(tagname, tag);
    }
}

const char* SATSolver::get_version()
{
    return Solver::getVersion();
}

void SATSolver::print_stats() const
{
    MY_SOLVERS
    data.solvers[data.which_solved]->printStats();
}

void SATSolver::set_drup(std::ostream* os)
{
    MY_SOLVERS
    if (data.solvers.size() > 1) {
        std::cerr << "ERROR: DRUP cannot be used in multi-threaded mode" << endl;
        exit(-1);
    }
    DrupFile* drup = new DrupFile();
    drup->setFile(os);
    data.solvers[0]->drup = drup;
}

void SATSolver::interrupt_asap()
{
    MY_SOLVERS
    for(Solver* solver: data.solvers) {
        solver->set_must_interrupt_asap();
    }
}

void SATSolver::open_file_and_dump_irred_clauses(std::string fname) const
{
    MY_SOLVERS
    data.solvers[data.which_solved]->open_file_and_dump_irred_clauses(fname);
}

void SATSolver::open_file_and_dump_red_clauses(std::string fname) const
{
    MY_SOLVERS
    data.solvers[data.which_solved]->open_file_and_dump_red_clauses(fname);
}

void SATSolver::add_in_partial_solving_stats()
{
    MY_SOLVERS
    data.solvers[data.which_solved]->add_in_partial_solving_stats();
}

std::vector<Lit> SATSolver::get_zero_assigned_lits() const
{
    MY_SOLVERS
    return data.solvers[data.which_solved]->get_zero_assigned_lits();
}

unsigned long SATSolver::get_sql_id() const
{
    MY_SOLVERS
    return data.solvers[0]->get_sql_id();
}

SolverConf SATSolver::get_conf() const
{
    MY_SOLVERS
    return data.solvers[0]->getConf();
}

bool SATSolver::okay() const
{
    MY_SOLVERS
    return data.okay;
}
