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
#define MY_SOLVERS vector<Solver*>* solvers = (vector<Solver*>*)s;

using namespace CMSat;

struct topass
{
    vector<Solver*> *solvers;
    vector<Lit> *lits_to_add;
    vector<Lit> *assumptions;
    mutex* update_mutex;
    int tid;
    int *which_solved;
    lbool* ret;
};

SATSolver::SATSolver(const SolverConf conf, bool* interrupt_asap)
{
    cls = 0;
    inter = interrupt_asap;
    which_solved = 0;
    shared_data = NULL;
    s = (void*)new vector<Solver*>;
    MY_SOLVERS
    solvers->push_back(new Solver(conf, inter));
}

SATSolver::~SATSolver()
{
    MY_SOLVERS
    for(Solver* this_s: *solvers) {
        delete this_s;
    }
    delete solvers;
    SharedData* sh = (SharedData*)shared_data;
    delete sh;
}

void SATSolver::set_num_threads(unsigned num)
{
    MY_SOLVERS
    if (num <= 0) {
        std::cerr << "ERROR: Number of threads must be at least 1" << endl;
        exit(-1);
    }
    if (num == 1)
        return;

    if (cls > 0 || nVars() > 0) {
        std::cerr << "ERROR: You must first call set_num_threads() and only then add clauses and variables" << endl;
        exit(-1);
    }

    cls_lits.reserve(CACHE_SIZE);
    for(unsigned i = 1; i < num; i++) {
        SolverConf conf = solvers->at(0)->getConf();
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
        solvers->push_back(new Solver(conf, inter));
    }

    //set shared data
    shared_data = (void*)new SharedData(solvers->size());
    for(unsigned i = 0; i < num; i++) {
        SolverConf conf = solvers->at(i)->getConf();
        if (i >= 1) {
            conf.verbosity = 0;
            conf.doSQL = 0;
            conf.doFindXors = 0;
        }
        solvers->at(i)->setConf(conf);
        solvers->at(i)->set_shared_data((SharedData*)shared_data, i);
    }
}

static void one_thread_add_cls(
    topass data
) {
    vector<Lit> lits;
    bool ret = true;
    size_t at = 0;
    Solver* solver = data.solvers->at(data.tid);
    const vector<Lit>& orig_lits = (*data.lits_to_add);
    const size_t size = orig_lits.size();
    while(at < size && ret) {
        lits.clear();
        for(; at < size && orig_lits[at] != lit_Undef
            ; at++
        ) {
            lits.push_back(orig_lits[at]);
        }
        at++;
        ret = solver->add_clause_outer(lits);
    }

    if (!ret) {
        data.update_mutex->lock();
        *data.ret = l_False;
        data.update_mutex->unlock();
    }
}

bool SATSolver::actually_add_clauses_to_threads()
{
    if (cls_lits.empty())
        return true;

    MY_SOLVERS

    topass data;
    data.solvers = solvers;
    data.lits_to_add = &cls_lits;
    data.update_mutex = new mutex;
    data.ret = new lbool;
    *data.ret = l_True;

    std::vector<std::thread> thds;
    for(size_t i = 0; i < solvers->size(); i++) {
        data.tid = i;
        thds.push_back(thread(one_thread_add_cls, data));
    }
    for(std::thread& thread : thds){
        thread.join();
    }
    delete data.update_mutex;
    bool ret = (*data.ret == l_True);
    delete data.ret;
    cls_lits.clear();

    return ret;
}

bool SATSolver::add_clause(const vector< Lit >& lits)
{
    MY_SOLVERS
    bool ret = true;
    if (solvers->size() > 1) {
        for(Lit lit: lits) {
            cls_lits.push_back(lit);
        }
        cls_lits.push_back(lit_Undef);

        if (cls_lits.size() > CACHE_SIZE) {
            ret = actually_add_clauses_to_threads();
            cls++;
            if (cls % 10 == 9) {
                check_over_mem_limit();
            }
        }
    } else {
        assert(solvers->size() == 1);
        ret = solvers->at(0)->add_clause_outer(lits);
        cls++;
    }

    return ret;
}

bool SATSolver::add_xor_clause(const std::vector<unsigned>& vars, bool rhs)
{
    MY_SOLVERS
    bool ret = true;
    for(size_t i = 0; i < solvers->size(); i++) {
        ret = solvers->at(i)->add_xor_clause_outer(vars, rhs);
    }
    return ret;
}

static void one_thread_solve(
    topass data
) {
    if (false) {
        data.update_mutex->lock();
        cout << "Starting thread" << data.tid << endl;
        data.update_mutex->unlock();
    }

    one_thread_add_cls(data);
    lbool ret = data.solvers->at(data.tid)->solve_with_assumptions(data.assumptions);

    if (false) {
        data.update_mutex->lock();
        cout << "Finished tread " << data.tid << " with result: " << ret << endl;
        data.update_mutex->unlock();
    }


    if (ret != l_Undef) {
        data.update_mutex->lock();
        *data.which_solved = data.tid;
        *data.ret = ret;
        for(size_t i = 0; i < data.solvers->size(); i++) {
            if ((int)i == data.tid)
                continue;

            data.solvers->at(i)->set_must_interrupt_asap();
        }
        data.update_mutex->unlock();
    }
    data.solvers->at(data.tid)->unset_must_interrupt_asap();
}

