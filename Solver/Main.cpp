/******************************************************************************************[Main.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
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

/**
@mainpage CryptoMiniSat
@author Mate Soos, and collaborators

CryptoMiniSat is an award-winning SAT solver based on MiniSat. It brings a
number of benefits relative to MiniSat, among them XOR clauses, extensive
failed literal probing, and better random search.

The solver basically performs the following steps:

1) parse CNF file into clause database

2) run Conflict-Driven Clause-Learning DPLL on the clauses

3) regularly run simplification passes on the clause-set

4) display solution and if not used as a library, exit

Here is a picture of of the above process in more detail:

\image html "main_flowgraph.png"

*/

#include <ctime>
#include <cstring>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include <signal.h>
#include <string>
using std::string;

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

#ifdef STATS_NEEDED
#include "Logger.h"
#endif //STATS_NEEDED

#include "Solver.h"
#include "time_mem.h"
#include "constants.h"
#include "DimacsParser.h"

using std::cout;
using std::endl;

/*************************************************************************************/
#if defined(__linux__)
#include <fpu_control.h>
#endif

static bool grouping = false;
static bool debugLib = false;
static bool debugNewVar = false;
static bool printResult = true;

//=================================================================================================

template<class T, class T2>
inline void printStatsLine(string left, T value, T2 value2, string extra)
{
    cout << std::fixed << std::left << std::setw(24) << left << ": " << std::setw(11) << std::setprecision(2) << value << " (" << std::left << std::setw(9) << std::setprecision(2) << value2 << " " << extra << ")" << std::endl;
}

template<class T>
inline void printStatsLine(string left, T value, string extra = "")
{
    cout << std::fixed << std::left << std::setw(24) << left << ": " << std::setw(11) << std::setprecision(2) << value << extra << std::endl;
}

