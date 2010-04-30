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

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

#include "Logger.h"
#include "Solver.h"
#include "time_mem.h"
#include "constants.h"

using std::cout;
using std::endl;

/*************************************************************************************/
#if defined(__linux__)
#include <fpu_control.h>
#endif

static bool grouping = false;
static bool debugLib = false;
static bool debugNewVar = false;
static char learnts_filename[500];
static bool dumpLearnts = false;
static uint32_t maxLearntsSize = std::numeric_limits<uint32_t>::max();
static bool printResult = true;

//=================================================================================================
// DIMACS Parser:

#define CHUNK_LIMIT 1048576

class StreamBuffer
{
#ifdef DISABLE_ZLIB
    FILE *  in;
#else
    gzFile  in;
#endif // DISABLE_ZLIB
    char    buf[CHUNK_LIMIT];
    int     pos;
    int     size;

    void assureLookahead() {
        if (pos >= size) {
            pos  = 0;
#ifdef DISABLE_ZLIB
            #ifdef VERBOSE_DEBUG
            printf("buf = %08X\n", buf);
            printf("sizeof(buf) = %u\n", sizeof(buf));
            #endif //VERBOSE_DEBUG
            size = fread(buf, 1, sizeof(buf), in);
#else
            size = gzread(in, buf, sizeof(buf));
#endif // DISABLE_ZLIB
        }
    }

public:
#ifdef DISABLE_ZLIB
    StreamBuffer(FILE * i) : in(i), pos(0), size(0) {
#else
    StreamBuffer(gzFile i) : in(i), pos(0), size(0) {
#endif // DISABLE_ZLIB
        assureLookahead();
    }

    int  operator *  () {
        return (pos >= size) ? EOF : buf[pos];
    }
    void operator ++ () {
        pos++;
        assureLookahead();
    }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<class B>
static void skipWhitespace(B& in)
{
    while ((*in >= 9 && *in <= 13) || *in == 32)
        ++in;
}

template<class B>
static void skipLine(B& in)
{
    for (;;) {
        if (*in == EOF || *in == '\0') return;
        if (*in == '\n') {
            ++in;
            return;
        }
        ++in;
    }
}

template<class B>
static void untilEnd(B& in, char* ret)
{
    for (;;) {
        if (*in == EOF || *in == '\0') return;
        if (*in == '\n') {
            return;
        }
        *ret = *in;
        ret++;
        *ret = '\0';
        ++in;
    }
}


template<class B>
static int parseInt(B& in)
{
    int     val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9') printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
              ++in;
    return neg ? -val : val;
}

inline std::string stringify(uint x)
{
    std::ostringstream o;
    o << x;
    return o.str();
}

template<class B>
static void parseString(B& in, std::string& str)
{
    str.clear();
    skipWhitespace(in);
    while (*in != ' ' && *in != '\n') {
        str += *in;
        ++in;
    }
}

template<class B>
static void readClause(B& in, Solver& S, vec<Lit>& lits)
{
    int     parsed_lit;
    Var     var;
    lits.clear();
    for (;;) {
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        if (!debugNewVar) {
            while (var >= S.nVars()) S.newVar();
        }
        lits.push( (parsed_lit > 0) ? Lit(var, false) : Lit(var, true) );
    }
}

template<class B>
static bool match(B& in, const char* str)
{
    for (; *str != 0; ++str, ++in)
        if (*str != *in)
            return false;
    return true;
}


template<class B>
static void parse_DIMACS_main(B& in, Solver& S)
{
    vec<Lit> lits;
    int group = 0;
    string str;
    uint debugLibPart = 1;


    for (;;) {
        skipWhitespace(in);
        switch (*in) {
        case EOF:
            return;
        case 'p':
            if (match(in, "p cnf")) {
                int vars    = parseInt(in);
                int clauses = parseInt(in);
                if (S.verbosity >= 1) {
                    printf("c |  Number of variables:  %-12d                                         |\n", vars);
                    printf("c |  Number of clauses:    %-12d                                         |\n", clauses);
                }
            } else {
                printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
            }
            break;
        case 'c':
            ++in;
            parseString(in, str);
            if (str == "v" || str == "var") {
                int var = parseInt(in);
                skipWhitespace(in);
                if (var <= 0) cout << "PARSE ERROR! Var number must be a positive integer" << endl, exit(3);
                char tmp[500];
                untilEnd(in, tmp);
                S.setVariableName(var-1, tmp);
            } if (debugLib && str == "Solver::solve()") {
                lbool ret = S.solve();
                std::string s = "debugLibPart" + stringify(debugLibPart) +".output";
                FILE* res = fopen(s.c_str(), "w");
                if (ret == l_True) {
                    fprintf(res, "SAT\n");
                    for (Var i = 0; i != S.nVars(); i++)
                        if (S.model[i] != l_Undef)
                            fprintf(res, "%s%s%d", (i==0)?"":" ", (S.model[i]==l_True)?"":"-", i+1);
                        fprintf(res, " 0\n");
                } else if (ret == l_False) {
                    fprintf(res, "UNSAT\n");
                } else if (ret == l_Undef) {
                    assert(false);
                } else {
                    assert(false);
                }
                fclose(res);
                debugLibPart++;
            } else if (debugNewVar && str == "Solver::newVar()") {
                S.newVar();
            }
            else {
                skipLine(in);
            }
            break;
        default:
            bool xor_clause = false;
            if ( *in == 'x') xor_clause = true, ++in;
            readClause(in, S, lits);
            skipLine(in);

            char group_name[500];
            group_name[0] = '\0';

            if (!grouping) group++;
            else {
                if (*in != 'c') {
                    cout << "PARSE ERROR! Group must be present after earch clause ('c' missing after clause line)" << endl;
                    exit(3);
                }
                ++in;

                parseString(in, str);
                if (str != "g" && str != "group") {
                    cout << "PARSE ERROR! Group must be present after each clause('group' missing)!" << endl;
                    cout << "Instead of 'group' there was:" << str << endl;
                    exit(3);
                }

                group = parseInt(in);
                if (group <= 0) printf("PARSE ERROR! Group number must be a positive integer\n"), exit(3);

                skipWhitespace(in);
                untilEnd(in, group_name);
            }

            if (xor_clause)
                S.addXorClause(lits, false, group, group_name);
            else
                S.addClause(lits, group, group_name);
            break;
        }
    }
}

// Inserts problem into solver.
//
#ifdef DISABLE_ZLIB
static void parse_DIMACS(FILE * input_stream, Solver& S)
#else
static void parse_DIMACS(gzFile input_stream, Solver& S)
#endif // DISABLE_ZLIB
{
    StreamBuffer in(input_stream);
    parse_DIMACS_main(in, S);
}


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


void printStats(Solver& solver)
{
    double   cpu_time = cpuTime();
    uint64_t mem_used = memUsed();
    printStatsLine("c restarts", solver.starts);
    printStatsLine("c dynamic restarts", solver.dynStarts);
    printStatsLine("c static restarts", solver.staticStarts);
    printStatsLine("c full restarts", solver.fullStarts);
    printStatsLine("c learnts DL2", solver.nbDL2);
    printStatsLine("c learnts size 2", solver.nbBin);
    printStatsLine("c learnts size 1", solver.get_unitary_learnts_num());
    printStatsLine("c OTF clause improved", solver.improvedClauseNo);
    printStatsLine("c OTF impr. size diff", solver.improvedClauseSize, (double)solver.improvedClauseSize/(double)solver.improvedClauseNo, " lits/clause");
    
    printStatsLine("c conflicts", solver.conflicts, (double)solver.conflicts/cpu_time, "/ sec");
    printStatsLine("c decisions", solver.decisions, (double)solver.rnd_decisions*100.0/(double)solver.decisions, "% random");
    printStatsLine("c propagations", solver.propagations, (double)solver.propagations/cpu_time, "/ sec");
    printStatsLine("c conflict literals", solver.tot_literals, (double)(solver.max_literals - solver.tot_literals)*100.0/ (double)solver.max_literals, "% deleted");
    printStatsLine("c Memory used", (double)mem_used / 1048576.0, " MB");
    printStatsLine("c CPU time", cpu_time, " s");
}

Solver* solver;
static void SIGINT_handler(int signum)
{
    printf("\n");
    printf("*** INTERRUPTED ***\n");
    printStats(*solver);
    if (dumpLearnts) {
        solver->dumpSortedLearnts(learnts_filename, maxLearntsSize);
        cout << "c Sorted learnt clauses dumped to file '" << learnts_filename << "'" << endl;
    }
    printf("\n");
    printf("*** INTERRUPTED ***\n");
    exit(1);
}


//=================================================================================================
// Main:

void printUsage(char** argv)
{
#ifdef DISABLE_ZLIB
    printf("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input is plain DIMACS.\n\n", argv[0]);
#else
    printf("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n\n", argv[0]);
#endif // DISABLE_ZLIB
    printf("OPTIONS:\n\n");
    printf("  -polarity-mode = {true,false,rnd,auto} [default: auto]. Selects the default\n");
    printf("                 = polarity mode. Auto is the Jeroslow&Wang method\n");
    //printf("  -decay         = <num> [ 0 - 1 ]\n");
    printf("  -rnd-freq      = <num> [ 0 - 1 ]\n");
    printf("  -verbosity     = {0,1,2}\n");
    #ifdef STATS_NEEDED
    printf("  -proof-log     = Logs the proof into files 'proofN.dot', where N is the\n");
    printf("                   restart number. The log can then be visualized using\n");
    printf("                   the 'dot' program from the graphviz package\n");
    printf("  -grouping      = Lets you group clauses, and customize the groups' names.\n");
    printf("                   This helps when printing statistics\n");
    printf("  -stats         = Computes and prints statistics during the search\n");
    #endif
    printf("  -randomize     = <seed> [0 - 2^32-1] Sets random seed, used for picking\n");
    printf("                   decision variables (default = 0)\n");
    printf("  -restrict      = <num> [1 - varnum] when picking random variables to branch\n");
    printf("                   on, pick one that in the 'num' most active vars useful\n");
    printf("                   for cryptographic problems, where the question is the key,\n");
    printf("                   which is usually small (e.g. 80 bits)\n");
    printf("  -gaussuntil    = <num> The depth until which Gaussian elimination is active.\n");
    printf("                   giving 0 means that Gaussian elimination is switched off\n");
    printf("  -restarts       = <num> [1 - 2^32-1] No more than the given number of\n");
    printf("                   restarts will be performed during search\n");
    printf("  -nonormxorfind  = Don't find and collect >2-long xor-clauses from regular clauses\n");
    printf("  -nobinxorfind   = Don't find and collect 2-long xor-clauses from regular clauses\n");
    printf("  -noregularbinxorfind  = Don't find and collect 2-long xor-clauses from regular clauses\n");
    printf("  -noconglomerate = Don't conglomerate 2 xor clauses when one var is dependent\n");
    printf("  -nosimplify     = Don't do regular simplification rounds\n");
    printf("  -greedyUnbound  = Greedily unbound variables that are not needed for SAT\n");
    printf("  -debugLib       = Solve at specific 'c Solver::solve()' points in the CNF file\n");
    printf("                    Used to debug file generated by Solver's DEBUG_LIB flag\n");
    printf("  -debugNewVar    = Add new vars at specific 'c Solver::newVar()' points in the CNF file\n");
    printf("                    Used to debug file generated by Solver's DEBUG_LIB flag\n");
    printf("  -novarreplace   = Don't perform variable replacement. Needed for programmable\n");
    printf("                    solver feature\n");
    printf("  -restart        = {auto, static, dynamic}   Which kind of restart strategy to\n");
    printf("                    follow. Default is auto\n");
    printf("  -dumpLearnts    = <filename> If interrupted or reached restart limit, dump the\n");
    printf("                    learnt unitary clauses to the specified file\n");
    printf("  -maxDumpLearntS = [0 - 2^32-1] When dumping the learnts to file, what should be\n");
    printf("                    maximum length of the clause dumped. Useful to make the\n");
    printf("                    resulting file smaller. Default is 2^32-1 (i.e. all lenghts)\n");
    printf("  -nofailedvar    = Don't search for failed vars, and don't search for vars\n");
    printf("                    doubly propagated to the same value\n");
    printf("  -noheuleprocess = Don't try to minimise XORs by XOR-ing them together.\n");
    printf("                    Algo. as per global/local substitution in Heule's thesis\n");
    printf("  -nosubsumption  = Don't try to subsume clauses.\n");
    printf("  -noxorsubsumption Don't try to subsume xor-clauses.\n");
    printf("  -nohyperbinres  = Don't carry out hyper-binary resolution\n");
    printf("  -noResPrint     = Don't print the satisfying assignment if the solution is SAT\n");
    printf("  -novarelim      = Don't perform variable elimination as per Een and Biere\n");
    printf("  -nosubsume1     = Don't perform clause contraction through resolution\n");
    printf("  -noparthander   = Don't find and solve subroblems with subsolvers\n");
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


int main(int argc, char** argv)
{
    Solver      S;
    S.verbosity = 2;

    const char* value;
    int j = 0;
    for (int i = 0; i < argc; i++) {
        if ((value = hasPrefix(argv[i], "-polarity-mode="))) {
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

        } else if ((value = hasPrefix(argv[i], "-rnd-freq="))) {
            double rnd;
            if (sscanf(value, "%lf", &rnd) <= 0 || rnd < 0 || rnd > 1) {
                printf("ERROR! illegal rnd-freq constant %s\n", value);
                exit(0);
            }
            S.random_var_freq = rnd;

        /*} else if ((value = hasPrefix(argv[i], "-decay="))) {
            double decay;
            if (sscanf(value, "%lf", &decay) <= 0 || decay <= 0 || decay > 1) {
                printf("ERROR! illegal decay constant %s\n", value);
                exit(0);
            }
            S.var_decay = 1 / decay;*/

        } else if ((value = hasPrefix(argv[i], "-verbosity="))) {
            int verbosity = (int)strtol(value, NULL, 10);
            if (errno == EINVAL) {
                printf("ERROR! illegal verbosity level %s\n", value);
                exit(0);
            }
            S.verbosity = verbosity;
        #ifdef STATS_NEEDED
        } else if ((value = hasPrefix(argv[i], "-grouping"))) {
            grouping = true;
        } else if ((value = hasPrefix(argv[i], "-proof-log"))) {
            S.needProofGraph();

        } else if ((value = hasPrefix(argv[i], "-stats"))) {
            S.needStats();
        #endif

        } else if ((value = hasPrefix(argv[i], "-randomize="))) {
            uint32_t seed;
            if (sscanf(value, "%d", &seed) < 0) {
                printf("ERROR! illegal seed %s\n", value);
                exit(0);
            }
            cout << "c seed:" << seed << endl;
            S.setSeed(seed);
        } else if ((value = hasPrefix(argv[i], "-restrict="))) {
            uint branchTo;
            if (sscanf(value, "%d", &branchTo) < 0 || branchTo < 1) {
                printf("ERROR! illegal restricted pick branch number %d\n", branchTo);
                exit(0);
            }
            S.restrictedPickBranch = branchTo;
        } else if ((value = hasPrefix(argv[i], "-gaussuntil="))) {
            uint32_t until;
            if (sscanf(value, "%d", &until) < 0) {
                printf("ERROR! until %s\n", value);
                exit(0);
            }
            cout << "c Gaussian until:" << until << endl;
            S.gaussconfig.decision_until = until;
        } else if ((value = hasPrefix(argv[i], "-restarts="))) {
            uint maxrest;
            if (sscanf(value, "%d", &maxrest) < 0 || maxrest == 0) {
                printf("ERROR! illegal maximum restart number %d\n", maxrest);
                exit(0);
            }
            S.setMaxRestarts(maxrest);
        } else if ((value = hasPrefix(argv[i], "-dumpLearnts="))) {
            if (sscanf(value, "%400s", learnts_filename) < 0 || strlen(learnts_filename) == 0) {
                printf("ERROR! wrong filename '%s'\n", learnts_filename);
                exit(0);
            }
            dumpLearnts = true;
        } else if ((value = hasPrefix(argv[i], "-maxDumpLearntS="))) {
            if (!dumpLearnts) {
                printf("ERROR! -dumpLearnts=<filename> must be first activated before issuing -maxDumpLearntS=<size>\n");
                exit(0);
            }
            int tmp;
            if (sscanf(value, "%d", &tmp) < 0 || tmp < 0) {
                cout << "ERROR! wrong maximum dumped learnt clause size is illegal: " << tmp << endl;
                exit(0);
            }
            maxLearntsSize = (uint32_t)tmp;
        } else if ((value = hasPrefix(argv[i], "-greedyUnbound"))) {
            S.greedyUnbound = true;
        } else if ((value = hasPrefix(argv[i], "-nonormxorfind"))) {
            S.findNormalXors = false;
        } else if ((value = hasPrefix(argv[i], "-nobinxorfind"))) {
            S.findBinaryXors = false;
        } else if ((value = hasPrefix(argv[i], "-noregularbinxorfind"))) {
            S.regularlyFindBinaryXors = false;
        } else if ((value = hasPrefix(argv[i], "-noconglomerate"))) {
            S.conglomerateXors = false;
        } else if ((value = hasPrefix(argv[i], "-nosimplify"))) {
            S.schedSimplification = false;
        } else if ((value = hasPrefix(argv[i], "-debugLib"))) {
            debugLib = true;
        } else if ((value = hasPrefix(argv[i], "-debugNewVar"))) {
            debugNewVar = true;
        } else if ((value = hasPrefix(argv[i], "-novarreplace"))) {
            S.performReplace = false;
        } else if ((value = hasPrefix(argv[i], "-nofailedvar"))) {
            S.failedVarSearch = false;
        } else if ((value = hasPrefix(argv[i], "-nodisablegauss"))) {
            S.gaussconfig.dontDisable = true;
        } else if ((value = hasPrefix(argv[i], "-noheuleprocess"))) {
            S.heuleProcess = false;
        } else if ((value = hasPrefix(argv[i], "-nosubsumption"))) {
            S.doSubsumption = false;
        } else if ((value = hasPrefix(argv[i], "-noparthandler"))) {
            S.doPartHandler = false;
        } else if ((value = hasPrefix(argv[i], "-noxorsubsumption"))) {
            S.doXorSubsumption = false;
        } else if ((value = hasPrefix(argv[i], "-nohyperbinres"))) {
            S.doHyperBinRes = false;
        } else if ((value = hasPrefix(argv[i], "-noblockedclause"))) {
            S.doBlockedClause = false;
        } else if ((value = hasPrefix(argv[i], "-novarelim"))) {
            S.doVarElim = false;
        } else if ((value = hasPrefix(argv[i], "-nosubsume1"))) {
            S.doSubsume1 = false;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv);
            exit(0);
        } else if ((value = hasPrefix(argv[i], "-restart="))) {
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
        } else if ((value = hasPrefix(argv[i], "-noResPrint"))) {
            printResult = false;
        } else if (strncmp(argv[i], "-", 1) == 0) {
            printf("ERROR! unknown flag %s\n", argv[i]);
            exit(0);

        } else
            argv[j++] = argv[i];
    }
    argc = j;
    if (!debugLib) S.libraryUsage = false;
    S.verbosity = 1;
    
    if (S.verbosity >= 1)
        printf("c This is CryptoMiniSat\n");
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
        printf("c ============================[ Problem Statistics ]=============================\n");
        printf("c |                                                                             |\n");
    }

    parse_DIMACS(in, S);

#ifdef DISABLE_ZLIB
    fclose(in);
#else
    gzclose(in);
#endif // DISABLE_ZLIB
    if (argc >= 3)
        printf("c Outputting solution to file: %s\n" , argv[2]);

    double parse_time = cpuTime() - cpu_time;
    if (S.verbosity >= 1)
        printf("c |  Parsing time:         %-12.2f s                                       |\n", parse_time);

    lbool ret = S.solve();
    printStats(S);
    printf("c \n");
    if (dumpLearnts) {
        S.dumpSortedLearnts(learnts_filename, maxLearntsSize);
        cout << "c Sorted learnt clauses dumped to file '" << learnts_filename << "'" << endl;
    }
    if (ret == l_Undef)
        printf("c Not finished running -- maximum restart reached\n");
    
    FILE* res = NULL;
    if (argc >= 3) {
        res = fopen(argv[2], "wb");
        if (res == NULL) {
            int backup_errno = errno;
            printf("Cannot open %s for writing. Problem: %s", argv[2], strerror(backup_errno));
            exit(1);
        }
    }
    
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

#ifdef NDEBUG
    exit(ret == l_True ? 10 : 20);     // (faster than "return", which will invoke the destructor for 'Solver')
#endif

    if (ret == l_True) return 10;
    if (ret == l_False) return 20;
    if (ret == l_Undef) return 15;
    assert(false);
    
    return 0;
}