lbool SATSolver::solve(vector< Lit >* assumptions)
{
    MY_SOLVERS
    if (solvers->size() == 1) {
        return solvers->at(0)->solve_with_assumptions(assumptions);
    }

    lbool* ret = new lbool;
    topass data;
    data.solvers = solvers;
    data.assumptions = assumptions;
    data.lits_to_add = &cls_lits;
    data.update_mutex = new mutex;
    data.which_solved = &which_solved;
    data.ret = ret;

    std::vector<std::thread> thds;
    for(size_t i = 0; i < solvers->size(); i++) {
        data.tid = i;
        thds.push_back(thread(one_thread_solve, data));
    }
    for(std::thread& thread : thds){
        thread.join();
    }
    cls_lits.clear();

    lbool real_ret = *ret;
    delete ret;
    delete data.update_mutex;

    return real_ret;
}

const vector< lbool >& SATSolver::get_model() const
{
    MY_SOLVERS
    return solvers->at(which_solved)->get_model();
}

const std::vector<Lit>& SATSolver::get_conflict() const
{
    MY_SOLVERS
    return solvers->at(which_solved)->get_final_conflict();
}

uint32_t SATSolver::nVars() const
{
    MY_SOLVERS
    return solvers->at(0)->nVarsOutside();
}

void SATSolver::new_var()
{
    MY_SOLVERS
    for(size_t i = 0; i < solvers->size(); i++) {
        solvers->at(i)->new_external_var();
    }

    if (nVars() % 100000 == 99999) {
        check_over_mem_limit();
    }
}

void SATSolver::check_over_mem_limit()
{
    MY_SOLVERS
    double usedGB = ((double)memUsedTotal()) / (1024.0*1024.0*1024.0);
    if (usedGB > 5 && solvers->size() > 1) {
        const size_t newsz = solvers->size() >> 1;
        cout
        << "c After " << (nVars()/1000) << "K vars"
        << " used mem: " << std::fixed << std::setprecision(2) << usedGB << " GB"
        << " deleting 50% threads. New num th: " << newsz << endl;

        for(size_t i = newsz; i < solvers->size(); i++) {
            delete solvers->at(i);
        }
        solvers->resize(newsz);
        ((SharedData*)shared_data)->num_threads = solvers->size();
    }
}

void SATSolver::new_vars(const size_t n)
{
    MY_SOLVERS
    for(size_t d = 0; d < n/500000ULL; d++) {
        for(size_t i = 0; i < solvers->size(); i++) {
            solvers->at(i)->new_external_vars((d+1)*500000ULL);
        }
        check_over_mem_limit();
    }
    for(size_t i = 0; i < solvers->size(); i++) {
        solvers->at(i)->new_external_vars(n % 100000ULL);
    }
}

void SATSolver::add_sql_tag(const std::string& tagname, const std::string& tag)
{
    MY_SOLVERS
    for(size_t i = 0; i < solvers->size(); i++) {
        solvers->at(i)->add_sql_tag(tagname, tag);
    }
}

const char* SATSolver::get_version()
{
    return Solver::getVersion();
}

void SATSolver::print_stats() const
{
    MY_SOLVERS
    solvers->at(which_solved)->printStats();
}

void SATSolver::set_drup(std::ostream* os)
{
    MY_SOLVERS
    assert(solvers->size() == 1);
    DrupFile* drup = new DrupFile();
    drup->setFile(os);
    solvers->at(0)->drup = drup;
}

void SATSolver::interrupt_asap()
{
    MY_SOLVERS
    for(size_t i = 0; i < solvers->size(); i++) {
        solvers->at(i)->set_must_interrupt_asap();
    }
}

void SATSolver::open_file_and_dump_irred_clauses(std::string fname) const
{
    MY_SOLVERS
    solvers->at(which_solved)->open_file_and_dump_irred_clauses(fname);
}

void SATSolver::open_file_and_dump_red_clauses(std::string fname) const
{
    MY_SOLVERS
    solvers->at(which_solved)->open_file_and_dump_red_clauses(fname);
}

void SATSolver::add_in_partial_solving_stats()
{
    MY_SOLVERS
    solvers->at(which_solved)->add_in_partial_solving_stats();
}

std::vector<Lit> SATSolver::get_zero_assigned_lits() const
{
    MY_SOLVERS
    return solvers->at(which_solved)->get_zero_assigned_lits();
}

unsigned long SATSolver::get_sql_id() const
{
    MY_SOLVERS
    return solvers->at(0)->get_sql_id();
}