/**
@brief prints the statistics line at the end of solving

Prints all sorts of statistics, like number of restarts, time spent in
SatELite-type simplification, number of unit claues found, etc.
*/
void printStats(Solver& solver)
{
    double   cpu_time = cpuTime();
    uint64_t mem_used = memUsed();

    //Restarts stats
    printStatsLine("c restarts", solver.starts);
    printStatsLine("c dynamic restarts", solver.dynStarts);
    printStatsLine("c static restarts", solver.staticStarts);
    printStatsLine("c full restarts", solver.fullStarts);

    //Learnts stats
    printStatsLine("c learnts DL2", solver.nbDL2);
    printStatsLine("c learnts size 2", solver.nbBin);
    printStatsLine("c learnts size 1", solver.get_unitary_learnts_num(), (double)solver.get_unitary_learnts_num()/(double)solver.nVars()*100.0, "% of vars");

    //Subsumer stats
    printStatsLine("c v-elim SatELite", solver.getNumElimSubsume(), (double)solver.getNumElimSubsume()/(double)solver.nVars()*100.0, "% vars");
    printStatsLine("c SatELite time", solver.getTotalTimeSubsumer(), solver.getTotalTimeSubsumer()/cpu_time*100.0, "% time");

    //XorSubsumer stats
    printStatsLine("c v-elim xor", solver.getNumElimXorSubsume(), (double)solver.getNumElimXorSubsume()/(double)solver.nVars()*100.0, "% vars");
    printStatsLine("c xor elim time", solver.getTotalTimeXorSubsumer(), solver.getTotalTimeXorSubsumer()/cpu_time*100.0, "% time");

    //VarReplacer stats
    printStatsLine("c num binary xor trees", solver.getNumXorTrees());
    printStatsLine("c binxor trees' crown", solver.getNumXorTreesCrownSize(), (double)solver.getNumXorTreesCrownSize()/(double)solver.getNumXorTrees(), "leafs/tree");

    //OTF clause improvement stats
    printStatsLine("c OTF clause improved", solver.improvedClauseNo, (double)solver.improvedClauseNo/(double)solver.conflicts, "clauses/conflict");
    printStatsLine("c OTF impr. size diff", solver.improvedClauseSize, (double)solver.improvedClauseSize/(double)solver.improvedClauseNo, " lits/clause");

    //Clause-shrinking through watchlists
    printStatsLine("c OTF cl watch-shrink", solver.numShrinkedClause, (double)solver.numShrinkedClause/(double)solver.conflicts, "clauses/conflict");
    printStatsLine("c OTF cl watch-sh-lit", solver.numShrinkedClauseLits, (double)solver.numShrinkedClauseLits/(double)solver.numShrinkedClause, " lits/clause");

    #ifdef USE_GAUSS
    if (solver.gaussconfig.decision_until > 0) {
        std::cout << "c " << std::endl;
        printStatsLine("c gauss unit truths ", solver.get_sum_gauss_unit_truths());
        printStatsLine("c gauss called", solver.get_sum_gauss_called());
        printStatsLine("c gauss conflicts ", solver.get_sum_gauss_confl(), (double)solver.get_sum_gauss_confl() / (double)solver.get_sum_gauss_called() * 100.0, " %");
        printStatsLine("c gauss propagations ", solver.get_sum_gauss_prop(), (double)solver.get_sum_gauss_prop() / (double)solver.get_sum_gauss_called() * 100.0, " %");
        printStatsLine("c gauss useful", ((double)solver.get_sum_gauss_prop() + (double)solver.get_sum_gauss_confl())/ (double)solver.get_sum_gauss_called() * 100.0, " %");
        std::cout << "c " << std::endl;
    }
    #endif

    //Search stats
    printStatsLine("c conflicts", solver.conflicts, (double)solver.conflicts/cpu_time, "/ sec");
    printStatsLine("c decisions", solver.decisions, (double)solver.rnd_decisions*100.0/(double)solver.decisions, "% random");
    printStatsLine("c bogo-props", solver.propagations, (double)solver.propagations/cpu_time, "/ sec");
    printStatsLine("c conflict literals", solver.tot_literals, (double)(solver.max_literals - solver.tot_literals)*100.0/ (double)solver.max_literals, "% deleted");

    //General stats
    printStatsLine("c Memory used", (double)mem_used / 1048576.0, " MB");
    printStatsLine("c CPU time", cpu_time, " s");
}

/**
@brief For correctly and gracefully exiting

It can happen that the user requests a dump of the learnt clauses. In this case,
the program must wait until it gets to a state where the learnt clauses are in
a correct state, then dump these and quit normally. This interrupt hander
is used to achieve this
*/
Solver* solver;
static void SIGINT_handler(int signum)
{
    printf("\n");
    printf("*** INTERRUPTED ***\n");
    if (solver->needToDumpLearnts || solver->needToDumpOrig) {
        solver->needToInterrupt = true;
        printf("*** Please wait. We need to interrupt cleanly\n");
        printf("*** This means we might need to finish some calculations\n");
        printf("*** INTERRUPTED ***\n");
    } else {
        printStats(*solver);
        exit(1);
    }
}


//=================================================================================================
// Main:

