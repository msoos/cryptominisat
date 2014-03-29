#ifndef __CRYPTOMINISAT_H__
#define __CRYPTOMINISAT_H__

#include <vector>
#include <iostream>
#include "cryptominisat3/solverconf.h"
#include "cryptominisat3/solvertypesmini.h"

namespace CMSat {
    class SATSolver
    {
    public:
        SATSolver(SolverConf conf = SolverConf());
        ~SATSolver();
        uint32_t nVars() const;
        bool add_clause(const std::vector<Lit>& lits);
        void new_var();
        lbool solve(std::vector<Lit>* assumptions = 0);
        const std::vector<lbool>& get_model() const;
        const std::vector<Lit>& get_conflict() const;
        const std::vector<Lit>& get_unitary_clauses() const;
        void add_sql_tag(const std::string& tagname, const std::string& tag);
        unsigned long get_sql_id() const;

        SolverConf get_conf() const;
        static const char* get_version();
        void print_stats() const;
        void set_drup(std::ostream* os);
        void interrupt_asap();
        void open_file_and_dump_irred_clauses(std::string fname) const;
        void open_file_and_dump_red_clauses(std::string fname) const;
        void add_in_partial_solving_stats();
        std::vector<Lit> get_zero_assigned_lits() const;
    private:
        void* solver;
    };
}

#endif //__CRYPTOMINISAT_H__
