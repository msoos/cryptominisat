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

#define MY_SOLVERS vector<Solver*>* solvers = (vector<Solver*>*)s;

using namespace CMSat;

SATSolver::SATSolver(const SolverConf conf, bool* interrupt_asap)
{
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
        std::cerr << "Number of threads must be at least 1" << endl;
        exit(-1);
    }
    if (num == 1)
        return;

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
                conf.simplify_at_startup = 1;
                conf.propBinFirst = 1;
                conf.doLHBR = 1;
                conf.increaseClean = 1.12;
                conf.ratioRemoveClauses = 0.7;
                break;
            }
            case 3: {
                conf.doVarElim = 0;
                conf.numCleanBetweenSimplify = 3;
                conf.shortTermHistorySize = 80;
                conf.clauseCleaningType = CMSat::clean_glue_based;
                conf.restartType = CMSat::restart_type_glue;
                conf.increaseClean = 1.08;
                conf.ratioRemoveClauses = 0.55;
                break;
            }
            case 4: {
                conf.doGateFind = 0;
                conf.more_red_minim_limit_cache = 400;
                conf.more_red_minim_limit_binary = 200;
                conf.probe_bogoprops_timeoutM = 3500;
                conf.restartType = CMSat::restart_type_agility;
                conf.ratioRemoveClauses = 0.6;
                break;
            }
            case 5: {
                conf.simplify_at_startup = 1;
                conf.regularly_simplify_problem = 0;
                conf.varElimRatioPerIter = 1;
                conf.restartType = restart_type_geom;
                conf.clauseCleaningType = CMSat::clean_sum_activity_based;
                conf.polarity_mode = CMSat::polarmode_neg;
                conf.ratioRemoveClauses = 0.65;
                break;
            }
            case 6: {
                conf.doGateFind = 0;
                conf.more_red_minim_limit_cache = 100;
                conf.more_red_minim_limit_binary = 100;
                conf.probe_bogoprops_timeoutM = 4000;
                conf.ratioRemoveClauses = 0.6;
                break;
            }
            case 7: {
                conf.clauseCleaningType = CMSat::clean_sum_confl_depth_based;
                conf.ratioRemoveClauses = 0.55;
                break;
            }
            default: {
                conf.ratioRemoveClauses = 0.7;
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

bool SATSolver::add_clause(const vector< Lit >& lits)
{
    MY_SOLVERS
    bool ret = true;
    for(size_t i = 0; i < solvers->size(); i++) {
        ret = solvers->at(i)->add_clause_outer(lits);
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

struct topass
{
    vector<Solver*> *solvers;
    vector<Lit> *assumptions;
    mutex* update_mutex;
    int tid;
    int *which_solved;
    lbool* ret;
};

static void one_thread(
    topass data
) {
    data.update_mutex->lock();
    //cout << "Starting thread" << data.tid << endl;
    data.update_mutex->unlock();

    lbool ret = data.solvers->at(data.tid)->solve_with_assumptions(data.assumptions);
    data.update_mutex->lock();
    //cout << "Finished tread " << data.tid << " with result: " << ret << endl;
    data.update_mutex->unlock();


    if (ret != l_Undef) {
        data.update_mutex->lock();
        *data.which_solved = data.tid;
        *data.ret = ret;
        for(size_t i = 0; i < data.solvers->size(); i++) {
            if (i == data.tid)
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
    data.update_mutex = new mutex;
    data.which_solved = &which_solved;
    data.ret = ret;

    std::vector<std::thread> thds;
    for(size_t i = 0; i < solvers->size(); i++) {
        data.tid = i;
        thds.push_back(thread(one_thread, data));
    }
    for(auto& thread : thds){
        thread.join();
    }
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