void printUsage(char** argv, Solver& S)
{
#ifdef DISABLE_ZLIB
    printf("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input is plain DIMACS.\n\n", argv[0]);
#else
    printf("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n\n", argv[0]);
#endif // DISABLE_ZLIB
    printf("OPTIONS:\n\n");
    printf("  --polarity-mode  = {true,false,rnd,auto} [default: auto]. Selects the default\n");
    printf("                     polarity mode. Auto is the Jeroslow&Wang method\n");
    //printf("  -decay         = <num> [ 0 - 1 ]\n");
    printf("  --rnd-freq       = <num> [ 0 - 1 ]\n");
    printf("  --verbosity      = {0,1,2}\n");
    #ifdef STATS_NEEDED
    printf("  --proof-log      = Logs the proof into files 'proofN.dot', where N is the\n");
    printf("                     restart number. The log can then be visualized using\n");
    printf("                     the 'dot' program from the graphviz package\n");
    printf("  --grouping       = Lets you group clauses, and customize the groups' names.\n");
    printf("                     This helps when printing statistics\n");
    printf("  --stats          = Computes and prints statistics during the search\n");
    #endif
    printf("  --randomize      = <seed> [0 - 2^32-1] Sets random seed, used for picking\n");
    printf("                     decision variables (default = 0)\n");
    printf("  --restrict       = <num> [1 - varnum] when picking random variables to branch\n");
    printf("                     on, pick one that in the 'num' most active vars useful\n");
    printf("                     for cryptographic problems, where the question is the key,\n");
    printf("                     which is usually small (e.g. 80 bits)\n");
    printf("  --gaussuntil     = <num> Depth until which Gaussian elimination is active.\n");
    printf("                     Giving 0 switches off Gaussian elimination\n");
    printf("  --restarts       = <num> [1 - 2^32-1] No more than the given number of\n");
    printf("                     restarts will be performed during search\n");
    printf("  --nonormxorfind  = Don't find and collect >2-long xor-clauses from\n");
    printf("                     regular clauses\n");
    printf("  --nobinxorfind   = Don't find and collect 2-long xor-clauses from\n");
    printf("                     regular clauses\n");
    printf("  --noregbxorfind  = Don't regularly find and collect 2-long xor-clauses\n");
    printf("                     from regular clauses\n");
    printf("  --noconglomerate = Don't conglomerate 2 xor clauses when one var is dependent\n");
    printf("  --nosimplify     = Don't do regular simplification rounds\n");
    printf("  --greedyunbound  = Greedily unbound variables that are not needed for SAT\n");
    printf("  --debuglib       = Solve at specific 'c Solver::solve()' points in the CNF\n");
    printf("                     file. Used to debug file generated by Solver's\n");
    printf("                     needLibraryCNFFile() function\n");
    printf("  --debugnewvar    = Add new vars at specific 'c Solver::newVar()' points in \n");
    printf("                     the CNF file. Used to debug file generated by Solver's\n");
    printf("                     needLibraryCNFFile() function.\n");
    printf("  --novarreplace   = Don't perform variable replacement. Needed for programmable\n");
    printf("                     solver feature\n");
    printf("  --restart        = {auto, static, dynamic}   Which kind of restart strategy to\n");
    printf("                     follow. Default is auto\n");
    printf("  --dumplearnts    = <filename> If interrupted or reached restart limit, dump\n");
    printf("                     the learnt clauses to the specified file. Maximum size of\n");
    printf("                     dumped clauses can be specified with next option.\n");
    printf("  --maxdumplearnts = [0 - 2^32-1] When dumping the learnts to file, what\n");
    printf("                     should be maximum length of the clause dumped. Useful\n");
    printf("                     to make the resulting file smaller. Default is 2^32-1\n");
    printf("                     note: 2-long XOR-s are always dumped.\n");
    printf("  --dumporig       = <filename> If interrupted or reached restart limit, dump\n");
    printf("                     the original problem instance, simplified to the\n");
    printf("                     current point.\n");
    printf("  --maxsolutions   = Search for given amount of solutions\n");
    printf("  --nofailedvar    = Don't search for failed vars, and don't search for vars\n");
    printf("                     doubly propagated to the same value\n");
    printf("  --noheuleprocess = Don't try to minimise XORs by XOR-ing them together.\n");
    printf("                     Algo. as per global/local substitution in Heule's thesis\n");
    printf("  --nosatelite     = Don't do clause subsumption, clause strengthening and\n");
    printf("                     variable elimination (implies -novarelim and -nosubsume1).\n");
    printf("  --noxorsubs      = Don't try to subsume xor-clauses.\n");
    printf("  --nohyperbinres  = Don't carry out hyper-binary resolution\n");
    printf("  --nosolprint     = Don't print the satisfying assignment if the solution\n");
    printf("                     is SAT\n");
    printf("  --novarelim      = Don't perform variable elimination as per Een and Biere\n");
    printf("  --nosubsume1     = Don't perform clause contraction through resolution\n");
    printf("  --noparthandler  = Don't find and solve subroblems with subsolvers\n");
#ifdef USE_GAUSS
    printf("  --nomatrixfind   = Don't find distinct matrixes. Put all xors into one\n");
    printf("                     big matrix\n");
    printf("  --noordercol     = Don't order variables in the columns of Gaussian\n");
    printf("                     elimination. Effectively disables iterative reduction\n");
    printf("                     of the matrix\n");
    printf("  --noiterreduce   = Don't reduce iteratively the matrix that is updated\n");
    printf("  --maxmatrixrows  = [0 - 2^32-1] Set maximum no. of rows for gaussian matrix.\n");
    printf("                     Too large matrixes should bee discarded for\n");
    printf("                     reasons of efficiency. Default: %d\n", S.gaussconfig.maxMatrixRows);
    printf("  --minmatrixrows  = [0 - 2^32-1] Set minimum no. of rows for gaussian matrix.\n");
    printf("                     Normally, too small matrixes are discarded for\n");
    printf("                     reasons of efficiency. Default: %d\n", S.gaussconfig.minMatrixRows);
    printf("  --savematrix     = [0 - 2^32-1] Save matrix every Nth decision level.\n");
    printf("                     Default: %d\n", S.gaussconfig.only_nth_gauss_save);
    printf("  --maxnummatrixes = [0 - 2^32-1] Maximum number of matrixes to treat.\n");
    printf("                     Default: %d\n", S.gaussconfig.maxNumMatrixes);
#endif //USE_GAUSS
    //printf("  --addoldlearnts  = Readd old learnts for failed variable searching.\n");
    //printf("                     These learnts are usually deleted, but may help\n");
    printf("  --noextrabins    = Don't add binary clauses when doing failed lit probing.\n");
    printf("  --noremovebins   = Don't remove useless binary clauses at the beginnning\n");
    printf("  --noregremovebins= Don't remove useless binary clauses regularly\n");
    printf("  --nosubswithbins = Don't subsume with non-existent bins at the beginnning\n");
    printf("  --norsubswithbins= Don't subsume with non-existent bins regularly \n");
    printf("  --noasymm        = Don't do asymmetric branching at the beginnning\n");
    printf("  --norasymm       = Don't do asymmetric branching regularly\n");
    printf("  --nosortwatched  = Don't sort watches according to size: bin, tri, etc.\n");
    printf("  --nolfminim      = Don't do on-the-fly self-subsuming resolution\n");
    printf("                     (called 'strong minimisation' in PrecoSat)\n");
    printf("  --lfminimrec     = Perform recursive/transitive OTF self-\n");
    printf("                     subsuming resolution (enhancement of \n");
    printf("                     'strong minimisation' in PrecoSat)\n");
    printf("\n");
}


