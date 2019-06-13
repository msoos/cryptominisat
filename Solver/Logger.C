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
#include <iomanip>
#include <fstream>
#include <sstream>
using std::cout;
using std::endl;
using std::ofstream;

#include "Logger.h"
#include "fcopy.h"
#include "SolverTypes.h"
#include "Solver.h"

#define FST_WIDTH 10
#define SND_WIDTH 35
#define TRD_WIDTH 10

Logger::Logger(int& _verbosity) :
        proof_graph_on(false),
        statistics_on(false),

        max_print_lines(20),
        uniqueid(1),
        level(0),
        begin_level(0),

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
        groupnames.resize(group+1, "Noname");
        times_group_caused_conflict.resize(group+1, 0);
        times_group_caused_propagation.resize(group+1, 0);
        depths_of_propagations_for_group.resize(group+1);
        depths_of_conflicts_for_group.resize(group+1);
    }
}

// Adds the new clause group's name to the information stored
void Logger::set_group_name(const uint group, const char* name)
{
    new_group(group);

    if (strlen(name) > SND_WIDTH-2) {
        cout << "A clause group name cannot have more than " << SND_WIDTH-2 << " number of characters. You gave '" << name << "', which is " << strlen(name) << " long." << endl;
        exit(-1);
    }
    
    if (groupnames[group].empty() || groupnames[group] == "Noname") {
        groupnames[group] = name;
    } else if (name[0] != '\0' && groupnames[group] != name) {
        printf("Error! Group no. %d has been named twice. First, as '%s', then second as '%s'. Name the same group the same always, or don't give a name to the second iteration of the same group (i.e just write 'c g groupnumber' on the line\n", group, groupnames[group].c_str(), name);
        exit(-1);
    }
}

// sets the variable's name
void Logger::set_variable_name(const uint var, const char* name)
{
    if (!proof_graph_on && !statistics_on) return;
    
    if (strlen(name) > SND_WIDTH-2) {
        cout << "A variable name cannot have more than " << SND_WIDTH-2 << " number of characters. You gave '" << name << "', which is " << strlen(name) << " long." << endl;
        exit(-1);
    }

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
            fprintf(proof,"node%d [shape=circle, label=\"BEGIN\", root];\n", uniqueid);
        }
    }

    if (statistics_on)
        reset_statistics();

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
        } else {
            if (groupnames.size() <= group || groupnames[group].empty())
                fprintf(proof,"%d\"", group);
            else fprintf(proof,"%s\"", groupnames[group].c_str());
        }

        fprintf(proof,"];\n");
        fprintf(proof,"node%d -> node%d [style=bold];\n",uniqueid,history[goback]);
    }

    if (statistics_on) {
        const uint depth = level - begin_level;

        times_group_caused_conflict[group]++;
        depths_of_conflicts_for_group[group].push_back(depth);
        
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
        fprintf(proof,"%s\\n", groupnames[group].c_str());
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
            fprintf(proof,"%s\\n", groupnames[group].c_str());
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
            
            depths_of_propagations_for_group[group].push_back(level - begin_level);
            times_group_caused_propagation[group]++;
            
            depths_of_assigns_for_var[lit.var()].push_back(level - begin_level);
            break;

        case learnt_unit_clause_type: //when learning unit clause
        case revert_guess_type: //when, after conflict, a guess gets reverted
            assert(group != UINT_MAX);
            times_group_caused_propagation[group]++;
            depths_of_propagations_for_group[group].push_back(level - begin_level);
            
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

void Logger::print_footer() const
{
    cout << "+" << std::setfill('-') << std::setw(FST_WIDTH+SND_WIDTH+TRD_WIDTH+4) << "-" << std::setfill(' ') << "+" << endl;
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
        print_footer();
        print_simple_line(" Variables are assigned in the following order");
        print_header("var", "var name", "avg order");
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
        print_footer();
        print_simple_line(" Propagation depth order of clause groups");
        print_header("group", "group name", "avg order");
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
        print_footer();
        print_simple_line(" Avg. conflict depth order of clause groups");
        print_header("groupno", "group name", "avg. depth");
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
        print_footer();
        print_simple_line(" No. times variable branched on");
        print_header("var", "var name", "no. times");
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
        print_footer();
        print_simple_line(" No. propagations made by clause groups");
        print_header("group", "group name", "no. props");
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
        print_footer();
        print_simple_line(" No. conflicts made by clause groups");
        print_header("group", "group name", "no. confl");
        std::sort(confls_group_ordered.rbegin(), confls_group_ordered.rend());
        print_groups(confls_group_ordered);
    }
}

