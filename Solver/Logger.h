/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>
#include <set>
#include <Vec.h>
#include <vector>

#include "Vec.h"
#include "Heap.h"
#include "Alg.h"
#include "Logger.h"
#include "SolverTypes.h"
#include <string>
#include <map>
#include "stdint.h"
#include "limits.h"

#ifndef uint
#define uint unsigned int
#endif

using std::vector;
using std::pair;
using std::string;
using std::map;

class Solver;

class Logger
{
public:
    Logger(int& vebosity);

    //types of props, confl, and finish
    enum prop_type { revert_guess_type, learnt_unit_clause_type, assumption_type, guess_type, addclause_type, simple_propagation_type, gauss_propagation_type };
    enum confl_type { simple_confl_type, gauss_confl_type };
    enum finish_type { model_found, unsat_model_found, restarting, done_adding_clauses };

    //Conflict and propagation(guess is also a proapgation...)
    void conflict(const confl_type type, uint goback, const uint group, const vec<Lit>& learnt_clause);
    void propagation(const Lit lit, const prop_type type, const uint group = UINT_MAX);
    void empty_clause(const uint group);

    //functions to add/name variables
    void new_var(const Var var);
    void set_variable_name(const uint var, const char* name);

    //functions to add/name clause groups
    void new_group(const uint group);
    void set_group_name(const uint group, const char* name);

    void begin();
    void end(const finish_type finish);
    void print_general_stats(uint restarts, uint64_t conflicts, int vars, int noClauses, uint64_t clauses_Literals, int noLearnts, double litsPerLearntCl, double progressEstimate) const;

    void newclause(const vec<Lit>& ps, const bool xor_clause, const uint group);

    bool proof_graph_on;
    bool statistics_on;
    
    void setSolver(const Solver* solver);
private:
    void print_groups(const vector<pair<uint, uint> >& to_print) const;
    void print_groups(const vector<pair<double, uint> >& to_print) const;
    void print_vars(const vector<pair<uint, uint> >& to_print) const;
    void print_vars(const vector<pair<double, uint> >& to_print) const;
    void print_times_var_guessed() const;
    void print_times_group_caused_propagation() const;
    void print_times_group_caused_conflict() const;
    void print_branch_depth_distrib() const;
    void print_learnt_clause_distrib() const;
    void print_leearnt_clause_graph_distrib(const uint maximum, const map<uint, uint>& learnt_sizes) const;
    void print_advanced_stats() const;
    void print_statistics_note() const;

    uint max_print_lines;
    template<class T>
    void print_line(const uint& number, const string& name, const T& value) const;
    void print_header(const string& first, const string& second, const string& third) const;
    void print_footer() const;
    template<class T>
    void print_line(const string& str, const T& num) const;
    void print_simple_line(const string& str) const;
    void print_center_line(const string& str) const;
    
    void print_confl_order() const;
    void print_prop_order() const;
    void print_assign_var_order() const;
    void printstats() const;
    void reset_statistics();

    //internal data structures
    uint uniqueid; //used to store the last unique ID given to a node
    vec<uint> history; //stores the node uniqueIDs
    uint level; //used to know the current level
    uint begin_level;

    //graph drawing
    FILE* proof; //The file to store the proof
    uint proof_num;
    char filename0[80];
    uint runid;
    uint proof0_lastid;

    //---------------------
    //statistics collection
    //---------------------

    //group and var names
    vector<string> groupnames;
    vector<string> varnames;

    //confls and props grouped by clause groups
    vector<uint> confls_by_group;
    vector<uint> props_by_group;

    //props and guesses grouped by vars
    vector<uint> times_var_guessed;
    vector<uint> times_var_propagated;

    vector<uint> times_group_caused_conflict;
    vector<uint> times_group_caused_propagation;

    vector<vector<uint> > depths_of_propagations_for_group;
    vector<vector<uint> > depths_of_conflicts_for_group;
    vector<vector<uint> > depths_of_assigns_for_var;

    //the distribution of branch depths. first = depth, second = number of occurances
    map<uint, uint> branch_depth_distrib;

    uint sum_conflict_depths;
    uint no_conflicts;
    uint no_decisions;
    uint no_propagations;
    vec<uint> decisions;
    vec<uint> propagations;
    uint sum_decisions_on_branches;
    uint sum_propagations_on_branches;

    //message display properties
    const int& verbosity;
    
    const Solver* solver;
};

#endif //__LOGGER_H__