const char* hasPrefix(const char* str, const char* prefix)
{
    int len = strlen(prefix);
    if (strncmp(str, prefix, len) == 0)
        return str + len;
    else
        return NULL;
}

void printResultFunc(const Solver& S, const lbool ret, FILE* res)
{
    if (res != NULL) {
        if (ret == l_True) {
            printf("c SAT\n");
            fprintf(res, "SAT\n");
            if (printResult) {
                for (Var var = 0; var != S.nVars(); var++)
                    if (S.model[var] != l_Undef)
                        fprintf(res, "%s%d ", (S.model[var] == l_True)? "" : "-", var+1);
                    fprintf(res, "0\n");
            }
        } else if (ret == l_False) {
            printf("c UNSAT\n");
            fprintf(res, "UNSAT\n");
        } else {
            printf("c INCONCLUSIVE\n");
            fprintf(res, "INCONCLUSIVE\n");
        }
        fclose(res);
    } else {
        if (ret == l_True)
            printf("s SATISFIABLE\n");
        else if (ret == l_False)
            printf("s UNSATISFIABLE\n");

        if(ret == l_True && printResult) {
            printf("v ");
            for (Var var = 0; var != S.nVars(); var++)
                if (S.model[var] != l_Undef)
                    printf("%s%d ", (S.model[var] == l_True)? "" : "-", var+1);
                printf("0\n");
        }
    }
}