template<class T>
void Logger::print_line(const uint& number, const string& name, const T& value) const
{
    cout << "|" << std::setw(FST_WIDTH) << number << "  " << std::setw(SND_WIDTH) << name << "  " << std::setw(TRD_WIDTH) << value << "|" << endl;
}

void Logger::print_header(const string& first, const string& second, const string& third) const
{
    cout << "|" << std::setw(FST_WIDTH) << first << "  " << std::setw(SND_WIDTH) << second << "  " << std::setw(TRD_WIDTH) << third << "|" << endl;
    print_footer();
}

void Logger::print_groups(const vector<pair<double, uint> >& to_print) const
{
    uint i = 0;
    typedef vector<pair<double, uint> >::const_iterator myiterator;
    for (myiterator it = to_print.begin(); it != to_print.end() && i < max_print_lines; it++, i++) {
        print_line(it->second+1, groupnames[it->second], it->first);
    }
    print_footer();
}

void Logger::print_groups(const vector<pair<uint, uint> >& to_print) const
{
    uint i = 0;
    typedef vector<pair<uint, uint> >::const_iterator myiterator;
    for (myiterator it = to_print.begin(); it != to_print.end() && i < max_print_lines; it++, i++) {
        print_line(it->second+1, groupnames[it->second], it->first);
    }
    print_footer();
}

void Logger::print_vars(const vector<pair<double, uint> >& to_print) const
{
    uint i = 0;
    for (vector<pair<double, uint> >::const_iterator it = to_print.begin(); it != to_print.end() && i < max_print_lines; it++, i++)
        print_line(it->second+1, varnames[it->second], it->first);
    
    print_footer();
}

void Logger::print_vars(const vector<pair<uint, uint> >& to_print) const
{
    uint i = 0;
    for (vector<pair<uint, uint> >::const_iterator it = to_print.begin(); it != to_print.end() && i < max_print_lines; it++, i++) {
        print_line(it->second+1, varnames[it->second], it->first);
    }
    
    print_footer();
}

template<class T>
void Logger::print_line(const string& str, const T& num) const
{
    cout << "|" << std::setw(FST_WIDTH+SND_WIDTH+4) << str << std::setw(TRD_WIDTH) << num << "|" << endl;
}

void Logger::print_simple_line(const string& str) const
{
    cout << "|" << std::setw(FST_WIDTH+SND_WIDTH+TRD_WIDTH+4) << str << "|" << endl;
}

void Logger::print_center_line(const string& str) const
{
    uint middle = (FST_WIDTH+SND_WIDTH+TRD_WIDTH+4-str.size())/2;
    int rest = FST_WIDTH+SND_WIDTH+TRD_WIDTH+4-middle*2-str.size();
    cout << "|" << std::setw(middle) << " " << str << std::setw(middle + rest) << " " << "|" << endl;
}

