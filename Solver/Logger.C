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

#include <time.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
using std::cout;
using std::endl;
using std::ofstream;

#include "Logger.h"
#include "fcopy.h"
#include "SolverTypes.h"

#define MAX_VAR 1000000

Logger::Logger(int& _verbosity) :
        proof_graph_on(false),
        statistics_on(false),

        uniqueid(1),
        level(0),
        begin_level(0),
        max_group(0),

        proof(NULL),
        proof_num(0),

        sum_conflict_depths(0),
        no_conflicts(0),
        no_decisions(0),
        no_propagations(0),
        sum_decisions_on_branches(0),
        sum_propagations_on_branches(0),

        verbosity(_verbosity)
{
    runid /= 10;
    runid=time(NULL)%10000;
    if (verbosity >= 1) printf("RunID is: #%d\n",runid);

    sprintf(filename0,"proofs/%d-proof0.dot", runid);
}

// Adds a new variable to the knowledge of the logger
void Logger::new_var(const Var var)
{
    assert(var < MAX_VAR);

    if (varnames.size() <= var) {
        varnames.resize(var+1);
        times_var_propagated.resize(var+1);
        times_var_guessed.resize(var+1);
        depths_of_assigns_for_var.resize(var+1);
    }
}

// Resizes the groupnames and other, related vectors to accomodate for a new group
void Logger::new_group(const uint group)
{
    if (groupnames.size() <= group) {
        uint old_size = times_group_caused_propagation.size();
        groupnames.resize(group+1);
        times_group_caused_conflict.resize(group+1);
        times_group_caused_propagation.resize(group+1);
        depths_of_propagations_for_group.resize(group+1);
        depths_of_conflicts_for_group.resize(group+1);
        for (uint i = old_size; i < times_group_caused_propagation.size(); i++) {
            times_group_caused_propagation[i] = 0;
            times_group_caused_conflict[i] = 0;
        }
    }

    max_group = std::max(group, max_group);
}

// Adds the new clause group's name to the information stored
void Logger::set_group_name(const uint group, const char* name)
{
    new_group(group);

    if (groupnames[group].empty()) {
        groupnames[group] = name;
    } else if (groupnames[group] != name) {
        printf("Error! Group no. %d has been named twice. First, as '%s', then second as '%s'. Name the same group the same always, or don't give a name to the second iteration of the same group (i.e just write 'c g groupnumber' on the line\n", group, groupnames[group].c_str(), name);
        exit(-1);
    }
}

// sets the variable's name
void Logger::set_variable_name(const uint var, const char* name)
{
    if (!proof_graph_on && !statistics_on) return;

    new_var(var);
    varnames[var] = name;
}

void Logger::begin()
{
    char filename[80];
    sprintf(filename, "proofs/%d-proof%d.dot", runid, proof_num);

    if (proof_num > 0) {
        if (proof_graph_on) {
            FileCopy(filename0, filename);
            proof = fopen(filename,"a");
            if (!proof) printf("Couldn't open proof file '%s' for writing\n", filename), exit(-1);
        }
    } else {
        history.growTo(10);
        history[level] = uniqueid;

        if (proof_graph_on) {
            proof = fopen(filename,"w");
            if (!proof) printf("Couldn't open proof file '%s' for writing\n", filename), exit(-1);
            fprintf(proof, "digraph G {\n");
            fprintf(proof,"node%d [shape=circle, label=\"BEGIN\"];\n", uniqueid);
        }
    }

    if (statistics_on) reset_statistics();

    level = begin_level;
}