int main(int argc, char** argv)
{
    Solver      S;
    S.verbosity = 2;

    const char* value;
    int j = 0;
    unsigned long max_nr_of_solutions = 1;
    unsigned long current_nr_of_solutions = 1;

    for (int i = 0; i < argc; i++) {
        if ((value = hasPrefix(argv[i], "--polarity-mode="))) {
            if (strcmp(value, "true") == 0)
                S.polarity_mode = Solver::polarity_true;
            else if (strcmp(value, "false") == 0)
                S.polarity_mode = Solver::polarity_false;
            else if (strcmp(value, "rnd") == 0)
                S.polarity_mode = Solver::polarity_rnd;
            else if (strcmp(value, "auto") == 0)
                S.polarity_mode = Solver::polarity_auto;
            else {
                printf("ERROR! unknown polarity-mode %s\n", value);
                exit(0);
            }

        } else if ((value = hasPrefix(argv[i], "--rnd-freq="))) {
            double rnd;
            if (sscanf(value, "%lf", &rnd) <= 0 || rnd < 0 || rnd > 1) {
                printf("ERROR! illegal rnd-freq constant %s\n", value);
                exit(0);
            }
            S.random_var_freq = rnd;

        /*} else if ((value = hasPrefix(argv[i], "--decay="))) {
            double decay;
            if (sscanf(value, "%lf", &decay) <= 0 || decay <= 0 || decay > 1) {
                printf("ERROR! illegal decay constant %s\n", value);
                exit(0);
            }
            S.var_decay = 1 / decay;*/

        } else if ((value = hasPrefix(argv[i], "--verbosity="))) {
            int verbosity = (int)strtol(value, NULL, 10);
            if (verbosity == EINVAL || verbosity == ERANGE) {
                printf("ERROR! illegal verbosity level %s\n", value);
                exit(0);
            }
            S.verbosity = verbosity;
        #ifdef STATS_NEEDED
        } else if ((value = hasPrefix(argv[i], "--grouping"))) {
            grouping = true;
        } else if ((value = hasPrefix(argv[i], "--proof-log"))) {
            S.needProofGraph();

        } else if ((value = hasPrefix(argv[i], "--stats"))) {
            S.needStats();
        #endif

        } else if ((value = hasPrefix(argv[i], "--randomize="))) {
            uint32_t seed;
            if (sscanf(value, "%d", &seed) < 0) {
                printf("ERROR! illegal seed %s\n", value);
                exit(0);
            }
            cout << "c seed:" << seed << endl;
            S.setSeed(seed);
        } else if ((value = hasPrefix(argv[i], "--restrict="))) {
            uint32_t branchTo;
            if (sscanf(value, "%d", &branchTo) < 0 || branchTo < 1) {
                printf("ERROR! illegal restricted pick branch number %d\n", branchTo);
                exit(0);
            }
            S.restrictedPickBranch = branchTo;
        } else if ((value = hasPrefix(argv[i], "--gaussuntil="))) {
            uint32_t until;
            if (sscanf(value, "%d", &until) < 0) {
                printf("ERROR! until %s\n", value);
                exit(0);
            }
            S.gaussconfig.decision_until = until;
        } else if ((value = hasPrefix(argv[i], "--restarts="))) {
            uint32_t maxrest;
            if (sscanf(value, "%d", &maxrest) < 0 || maxrest == 0) {
                printf("ERROR! illegal maximum restart number %d\n", maxrest);
                exit(0);
            }
            S.setMaxRestarts(maxrest);
        } else if ((value = hasPrefix(argv[i], "--dumplearnts="))) {
            if (sscanf(value, "%400s", S.learntsFilename) < 0 || strlen(S.learntsFilename) == 0) {
                printf("ERROR! wrong filename '%s'\n", S.learntsFilename);
                exit(0);
            }
            S.needToDumpLearnts = true;
        } else if ((value = hasPrefix(argv[i], "--dumporig="))) {
            if (sscanf(value, "%400s", S.origFilename) < 0 || strlen(S.origFilename) == 0) {
                printf("ERROR! wrong filename '%s'\n", S.origFilename);
                exit(0);
            }
            S.needToDumpOrig = true;
        } else if ((value = hasPrefix(argv[i], "--maxdumplearnts="))) {
            if (!S.needToDumpLearnts) {
                printf("ERROR! -dumplearnts=<filename> must be first activated before issuing -maxdumplearnts=<size>\n");
                exit(0);
            }
            int tmp;
            if (sscanf(value, "%d", &tmp) < 0 || tmp < 0) {
                cout << "ERROR! wrong maximum dumped learnt clause size is illegal: " << tmp << endl;
                exit(0);
            }
            S.maxDumpLearntsSize = (uint32_t)tmp;
        } else if ((value = hasPrefix(argv[i], "--maxsolutions="))) {
            int tmp;
            if (sscanf(value, "%d", &tmp) < 0 || tmp < 0) {
                cout << "ERROR! wrong maximum number of solutions is illegal: " << tmp << endl;
                exit(0);
            }
            max_nr_of_solutions = (uint32_t)tmp;
        } else if ((value = hasPrefix(argv[i], "--greedyunbound"))) {
            S.greedyUnbound = true;
        } else if ((value = hasPrefix(argv[i], "--nonormxorfind"))) {
            S.findNormalXors = false;
        } else if ((value = hasPrefix(argv[i], "--nobinxorfind"))) {
            S.findBinaryXors = false;
        } else if ((value = hasPrefix(argv[i], "--noregbxorfind"))) {
            S.regFindBinaryXors = false;
        } else if ((value = hasPrefix(argv[i], "--noconglomerate"))) {
            S.conglomerateXors = false;
        } else if ((value = hasPrefix(argv[i], "--nosimplify"))) {
            S.schedSimplification = false;
        } else if ((value = hasPrefix(argv[i], "--debuglib"))) {
            debugLib = true;
        } else if ((value = hasPrefix(argv[i], "--debugnewvar"))) {
            debugNewVar = true;
        } else if ((value = hasPrefix(argv[i], "--novarreplace"))) {
            S.doReplace = false;
        } else if ((value = hasPrefix(argv[i], "--nofailedvar"))) {
            S.failedVarSearch = false;
        } else if ((value = hasPrefix(argv[i], "--nodisablegauss"))) {
            S.gaussconfig.dontDisable = true;
        } else if ((value = hasPrefix(argv[i], "--maxnummatrixes="))) {
            uint32_t maxNumMatrixes;
            if (sscanf(value, "%d", &maxNumMatrixes) < 0) {
                printf("ERROR! maxnummatrixes: %s\n", value);
                exit(0);
            }
            S.gaussconfig.maxNumMatrixes = maxNumMatrixes;
        } else if ((value = hasPrefix(argv[i], "--noheuleprocess"))) {
            S.heuleProcess = false;
        } else if ((value = hasPrefix(argv[i], "--nosatelite"))) {
            S.doSubsumption = false;
        } else if ((value = hasPrefix(argv[i], "--noparthandler"))) {
            S.doPartHandler = false;
        } else if ((value = hasPrefix(argv[i], "--noxorsubs"))) {
            S.doXorSubsumption = false;
        } else if ((value = hasPrefix(argv[i], "--nohyperbinres"))) {
            S.doHyperBinRes = false;
        } else if ((value = hasPrefix(argv[i], "--noblockedclause"))) {
            S.doBlockedClause = false;
        } else if ((value = hasPrefix(argv[i], "--novarelim"))) {
            S.doVarElim = false;
        } else if ((value = hasPrefix(argv[i], "--nosubsume1"))) {
            S.doSubsume1 = false;
        } else if ((value = hasPrefix(argv[i], "--nomatrixfind"))) {
            S.gaussconfig.noMatrixFind = true;
        } else if ((value = hasPrefix(argv[i], "--noiterreduce"))) {
            S.gaussconfig.iterativeReduce = false;
        } else if ((value = hasPrefix(argv[i], "--noiterreduce"))) {
            S.gaussconfig.iterativeReduce = false;
        } else if ((value = hasPrefix(argv[i], "--noordercol"))) {
            S.gaussconfig.orderCols = false;
        } else if ((value = hasPrefix(argv[i], "--maxmatrixrows"))) {
            uint32_t rows;
            if (sscanf(value, "%d", &rows) < 0) {
                printf("ERROR! maxmatrixrows: %s\n", value);
                exit(0);
            }
            S.gaussconfig.maxMatrixRows = rows;
        } else if ((value = hasPrefix(argv[i], "--minmatrixrows"))) {
            uint32_t rows;
            if (sscanf(value, "%d", &rows) < 0) {
                printf("ERROR! minmatrixrows: %s\n", value);
                exit(0);
            }
            S.gaussconfig.minMatrixRows = rows;
        } else if ((value = hasPrefix(argv[i], "--savematrix"))) {
            uint32_t every;
            if (sscanf(value, "%d", &every) < 0) {
                printf("ERROR! savematrix: %s\n", value);
                exit(0);
            }
            cout << "c Matrix saved every " <<  every << " decision levels" << endl;
            S.gaussconfig.only_nth_gauss_save = every;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv, S);
            exit(0);
        } else if ((value = hasPrefix(argv[i], "--restart="))) {
            if (strcmp(value, "auto") == 0)
                S.fixRestartType = auto_restart;
            else if (strcmp(value, "static") == 0)
                S.fixRestartType = static_restart;
            else if (strcmp(value, "dynamic") == 0)
                S.fixRestartType = dynamic_restart;
            else {
                printf("ERROR! unknown restart type %s\n", value);
                exit(0);
            }
        } else if ((value = hasPrefix(argv[i], "--nosolprint"))) {
            printResult = false;
        //} else if ((value = hasPrefix(argv[i], "--addoldlearnts"))) {
        //    S.readdOldLearnts = true;
        } else if ((value = hasPrefix(argv[i], "--noextrabins"))) {
            S.addExtraBins = false;
        } else if ((value = hasPrefix(argv[i], "--noremovebins"))) {
            S.remUselessBins = false;
        } else if ((value = hasPrefix(argv[i], "--noregremovebins"))) {
            S.regRemUselessBins = false;
        } else if ((value = hasPrefix(argv[i], "--nosubswithbins"))) {
            S.subsWNonExistBins = false;
        } else if ((value = hasPrefix(argv[i], "--norsubswithbins"))) {
            S.regSubsWNonExistBins = false;
        } else if ((value = hasPrefix(argv[i], "--noasymm"))) {
            S.doAsymmBranch = false;
        } else if ((value = hasPrefix(argv[i], "--norasymm"))) {
            S.doAsymmBranchReg = false;
        } else if ((value = hasPrefix(argv[i], "--nosortwatched"))) {
            S.doSortWatched = false;
        } else if ((value = hasPrefix(argv[i], "--nolfminim"))) {
            S.doMinimLearntMore = false;
        } else if ((value = hasPrefix(argv[i], "--lfminimrec"))) {
            S.doMinimLMoreRecur = true;
        } else if (strncmp(argv[i], "-", 1) == 0 || strncmp(argv[i], "--", 2) == 0) {
            printf("ERROR! unknown flag %s\n", argv[i]);
            exit(0);
        } else
            argv[j++] = argv[i];
    }
    argc = j;
    if (!debugLib) S.libraryUsage = false;

    if (S.verbosity >= 1)
        printf("c This is CryptoMiniSat %s\n", VERSION);