void Logger::print_branch_depth_distrib() const
{
    //cout << "--- Branch depth stats ---" << endl;

    const uint range = 20;
    map<uint, uint> range_stat;

    for (map<uint, uint>::const_iterator it = branch_depth_distrib.begin(); it != branch_depth_distrib.end(); it++) {
        //cout << it->first << " : " << it->second << endl;
        range_stat[it->first/range] += it->second;
    }
    //cout << endl;

    print_footer();
    print_simple_line(" No. search branches with branch depth between");
    print_line("Branch depth between", "no. br.-s");
    print_footer();

    std::stringstream ss;
    ss << "branch_depths/branch_depth_file" << runid << "-" << proof_num << ".txt";
    ofstream branch_depth_file;
    branch_depth_file.open(ss.str().c_str());
    uint i = 0;
    
    for (map<uint, uint>::iterator it = range_stat.begin(); it != range_stat.end(); it++) {
        std::stringstream ss2;
        ss2 << it->first*range << " - " << it->first*range + range-1;
        print_line(ss2.str(), it->second);

        if (branch_depth_file.is_open()) {
                branch_depth_file << i << "\t" << it->second << "\t";
            if (i %  5 == 0)
                branch_depth_file  << "\"" << it->first*range << "\"";
            else
                branch_depth_file << "\"\"";
            branch_depth_file << endl;
        }
        i++;
    }
    if (branch_depth_file.is_open())
        branch_depth_file.close();
    print_footer();

}

void Logger::print_learnt_clause_distrib() const
{
    map<uint, uint> learnt_sizes;
    const vec<Clause*>& learnts = solver->get_learnts();
    
    uint maximum = 0;
    
    for (uint i = 0; i < learnts.size(); i++)
    {
        uint size = learnts[i]->size();
        maximum = std::max(maximum, size);
        
        map<uint, uint>::iterator it = learnt_sizes.find(size);
        if (it == learnt_sizes.end())
            learnt_sizes[size] = 1;
        else
            it->second++;
    }
    
    learnt_sizes[0] = solver->get_unitary_learnts().size();
    
    uint slice = (maximum+1)/max_print_lines + (bool)((maximum+1)%max_print_lines);
    
    print_footer();
    print_simple_line(" Learnt clause length distribution");
    print_line("Length between", "no. cl.");
    print_footer();
    
    uint until = slice;
    uint from = 0;
    while(until < maximum+1) {
        std::stringstream ss2;
        ss2 << from << " - " << until-1;
        
        uint sum = 0;
        for (; from < until; from++) {
            map<uint, uint>::const_iterator it = learnt_sizes.find(from);
            if (it != learnt_sizes.end())
                sum += it->second;
        }
        
        print_line(ss2.str(), sum);
        
        until += slice;
    }
    
    print_footer();
    
    print_leearnt_clause_graph_distrib(maximum, learnt_sizes);
}

void Logger::print_leearnt_clause_graph_distrib(const uint maximum, const map<uint, uint>& learnt_sizes) const
{
    uint no_slices = FST_WIDTH  + SND_WIDTH + TRD_WIDTH + 4-3;
    uint slice = (maximum+1)/no_slices + (bool)((maximum+1)%no_slices);
    uint until = slice;
    uint from = 0;
    vector<uint> slices;
    uint hmax = 0;
    while(until < maximum+1) {
        uint sum = 0;
        for (; from < until; from++) {
            map<uint, uint>::const_iterator it = learnt_sizes.find(from);
            if (it != learnt_sizes.end())
                sum += it->second;
        }
        slices.push_back(sum);
        until += slice;
        hmax = std::max(hmax, sum);
    }
    slices.resize(no_slices, 0);
    
    uint height = max_print_lines;
    uint hslice = (hmax+1)/height + (bool)((hmax+1)%height);
    if (hslice == 0) return;
    
    print_simple_line(" Learnt clause distribution in graph form");
    print_footer();
    string yaxis = "Number";
    uint middle = (height-yaxis.size())/2;
    
    for (int i = height-1; i > 0; i--) {
        cout << "| ";
        if (height-1-i >= middle && height-1-i-middle < yaxis.size())
            cout << yaxis[height-1-i-middle] << " ";
        else
            cout << "  ";
        for (uint i2 = 0; i2 < no_slices; i2++) {
            if (slices[i2]/hslice >= i) cout << "+";
            else cout << " ";
        }
        cout << "|" << endl;
    }
    print_center_line(" Learnt clause size");
    print_footer();
}