// For noting conflicts. Updates the proof graph and the statistics.
void Logger::conflict(const confl_type type, uint goback, const uint group, const vec<Lit>& learnt_clause)
{
    assert(!(proof == NULL && proof_graph_on));
    assert(goback < level);

    goback += begin_level;
    uniqueid++;

    if (proof_graph_on) {
        fprintf(proof,"node%d [shape=polygon,sides=5,label=\"",uniqueid);
        for (int i = 0; i < learnt_clause.size(); i++) {
            if (learnt_clause[i].sign()) fprintf(proof,"-");
            int myvar = learnt_clause[i].var();
            if (varnames.size() <= myvar || varnames[myvar].empty())
                fprintf(proof,"%d\\n",myvar+1);
            else fprintf(proof,"%s\\n",varnames[myvar].c_str());
        }

        fprintf(proof,"\"];\n");

        fprintf(proof,"node%d -> node%d [label=\"",history[level],uniqueid);

        if (type == gauss_confl_type) {
            fprintf(proof,"Gauss\",style=bold");
        } else if (group > max_group) fprintf(proof,"**%d\"",group);
        else {
            if (groupnames.size() <= group || groupnames[group].empty())
                fprintf(proof,"%d\"", group);
            else fprintf(proof,"%s\"", groupnames[group].c_str());
        }

        fprintf(proof,"];\n");
        fprintf(proof,"node%d -> node%d [style=bold];\n",uniqueid,history[goback]);
    }

    if (statistics_on) {
        const uint depth = level - begin_level;

        if (group < max_group) { //TODO make work for learnt clauses
            times_group_caused_conflict[group]++;
            depths_of_conflicts_for_group[group].push_back(depth);
        }
        no_conflicts++;
        sum_conflict_depths += depth;
        sum_decisions_on_branches += decisions[depth];
        sum_propagations_on_branches += propagations[depth];
        branch_depth_distrib[depth]++;
    }

    level = goback;
}

// For the really strange event that the solver is given an empty clause
void Logger::empty_clause(const uint group)
{
    assert(!(proof == NULL && proof_graph_on));

    if (proof_graph_on) {
        fprintf(proof,"node%d -> node%d [label=\"emtpy clause:",history[level],uniqueid+1);
        if (group > max_group) fprintf(proof,"**%d\\n",group);
        else {
            if (groupnames.size() <= group || groupnames[group].empty())
                fprintf(proof,"%d\\n", group);
            else fprintf(proof,"%s\\n", groupnames[group].c_str());
        }

        fprintf(proof,"\"];\n");
    }
}