#if defined(__linux__)
    fpu_control_t oldcw, newcw;
    _FPU_GETCW(oldcw);
    newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
    _FPU_SETCW(newcw);
    if (S.verbosity >= 1) printf("c WARNING: for repeatability, setting FPU to use double precision\n");
#endif
    double cpu_time = cpuTime();

    solver = &S;
    signal(SIGINT,SIGINT_handler);
    //signal(SIGHUP,SIGINT_handler);

    if (argc == 1)
        printf("c Reading from standard input... Use '-h' or '--help' for help.\n");

#ifdef DISABLE_ZLIB
    FILE * in = (argc == 1) ? fopen(0, "rb") : fopen(argv[1], "rb");
#else
    gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
#endif // DISABLE_ZLIB
    if (in == NULL) {
        printf("ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]);
        exit(1);
    }

    if (S.verbosity >= 1) {
        printf("c =================================[ Problem Statistics ]==================================\n");
        printf("c |                                                                                       |\n");
    }

    DimacsParser parser(&S, debugLib, debugNewVar, grouping);
    parser.parse_DIMACS(in);

#ifdef DISABLE_ZLIB
    fclose(in);
#else
    gzclose(in);
#endif // DISABLE_ZLIB
    if (argc >= 3)
        printf("c Outputting solution to file: %s\n" , argv[2]);

    double parse_time = cpuTime() - cpu_time;
    if (S.verbosity >= 1)
        printf("c |  Parsing time:         %-12.2f s                                                 |\n", parse_time);

    lbool ret;

    FILE* res = NULL;
    if (argc >= 3) {
        res = fopen(argv[2], "wb");
        if (res == NULL) {
            int backup_errno = errno;
            printf("Cannot open %s for writing. Problem: %s", argv[2], strerror(backup_errno));
            exit(1);
        }
    }

    while(1)
    {
        ret = S.solve();
        if ( ret != l_True ) break;

        std::cout << "c " << std::setw(8) << current_nr_of_solutions++ << " solution(s) found" << std::endl;

        if (current_nr_of_solutions > max_nr_of_solutions) break;
        printf("c Prepare for next run...\n");

        vec<Lit> lits;
        for (Var var = 0; var != S.nVars(); var++) {
            if (S.model[var] != l_Undef) {
                lits.push( Lit(var, (S.model[var] == l_True)? true : false) );
            }
        }
        printResultFunc(S, ret, res);

        S.addClause(lits);
    }

    printStats(S);
    printf("c \n");
    if (S.needToDumpLearnts) {
        S.dumpSortedLearnts(S.learntsFilename, S.maxDumpLearntsSize);
        cout << "c Sorted learnt clauses dumped to file '" << S.learntsFilename << "'" << endl;
    }
    if (S.needToDumpOrig) {
        S.dumpOrigClauses(S.origFilename);
        std::cout << "c Simplified original clauses dumped to file '" << S.origFilename << "'" << std::endl;
    }
    if (ret == l_Undef)
        printf("c Not finished running -- maximum restart reached\n");

    printResultFunc(S, ret, res);

#ifdef NDEBUG
    exit(ret == l_True ? 10 : 20);     // (faster than "return", which will invoke the destructor for 'Solver')
#endif

    if (ret == l_True) return 10;
    if (ret == l_False) return 20;
    if (ret == l_Undef) return 15;
    assert(false);

    return 0;
}