void Logger::print_general_stats(uint restarts, uint64_t conflicts, int vars, int noClauses, uint64_t clauses_Literals, int noLearnts, double litsPerLearntCl, double progressEstimate) const
{
    print_footer();
    print_simple_line(" Standard MiniSat restart statistics");
    print_footer();
    print_line("Restart number", restarts);
    print_line("Number of conflicts", conflicts);
    print_line("Number of variables", vars);
    print_line("Number of clauses", noClauses);
    print_line("Number of literals in clauses",clauses_Literals);
    print_line("Avg. literals per learnt clause",litsPerLearntCl);
    print_line("Progress estimate (%):", progressEstimate);
    print_footer();
}


// Prints statistics on the console
void Logger::printstats() const
{
    assert(statistics_on);
    assert(varnames.size() == times_var_guessed.size());
    assert(varnames.size() == times_var_propagated.size());

    printf("\n");
    cout << "+" << std::setfill('=') << std::setw(FST_WIDTH+SND_WIDTH+TRD_WIDTH+4) << "=" << "+" << endl;
    cout << "||" << std::setfill('*') << std::setw(FST_WIDTH+SND_WIDTH+TRD_WIDTH+2) << "********* STATS FOR THIS RESTART BEGIN " << "||" << endl;
    cout << "+" << std::setfill('=') << std::setw(FST_WIDTH+SND_WIDTH+TRD_WIDTH+4) << "=" << std::setfill(' ') << "+" << endl;
    
    cout.setf(std::ios_base::left);
    cout.precision(4);
    print_statistics_note();
    print_times_var_guessed();
    print_times_group_caused_propagation();
    print_times_group_caused_conflict();
    print_prop_order();
    print_confl_order();
    print_assign_var_order();
    print_branch_depth_distrib();
    print_learnt_clause_distrib();
    print_advanced_stats();
}

void Logger::print_advanced_stats() const
{
    print_footer();
    print_simple_line(" Advanced statistics");
    print_footer();
    print_line("Unitary learnts", solver->get_unitary_learnts().size());
    print_line("No. branches visited", no_conflicts);
    print_line("Avg. branch depth", (double)sum_conflict_depths/(double)no_conflicts);
    print_line("No. decisions", no_decisions);
    print_line("No. propagations",no_propagations);
    
    //printf("no progatations/no decisions (i.e. one decision gives how many propagations on average *for the whole search graph*): %f\n", (double)no_propagations/(double)no_decisions);
    //printf("no propagations/sum decisions on branches (if you look at one specific branch, what is the average number of propagations you will find?): %f\n", (double)no_propagations/(double)sum_decisions_on_branches);
    
    print_simple_line("sum decisions on branches/no. branches");
    print_simple_line(" (in a given branch, what is the avg.");
    print_line("  no. of decisions?)",(double)sum_decisions_on_branches/(double)no_conflicts);
    
    print_simple_line("sum propagations on branches/no. branches");
    print_simple_line(" (in a given branch, what is the");
    print_line("  avg. no. of propagations?)",(double)sum_propagations_on_branches/(double)no_conflicts);
    print_footer();
}

void Logger::print_statistics_note() const
{
    print_footer();
    print_simple_line("Statistics note: If you used CryptoMiniSat as");
    print_simple_line("a library then vars are all shifted by 1 here");
    print_simple_line("and in every printed output of the solver.");
    print_simple_line("This does not apply when you use CryptoMiniSat");
    print_simple_line("as a stand-alone program.");
    print_footer();
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

void Logger::setSolver(const Solver* _solver)
{
    solver = _solver;
}