// Propagating a literal. Type of literal and the (learned clause's)/(propagating clause's)/(etc) group must be given. Updates the proof graph and the statistics. note: the meaning of the variable 'group' depends on the type
void Logger::propagation(const Lit lit, const prop_type type, const uint group)
{
    assert(!(proof == NULL && proof_graph_on));
    uniqueid++;

    //graph
    if (proof_graph_on) {
        fprintf(proof,"node%d [shape=box, label=\"",uniqueid);;
        if (lit.sign()) fprintf(proof,"-");
        if (varnames.size() <= lit.var() || varnames[lit.var()].empty())
            fprintf(proof,"%d\"];\n",lit.var()+1);
        else fprintf(proof,"%s\"];\n",varnames[lit.var()].c_str());

        fprintf(proof,"node%d -> node%d [label=\"",history[level],uniqueid);
        switch (type) {

        case revert_guess_type:
        case simple_propagation_type:
            assert(group != UINT_MAX);
            if (group > max_group) fprintf(proof,"**%d\\n",group);
            else {
                if (groupnames.size() <= group || groupnames[group].empty())
                    fprintf(proof,"%d\\n", group);
                else fprintf(proof,"%s\\n", groupnames[group].c_str());
            }

            fprintf(proof,"\"];\n");
            break;

        case gauss_propagation_type:
            fprintf(proof,"Gauss\",style=bold];\n");
            break;

        case learnt_unit_clause_type:
            fprintf(proof,"learnt unit clause\",style=bold];\n");
            break;

        case assumption_type:
            fprintf(proof,"assumption\"];\n");
            break;

        case guess_type:
            fprintf(proof,"guess\",style=dotted];\n");
            break;

        case addclause_type:
            assert(group != UINT_MAX);
            if (groupnames.size() <= group || groupnames[group].empty())
                fprintf(proof,"red. from %d\"];\n",group);
            else fprintf(proof,"red. from %s\"];\n",groupnames[group].c_str());
            break;
        }
    }

    if (statistics_on && proof_num > 0) switch (type) {
        case gauss_propagation_type:
        case simple_propagation_type:
            no_propagations++;
            times_var_propagated[lit.var()]++;
            if (group < max_group) { //TODO make work for learnt clauses
                depths_of_propagations_for_group[group].push_back(level - begin_level);
                times_group_caused_propagation[group]++;
            }
            depths_of_assigns_for_var[lit.var()].push_back(level - begin_level);
            break;

        case learnt_unit_clause_type: //when learning unit clause
        case revert_guess_type: //when, after conflict, a guess gets reverted
            if (group < max_group) { //TODO make work for learnt clauses
                times_group_caused_propagation[group]++;
                depths_of_propagations_for_group[group].push_back(level - begin_level);
            }
            depths_of_assigns_for_var[lit.var()].push_back(level - begin_level);
        case guess_type:
            times_var_guessed[lit.var()]++;
            depths_of_assigns_for_var[lit.var()].push_back(level - begin_level);
            no_decisions++;
            break;

        case addclause_type:
        case assumption_type:
            assert(false);
        }

    level++;

    if (proof_num > 0) {
        decisions.growTo(level-begin_level+1);
        propagations.growTo(level-begin_level+1);
        if (level-begin_level == 1) {
            decisions[0] = 0;
            propagations[0] = 0;
            //note: we might reach this place TWICE in the same restart. This is because the first assignement might get reverted
        }
        if (type == simple_propagation_type) {
            decisions[level-begin_level] = decisions[level-begin_level-1];
            propagations[level-begin_level] = propagations[level-begin_level-1]+1;
        } else {
            decisions[level-begin_level] = decisions[level-begin_level-1]+1;
            propagations[level-begin_level] = propagations[level-begin_level-1];
        }
    }

    if (history.size() < level+1) history.growTo(level+10);
    history[level] = uniqueid;
}

// Ending of a restart iteration. Also called when ending S.simplify();
void Logger::end(const finish_type finish)
{
    assert(!(proof == NULL && proof_graph_on));

    switch (finish) {
    case model_found: {
        uniqueid++;
        if (proof_graph_on) fprintf(proof,"node%d [shape=doublecircle, label=\"MODEL\"];\n",uniqueid);
        break;
    }
    case unsat_model_found: {
        uniqueid++;
        if (proof_graph_on) fprintf(proof,"node%d [shape=doublecircle, label=\"UNSAT\"];\n",uniqueid);
        break;
    }
    case restarting: {
        uniqueid++;
        if (proof_graph_on) fprintf(proof,"node%d [shape=doublecircle, label=\"Re-starting\\nsearch\"];\n",uniqueid);
        break;
    }
    case done_adding_clauses: {
        begin_level = level;
        break;
    }
    }

    if (proof_graph_on) {
        if (proof_num > 0) {
            fprintf(proof,"node%d -> node%d;\n",history[level],uniqueid);
            fprintf(proof,"}\n");
        } else proof0_lastid = uniqueid;

        proof = (FILE*)fclose(proof);
        assert(proof == NULL);

        if (finish == model_found || finish == unsat_model_found) {
            proof = fopen(filename0,"a");
            fprintf(proof,"node%d [shape=doublecircle, label=\"Done adding\\nclauses\"];\n",proof0_lastid+1);
            fprintf(proof,"node%d -> node%d;\n",proof0_lastid,proof0_lastid+1);
            fprintf(proof,"}\n");
            proof = (FILE*)fclose(proof);
            assert(proof == NULL);
        }
    }

    if (statistics_on) printstats();

    proof_num++;
}

