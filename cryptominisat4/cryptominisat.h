/*
 * This library header is under MIT license but the library itself is under
 * LGPLv2 license. This means you can link to this library as you wish in
 * proprietary code. Please read the LGPLv2 license for details.
*/

#ifndef __CRYPTOMINISAT_H__
#define __CRYPTOMINISAT_H__

#include <vector>
#include <iostream>
#include "cryptominisat4/solverconf.h"
#include "cryptominisat4/solvertypesmini.h"

namespace CMSat {
    class SATSolver
    {
    public:
        SATSolver(SolverConf conf = SolverConf(), bool* interrupt_asap = NULL);
        ~SATSolver();
        void set_num_threads(unsigned n);
        unsigned nVars() const;
        bool add_clause(const std::vector<Lit>& lits);
        bool add_xor_clause(const std::vector<unsigned>& vars, bool rhs);
        void new_var();
        void new_vars(const size_t n);
        lbool solve(std::vector<Lit>* assumptions = 0);
        const std::vector<lbool>& get_model() const;
        const std::vector<Lit>& get_conflict() const;
        void add_sql_tag(const std::string& tagname, const std::string& tag);
        unsigned long get_sql_id() const;
        bool okay() const;

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
        void *s;
    };
}

#endif //__CRYPTOMINISAT_H__
