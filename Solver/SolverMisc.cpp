/***************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "Solver.h"
#include "VarReplacer.h"
#include "Subsumer.h"
#include "XorSubsumer.h"
#include "time_mem.h"
#include "DimacsParser.h"
#include <iomanip>

#ifdef USE_GAUSS
#include "Gaussian.h"
#endif

static const int space = 10;

void Solver::dumpSortedLearnts(const char* fileName, const uint32_t maxSize)
{
    FILE* outfile = fopen(fileName, "w");
    if (!outfile) {
        printf("Error: Cannot open file '%s' to write learnt clauses!\n", fileName);
        exit(-1);
    }

    fprintf(outfile, "c \nc ---------\n");
    fprintf(outfile, "c unitaries\n");
    fprintf(outfile, "c ---------\n");
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++) {
        trail[i].printFull(outfile);
        #ifdef STATS_NEEDED
        if (dynamic_behaviour_analysis)
            fprintf(outfile, "c name of var: %s\n", logger.get_var_name(trail[i].var()).c_str());
        #endif //STATS_NEEDED
    }

    fprintf(outfile, "c conflicts %lu\n", (unsigned long)conflicts);
    if (maxSize == 1) goto end;

    fprintf(outfile, "c \nc ---------------------------------\n");
    fprintf(outfile, "c learnt clauses from binaryClauses\n");
    fprintf(outfile, "c ---------------------------------\n");
    for (uint32_t i = 0; i != binaryClauses.size(); i++) {
        if (binaryClauses[i]->learnt()) {
            binaryClauses[i]->print(outfile);
        }
    }

    fprintf(outfile, "c \nc ---------------------------------------\n");
    fprintf(outfile, "c clauses representing 2-long XOR clauses\n");
    fprintf(outfile, "c ---------------------------------------\n");
    {
        const vector<Lit>& table = varReplacer->getReplaceTable();
        for (Var var = 0; var != table.size(); var++) {
            Lit lit = table[var];
            if (lit.var() == var)
                continue;

            fprintf(outfile, "%s%d %d 0\n", (!lit.sign() ? "-" : ""), lit.var()+1, var+1);
            fprintf(outfile, "%s%d -%d 0\n", (lit.sign() ? "-" : ""), lit.var()+1, var+1);
            #ifdef STATS_NEEDED
            if (dynamic_behaviour_analysis)
                fprintf(outfile, "c name of two vars that are anti/equivalent: '%s' and '%s'\n", logger.get_var_name(lit.var()).c_str(), logger.get_var_name(var).c_str());
            #endif //STATS_NEEDED
        }
    }
    fprintf(outfile, "c \nc --------------------\n");
    fprintf(outfile, "c clauses from learnts\n");
    fprintf(outfile, "c --------------------\n");
    if (lastSelectedRestartType == dynamic_restart)
        std::sort(learnts.getData(), learnts.getData()+learnts.size(), reduceDB_ltGlucose());
    else
        std::sort(learnts.getData(), learnts.getData()+learnts.size(), reduceDB_ltMiniSat());
    for (int i = learnts.size()-1; i >= 0 ; i--) {
        if (learnts[i]->size() <= maxSize) {
            learnts[i]->print(outfile);
        }
    }

    end:

    fclose(outfile);
}

void Solver::dumpOrigClauses(const char* fileName, const bool alsoLearntBin) const
{
    FILE* outfile = fopen(fileName, "w");
    if (!outfile) {
        printf("Error: Cannot open file '%s' to write learnt clauses!\n", fileName);
        exit(-1);
    }

    uint32_t numClauses = 0;
    //unitary clauses
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++)
      numClauses++;
    //binary XOR clauses
    const vector<Lit>& table = varReplacer->getReplaceTable();
    for (Var var = 0; var != table.size(); var++) {
        Lit lit = table[var];
        if (lit.var() == var)
            continue;
        numClauses+=2;
    }
    //binary normal clauses
    for (Clause *const *i = binaryClauses.getData(); i != binaryClauses.getDataEnd(); i++) {
        if (!alsoLearntBin && (*i)->learnt()) continue;
        numClauses++;
    }
    //normal clauses
    numClauses += clauses.size();
    //previously eliminated clauses
    const map<Var, vector<Clause*> >& elimedOutVar = subsumer->getElimedOutVar();
    for (map<Var, vector<Clause*> >::const_iterator it = elimedOutVar.begin(); it != elimedOutVar.end(); it++) {
        const vector<Clause*>& cs = it->second;
        numClauses += cs.size();
    }
    fprintf(outfile, "p cnf %d %d\n", nVars(), numClauses);

    fprintf(outfile, "c \nc ---------\n");
    fprintf(outfile, "c unitaries\n");
    fprintf(outfile, "c ---------\n");
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++) {
        trail[i].printFull(outfile);
        #ifdef STATS_NEEDED
        if (dynamic_behaviour_analysis)
            fprintf(outfile, "c name of var: %s\n", logger.get_var_name(trail[i].var()).c_str());
        #endif //STATS_NEEDED
    }

    fprintf(outfile, "c \nc ---------------------------------------\n");
    fprintf(outfile, "c clauses representing 2-long XOR clauses\n");
    fprintf(outfile, "c ---------------------------------------\n");
    for (Var var = 0; var != table.size(); var++) {
        Lit lit = table[var];
        if (lit.var() == var)
            continue;

        fprintf(outfile, "%s%d %d 0\n", (!lit.sign() ? "-" : ""), lit.var()+1, var+1);
        fprintf(outfile, "%s%d -%d 0\n", (lit.sign() ? "-" : ""), lit.var()+1, var+1);
        #ifdef STATS_NEEDED
        if (dynamic_behaviour_analysis)
            fprintf(outfile, "c name of two vars that are anti/equivalent: '%s' and '%s'\n", logger.get_var_name(lit.var()).c_str(), logger.get_var_name(var).c_str());
        #endif //STATS_NEEDED
    }

    fprintf(outfile, "c \nc ------------\n");
    fprintf(outfile, "c binary clauses\n");
    fprintf(outfile, "c ---------------\n");
    for (Clause *const *i = binaryClauses.getData(); i != binaryClauses.getDataEnd(); i++) {
        if (!alsoLearntBin && (*i)->learnt()) continue;
        (*i)->print(outfile);
    }

    fprintf(outfile, "c \nc ------------\n");
    fprintf(outfile, "c normal clauses\n");
    fprintf(outfile, "c ---------------\n");
    for (Clause *const *i = clauses.getData(); i != clauses.getDataEnd(); i++) {
        assert(!(*i)->learnt());
        (*i)->print(outfile);
    }

    //map<Var, vector<Clause*> > elimedOutVar
    fprintf(outfile, "c -------------------------------\n");
    fprintf(outfile, "c previously eliminated variables\n");
    fprintf(outfile, "c -------------------------------\n");
    for (map<Var, vector<Clause*> >::const_iterator it = elimedOutVar.begin(); it != elimedOutVar.end(); it++) {
        const vector<Clause*>& cs = it->second;
        for (vector<Clause*>::const_iterator it2 = cs.begin(); it2 != cs.end(); it2++) {
            (*it2)->print(outfile);
        }
    }

    fclose(outfile);
}

const vector<Lit> Solver::get_unitary_learnts() const
{
    vector<Lit> unitaries;
    if (decisionLevel() > 0) {
        for (uint32_t i = 0; i != trail_lim[0]; i++) {
            unitaries.push_back(trail[i]);
        }
    }
    
    return unitaries;
}

const vec<Clause*>& Solver::get_learnts() const
{
    return learnts;
}

const vec<Clause*>& Solver::get_sorted_learnts()
{
    if (lastSelectedRestartType == dynamic_restart)
        std::sort(learnts.getData(), learnts.getData()+learnts.size(), reduceDB_ltGlucose());
    else
        std::sort(learnts.getData(), learnts.getData()+learnts.size(), reduceDB_ltMiniSat());
    return learnts;
}

const uint32_t Solver::getNumElimSubsume() const
{
    return subsumer->getNumElimed();
}

const uint32_t Solver::getNumElimXorSubsume() const
{
    return xorSubsumer->getNumElimed();
}

const uint32_t Solver::getNumXorTrees() const
{
    return varReplacer->getNumTrees();
}

const uint32_t Solver::getNumXorTreesCrownSize() const
{
    return varReplacer->getNumReplacedVars();
}

const double Solver::getTotalTimeSubsumer() const
{
    return subsumer->getTotalTime();
}

const double Solver::getTotalTimeXorSubsumer() const
{
    return xorSubsumer->getTotalTime();
}


void Solver::setMaxRestarts(const uint32_t num)
{
    maxRestarts = num;
}

void Solver::printStatHeader() const
{
    #ifdef STATS_NEEDED
    if (verbosity >= 1 && !(dynamic_behaviour_analysis && logger.statistics_on)) {
    #else
    if (verbosity >= 1) {
    #endif
        std::cout << "c "
        << "========================================================================================="
        << std::endl;
        std::cout << "c"
        << " types(t): F = full restart, N = normal restart" << std::endl;
        std::cout << "c"
        << " types(t): S = simplification begin/end, E = solution found" << std::endl;
        std::cout << "c"
        << " restart types(rt): st = static, dy = dynamic" << std::endl;

        std::cout << "c "
        << std::setw(2) << "t"
        << std::setw(3) << "rt"
        << std::setw(6) << "Rest"
        << std::setw(space) << "Confl"
        << std::setw(space) << "Vars"
        << std::setw(space) << "NormCls"
        << std::setw(space) << "BinCls"
        << std::setw(space) << "Learnts"
        << std::setw(space) << "ClLits"
        << std::setw(space) << "LtLits"
        << std::endl;
    }
}

void Solver::printRestartStat(const char* type)
{
    if (verbosity >= 2) {
        //printf("c | %9d | %7d %8d %8d | %8d %8d %6.0f |", (int)conflicts, (int)order_heap.size(), (int)(nClauses()-nbBin), (int)clauses_literals, (int)(nbclausesbeforereduce*curRestart+nbCompensateSubsumer), (int)(nLearnts()+nbBin), (double)learnts_literals/(double)(nLearnts()+nbBin));

        std::cout << "c "
        << std::setw(2) << type
        << std::setw(3) << ((restartType == static_restart) ? "st" : "dy")
        << std::setw(6) << starts
        << std::setw(space) << conflicts
        << std::setw(space) << order_heap.size()
        << std::setw(space) << clauses.size()
        << std::setw(space) << binaryClauses.size()
        << std::setw(space) << learnts.size()
        << std::setw(space) << clauses_literals
        << std::setw(space) << learnts_literals;

        #ifdef USE_GAUSS
        print_gauss_sum_stats();
        #endif //USE_GAUSS

        std::cout << std::endl;
    }
}

void Solver::printEndSearchStat()
{
    #ifdef STATS_NEEDED
    if (verbosity >= 1 && !(dynamic_behaviour_analysis && logger.statistics_on)) {
    #else
    if (verbosity >= 1) {
    #endif //STATS_NEEDED
        printRestartStat("E");
    }
}

#ifdef USE_GAUSS
void Solver::print_gauss_sum_stats()
{
    if (gauss_matrixes.size() == 0 && verbosity >= 2) {
        std::cout << "  no matrixes";
        return;
    }

    uint32_t called = 0;
    uint32_t useful_prop = 0;
    uint32_t useful_confl = 0;
    uint32_t disabled = 0;
    for (vector<Gaussian*>::const_iterator gauss = gauss_matrixes.begin(), end= gauss_matrixes.end(); gauss != end; gauss++) {
        disabled += (*gauss)->get_disabled();
        called += (*gauss)->get_called();
        useful_prop += (*gauss)->get_useful_prop();
        useful_confl += (*gauss)->get_useful_confl();
        sum_gauss_unit_truths += (*gauss)->get_unit_truths();
        //gauss->print_stats();
        //gauss->print_matrix_stats();
    }
    sum_gauss_called += called;
    sum_gauss_confl += useful_confl;
    sum_gauss_prop += useful_prop;
    
    if (verbosity >= 2) {
        if (called == 0) {
            printf("      disabled      |\n");
        } else {
            printf(" %3.0lf%% |", (double)useful_prop/(double)called*100.0);
            printf(" %3.0lf%% |", (double)useful_confl/(double)called*100.0);
            printf(" %3.0lf%% |\n", 100.0-(double)disabled/(double)gauss_matrixes.size()*100.0);
        }
    }
}
#endif //USE_GAUSS

void Solver::sortWatched()
{
    double myTime = cpuTime();
    for (vec<Watched> *i = watches.getData(), *end = watches.getDataEnd(); i != end; i++) {
        #ifdef VERBOSE_DEBUG
        vec<Watched>& ws = *i;
        std::cout << "Before:" << std::endl;
        for (uint32_t i2 = 0; i2 < ws.size(); i2++) {
            if (ws[i2].isBinary()) std::cout << "Binary,";
            if (ws[i2].isTriClause()) std::cout << "Tri,";
            if (ws[i2].isClause()) std::cout << "Normal,";
            if (ws[i2].isXorClause()) std::cout << "Xor,";
        }
        #endif //VERBOSE_DEBUG

        std::sort(i->getData(), i->getDataEnd(), WatchedSorter());

        #ifdef VERBOSE_DEBUG
        std::cout << "After:" << std::endl;
        for (uint32_t i2 = 0; i2 < ws.size(); i2++) {
            if (ws[i2].isBinary()) std::cout << "Binary,";
            if (ws[i2].isTriClause()) std::cout << "Tri,";
            if (ws[i2].isClause()) std::cout << "Normal,";
            if (ws[i2].isXorClause()) std::cout << "Xor,";
        }
        std::cout << std::endl;
        #endif //VERBOSE_DEBUG
    }

    if (verbosity >= 2) {
        std::cout << "c watched "
        << "sorting time: " << cpuTime() - myTime
        << std::endl;
    }
}

void Solver::addSymmBreakClauses()
{
    if (xorclauses.size() > 0) {
        std::cout << "c xor clauses present -> no saucy" << std::endl;
        return;
    }
    double myTime = cpuTime();
    std::cout << "c Doing saucy" << std::endl;
    dumpOrigClauses("origProblem.cnf", true);
    system("grep -v \"^c\" origProblem.cnf > origProblem2.cnf");
    system("python saucyReader.py origProblem2.cnf > output");
    DimacsParser parser(this, false, false, false, true);

    #ifdef DISABLE_ZLIB
    FILE * in = fopen("output", "rb");
    #else
    gzFile in = gzopen("output", "rb");
    #endif // DISABLE_ZLIB
    parser.parse_DIMACS(in);
    #ifdef DISABLE_ZLIB
    fclose(in);
    #else
    gzclose(in);
    #endif // DISABLE_ZLIB
    std::cout << "c Finished saucy, time: " << (cpuTime() - myTime) << std::endl;
}