void Logger::print_assign_var_order() const
{
    vector<pair<double, uint> > prop_ordered;
    for (uint i = 0; i < depths_of_assigns_for_var.size(); i++) {
        double avg = 0.0;
        for (vector<uint>::const_iterator it = depths_of_assigns_for_var[i].begin(); it != depths_of_assigns_for_var[i].end(); it++)
            avg += *it;
        if (depths_of_assigns_for_var[i].size() > 0) {
            avg /= (double) depths_of_assigns_for_var[i].size();
            prop_ordered.push_back(std::make_pair(avg, i));
        }
    }

    if (!prop_ordered.empty()) {
        cout << "--- Variables are assigned in the order of: (avg) ---" << endl;
        std::sort(prop_ordered.begin(), prop_ordered.end());
        print_vars(prop_ordered);
    }
}

void Logger::print_prop_order() const
{
    vector<pair<double, uint> > prop_ordered;
    for (uint i = 0; i < depths_of_propagations_for_group.size(); i++) {
        double avg = 0.0;
        for (vector<uint>::const_iterator it = depths_of_propagations_for_group[i].begin(); it != depths_of_propagations_for_group[i].end(); it++)
            avg += *it;
        if (depths_of_propagations_for_group[i].size() > 0) {
            avg /= (double) depths_of_propagations_for_group[i].size();
            prop_ordered.push_back(std::make_pair(avg, i));
        }
    }

    if (!prop_ordered.empty()) {
        cout << "--- Propagation depth order of clause groups ---" << endl;
        std::sort(prop_ordered.begin(), prop_ordered.end());
        print_groups(prop_ordered);
    }
}

void Logger::print_confl_order() const
{
    vector<pair<double, uint> > confl_ordered;
    for (uint i = 0; i < depths_of_conflicts_for_group.size(); i++) {
        double avg = 0.0;
        for (vector<uint>::const_iterator it = depths_of_conflicts_for_group[i].begin(); it != depths_of_conflicts_for_group[i].end(); it++)
            avg += *it;
        if (depths_of_conflicts_for_group[i].size() > 0) {
            avg /= (double) depths_of_conflicts_for_group[i].size();
            confl_ordered.push_back(std::make_pair(avg, i));
        }
    }

    if (!confl_ordered.empty()) {
        cout << "--- Avg. conflict depth order of clause groups ---" << endl;
        std::sort(confl_ordered.begin(), confl_ordered.end());
        print_groups(confl_ordered);
    }
}


void Logger::print_times_var_guessed() const
{
    vector<pair<uint, uint> > times_var_ordered;
    for (int i = 0; i < varnames.size(); i++) if (times_var_guessed[i] > 0)
            times_var_ordered.push_back(std::make_pair(times_var_guessed[i], i));

    if (!times_var_ordered.empty()) {
        cout << "--- Times var guessed ---" << endl;
        std::sort(times_var_ordered.rbegin(), times_var_ordered.rend());
        print_vars(times_var_ordered);
    }
}

void Logger::print_times_group_caused_propagation() const
{
    vector<pair<uint, uint> > props_group_ordered;
    for (uint i = 0; i < times_group_caused_propagation.size(); i++)
        if (times_group_caused_propagation[i] > 0)
            props_group_ordered.push_back(std::make_pair(times_group_caused_propagation[i], i));

    if (!props_group_ordered.empty()) {
        cout << "--- Number of propagations made by clause groups ---" << endl;
        std::sort(props_group_ordered.rbegin(),props_group_ordered.rend());
        print_groups(props_group_ordered);
    }
}

void Logger::print_times_group_caused_conflict() const
{
    vector<pair<uint, uint> > confls_group_ordered;
    for (uint i = 0; i < times_group_caused_conflict.size(); i++)
        if (times_group_caused_conflict[i] > 0)
            confls_group_ordered.push_back(std::make_pair(times_group_caused_conflict[i], i));

    if (!confls_group_ordered.empty()) {
        cout << "--- Number of conflicts made by clause groups ---" << endl;
        std::sort(confls_group_ordered.rbegin(), confls_group_ordered.rend());
        print_groups(confls_group_ordered);
    }
}

void Logger::print_groups(const vector<pair<double, uint> >& to_print) const
{
    bool no_name = true;
    typedef vector<pair<double, uint> >::const_iterator myiterator;
    for (myiterator it = to_print.begin(); it != to_print.end(); it++) {
        /*if (it->first <= 1) {
        printf("Skipped all frequencies <= %d\n", it->first);
        break;
    }*/
        if (it->second > max_group) {
            cout << "group " << it->second+1 << " ( learnt clause ) : " << it->first << endl;
        } else {
            if (!groupnames[it->second].empty())
                no_name = false;
            cout << "group " << it->second+1 << " ( " << groupnames[it->second]<< " ) : " << it->first << endl;
        }
    }
    if (no_name) printf("Tip: You can name your clauses using the syntax \"c g 32 Blah blah\" in your DIMACS file, where 32 is the clause group number, which can be shared between clauses (note: if the clause group matches, the name must match, too).\n");
}

void Logger::print_groups(const vector<pair<uint, uint> >& to_print) const
{
    bool no_name = true;
    typedef vector<pair<uint, uint> >::const_iterator myiterator;
    for (myiterator it = to_print.begin(); it != to_print.end(); it++) {
        /*if (it->first <= 1) {
        	printf("Skipped all frequencies <= %d\n", it->first);
        	break;
        }*/
        if (it->second > max_group) {
            cout << "group " << it->second+1 << " ( learnt clause ) : " << it->first << endl;
        } else {
            if (!groupnames[it->second].empty())
                no_name = false;
            cout << "group " << it->second+1 << " ( " << groupnames[it->second]<< " ) : " << it->first << endl;
        }
    }
    if (no_name) printf("Tip: You can name your clauses using the syntax \"c g 32 Blah blah\" in your DIMACS file, where 32 is the clause group number, which can be shared between clauses (note: if the clause group matches, the name must match, too).\n");
}

void Logger::print_vars(const vector<pair<double, uint> >& to_print) const
{
    bool no_name = true;
    for (vector<pair<double, uint> >::const_iterator it = to_print.begin(); it != to_print.end(); it++) {
        /*if (it->first <= 1) {
        printf("Skipped all frequencies <= %d\n", it->first);
        break;
    }*/
        if (!varnames[it->second].empty())
            no_name = false;
        cout << "var " << it->second+1 << " ( " << varnames[it->second] << " ) : " << it->first << endl;
    }
    if (no_name) printf("Tip: You can name your variables using the syntax \"c v 10 Blah blah\" in your DIMACS file. It helps.\n");
}

void Logger::print_vars(const vector<pair<uint, uint> >& to_print) const
{
    bool no_name = true;
    for (vector<pair<uint, uint> >::const_iterator it = to_print.begin(); it != to_print.end(); it++) {
        /*if (it->first <= 1) {
        	printf("Skipped all frequencies <= %d\n", it->first);
        	break;
        }*/
        if (!varnames[it->second].empty())
            no_name = false;
        cout << "var " << it->second+1 << " ( " << varnames[it->second] << " ) : " << it->first << endl;
    }
    if (no_name) printf("Tip: You can name your variables using the syntax \"c v 10 Blah blah\" in your DIMACS file. It helps.\n");
}

void Logger::print_branch_depth_distrib() const
{
    cout << "--- Branch depth stats ---" << endl;

    const uint range = 20;
    map<uint, uint> range_stat;

    for (map<uint, uint>::const_iterator it = branch_depth_distrib.begin(); it != branch_depth_distrib.end(); it++) {
        cout << it->first << " : " << it->second << endl;
        range_stat[it->first/range] += it->second;
    }
    cout << endl;

    cout << "--- Branch depth stats ranged to " << range << " ---" << endl;

    std::stringstream ss;
    ss << "branch_depths/branch_depth_file" << runid << "-" << proof_num << ".txt";
    ofstream branch_depth_file;
    branch_depth_file.open(ss.str().c_str());
    uint i = 0;
    
    for (map<uint, uint>::iterator it = range_stat.begin(); it != range_stat.end(); it++) {
        cout << it->first*range << "-" << it->first*range + range-1 << " : " << it->second << endl;

        branch_depth_file << i << "\t" << it->second << "\t";
        //branch_depth_file  << "\"" << lexical_cast<string>(it.first*range) << "-" << lexical_cast<string>(it.first*range + range-1) << "\"" << endl;
        if (i %  5 == 0)
            branch_depth_file  << "\"" << it->first*range << "\"";
        else
            branch_depth_file << "\"\"";
        branch_depth_file << endl;
        i++;
    }
    branch_depth_file.close();
    cout << endl;

}

// Prints statistics on the console
void Logger::printstats() const
{
    assert(statistics_on);
    assert(varnames.size() == times_var_guessed.size());
    assert(varnames.size() == times_var_propagated.size());

    printf("\n----------- STATS FOR THIS RESTART BEGIN ------------\n");
    printf("ATTENTION! If you used minisat as a library, then vars are all shifted by 1 here and everywhere in ALL outputs of minisat. This does not apply when you use minisat as a stand-alone program)\n");

    print_times_var_guessed();
    print_times_group_caused_propagation();
    print_times_group_caused_conflict();
    print_prop_order();
    print_confl_order();
    print_assign_var_order();

    printf("No. branches visited: %d\n", no_conflicts);
    printf("Avg branch depth: %f\n", (double)sum_conflict_depths/(double)no_conflicts);
    printf("No decisions: %d\n", no_decisions);
    printf("No propagations: %d\n", no_propagations);

    //printf("no progatations/no decisions (i.e. one decision gives how many propagations on average *for the whole search graph*): %f\n", (double)no_propagations/(double)no_decisions);
    //printf("no propagations/sum decisions on branches (if you look at one specific branch, what is the average number of propagations you will find?): %f\n", (double)no_propagations/(double)sum_decisions_on_branches);

    printf("sum decisions on branches/no. branches =\n(if you look at one specific branch, what is the average number of decisions you will find?) =\n %f\n", (double)sum_decisions_on_branches/(double)no_conflicts);

    printf("sum propagations on branches/no. branches =\n(if you look at one specific branch, what is the average number of propagations you will find?) =\n %f\n", (double)sum_propagations_on_branches/(double)no_conflicts);

    print_branch_depth_distrib();

    printf("\n----------- STATS END ------------\n\n");
}

// resets all stored statistics. Might be useful, to generate statistics for each restart and not for the whole search in general
void Logger::reset_statistics()
{
    assert(times_var_guessed.size() == times_var_propagated.size());
    assert(times_group_caused_conflict.size() == times_group_caused_propagation.size());
    
    typedef vector<uint>::iterator vecit;
    for (vecit it = times_var_guessed.begin(); it != times_var_guessed.end(); it++)
        *it = 0;

    for (vecit it = times_var_propagated.begin(); it != times_var_propagated.end(); it++)
        *it = 0;

    for (vecit it = times_group_caused_conflict.begin(); it != times_group_caused_conflict.end(); it++)
        *it = 0;

    for (vecit it = times_group_caused_propagation.begin(); it != times_group_caused_propagation.end(); it++)
        *it = 0;

    for (vecit it = confls_by_group.begin(); it != confls_by_group.end(); it++)
        *it = 0;

    for (vecit it = props_by_group.begin(); it != props_by_group.end(); it++)
        *it = 0;

    typedef vector<vector<uint> >::iterator vecvecit;

    for (vecvecit it = depths_of_propagations_for_group.begin(); it != depths_of_propagations_for_group.end(); it++)
        it->clear();

    for (vecvecit it = depths_of_conflicts_for_group.begin(); it != depths_of_conflicts_for_group.end(); it++)
        it->clear();

    for (vecvecit it = depths_of_assigns_for_var.begin(); it != depths_of_assigns_for_var.end(); it++)
        it->clear();

    sum_conflict_depths = 0;
    no_conflicts = 0;
    no_decisions = 0;
    no_propagations = 0;
    decisions.clear();
    propagations.clear();
    sum_decisions_on_branches = 0;
    sum_propagations_on_branches = 0;
    branch_depth_distrib.clear();
}
