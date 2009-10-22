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
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <signal.h>
#include <zlib.h>
#include "Logger.h"
#include "Solver.h"
using std::cout;
using std::endl;

/*************************************************************************************/
#ifdef _MSC_VER
#include <ctime>

static inline double cpuTime(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
#else
#ifdef CROSS_COMPILE
#include <ctime>

static inline double cpuTime(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

static inline double cpuTime(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000;
}
#endif
#endif


#if defined(__linux__)
static inline int memReadStat(int field)
{
    char    name[256];
    pid_t pid = getpid();
    sprintf(name, "/proc/%d/statm", pid);
    FILE*   in = fopen(name, "rb");
    if (in == NULL) return 0;
    int     value;
    for (; field >= 0; field--)
        fscanf(in, "%d", &value);
    fclose(in);
    return value;
}
static inline uint64_t memUsed()
{
    return (uint64_t)memReadStat(0) * (uint64_t)getpagesize();
}


#elif defined(__FreeBSD__)
static inline uint64_t memUsed(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss*1024;
}


#else
static inline uint64_t memUsed()
{
    return 0;
}
#endif

#if defined(__linux__)
#include <fpu_control.h>
#endif

static bool grouping = false;

//=================================================================================================
// DIMACS Parser:

#define CHUNK_LIMIT 1048576

class StreamBuffer
{
    gzFile  in;
    char    buf[CHUNK_LIMIT];
    int     pos;
    int     size;

    void assureLookahead() {
        if (pos >= size) {
            pos  = 0;
            size = gzread(in, buf, sizeof(buf));
        }
    }

public:
    StreamBuffer(gzFile i) : in(i), pos(0), size(0) {
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
            ++in;
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
    int     parsed_lit, var;
    lits.clear();
    for (;;) {
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
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

    for (;;) {
        skipWhitespace(in);
        switch (*in) {
        case EOF:
            return;
        case 'p':
            if (match(in, "p cnf")) {
                int vars    = parseInt(in);
                int clauses = parseInt(in);
                printf("|  Number of variables:  %-12d                                         |\n", vars);
                printf("|  Number of clauses:    %-12d                                         |\n", clauses);
            } else {
                printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
            }
            break;
        case 'c':
            ++in;
            parseString(in, str);
            if (str == "v" || str == "var") {
                int var = parseInt(in);
                if (var <= 0) cout << "PARSE ERROR! Var number must be a positive integer" << endl, exit(3);
                char tmp[500];
                untilEnd(in, tmp);
                S.setVariableName(var-1, tmp);
            } else {
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
static void parse_DIMACS(gzFile input_stream, Solver& S)
{
    StreamBuffer in(input_stream);
    parse_DIMACS_main(in, S);
}


//=================================================================================================


void printStats(Solver& solver)
{
    double   cpu_time = cpuTime();
    uint64_t mem_used = memUsed();
    cout << "restarts              : " << solver.starts << endl ;
    cout << "conflicts             : " << solver.conflicts << " (" << (double)solver.conflicts/cpu_time << " /sec)" << endl;
    cout << "decisions             : " << solver.decisions << " (" << (double)solver.rnd_decisions*100.0/(double)solver.decisions << "% random)" << endl;
    cout << "propagations          : " << solver.propagations << " (" << (double)solver.propagations/cpu_time << " /sec)" << endl;
    cout << "conflict literals     : " << solver.tot_literals << " (" << (double)(solver.max_literals - solver.tot_literals)*100.0/ (double)solver.max_literals << "% deleted)" << endl;
    if (mem_used != 0) cout << "Memory used           : " << (double)mem_used / 1048576.0 << " MB\n";
    cout << "CPU time              : " << cpu_time << " s" << endl;
}

Solver* solver;
static void SIGINT_handler(int signum)
{
    printf("\n");
    printf("*** INTERRUPTED ***\n");
    printStats(*solver);
    printf("\n");
    printf("*** INTERRUPTED ***\n");
    exit(1);
}


//=================================================================================================
// Main:

void printUsage(char** argv)
{
    printf("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n\n", argv[0]);
    printf("OPTIONS:\n\n");
    printf("  -polarity-mode = {true,false,rnd}\n");
    printf("  -decay         = <num> [ 0 - 1 ]\n");
    printf("  -rnd-freq      = <num> [ 0 - 1 ]\n");
    printf("  -verbosity     = {0,1,2}\n");
    printf("  -proof-log     = Logs the proof into files 'proofN.dot', where N is the\n");
    printf("                   restart number. The log can then be visualized using\n");
    printf("                   the 'dot' program from the graphviz package\n");
    printf("  -grouping      = Lets you group clauses, and customize the groups' names.\n");
    printf("                   This helps when printing statistics\n");
    printf("  -stats         = Computes and prints statistics during the search\n");
    printf("  -randomize     = <seed> [0 - 2^32-1] Randomly permutates the clauses. The \n");
    printf("                   seed is later also used for picking decision variables\n");
    printf("  -restrict      = <num> [1 - varnum] when picking random variables to branch\n");
    printf("                   on, pick one that in the 'num' most active vars useful\n");
    printf("                   for cryptographic problems, where the question is the key,\n");
    printf("                   which is usually small (e.g. 80 bits)\n");
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
    S.verbosity = 1;
    bool permutateClauses = false;


    int         i, j;
    const char* value;
    for (i = j = 0; i < argc; i++) {
        if ((value = hasPrefix(argv[i], "-polarity-mode="))) {
            if (strcmp(value, "true") == 0)
                S.polarity_mode = Solver::polarity_true;
            else if (strcmp(value, "false") == 0)
                S.polarity_mode = Solver::polarity_false;
            else if (strcmp(value, "rnd") == 0)
                S.polarity_mode = Solver::polarity_rnd;
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

        } else if ((value = hasPrefix(argv[i], "-decay="))) {
            double decay;
            if (sscanf(value, "%lf", &decay) <= 0 || decay <= 0 || decay > 1) {
                printf("ERROR! illegal decay constant %s\n", value);
                exit(0);
            }
            S.var_decay = 1 / decay;

        } else if ((value = hasPrefix(argv[i], "-verbosity="))) {
            int verbosity = (int)strtol(value, NULL, 10);
            if (verbosity == 0 && errno == EINVAL) {
                printf("ERROR! illegal verbosity level %s\n", value);
                exit(0);
            }
            S.verbosity = verbosity;
        } else if ((value = hasPrefix(argv[i], "-grouping"))) {
            grouping = true;
        } else if ((value = hasPrefix(argv[i], "-proof-log"))) {
            S.needProofGraph();

        } else if ((value = hasPrefix(argv[i], "-stats"))) {
            S.needStats();

        } else if ((value = hasPrefix(argv[i], "-randomize="))) {
            uint32_t seed;
            if (sscanf(value, "%d", &seed) < 0) {
                printf("ERROR! illegal seed %s\n", value);
                exit(0);
            }
            cout << "seed:" << seed << endl;
            S.setSeed(seed);
            permutateClauses = true;
        } else if ((value = hasPrefix(argv[i], "-restrict="))) {
            uint branchTo;
            if (sscanf(value, "%d", &branchTo) < 0) {
                printf("ERROR! illegal variable number %d\n", branchTo);
                exit(0);
            }
            S.restrictedPickBranch = branchTo-1; //-1 needed as var 1 is represented as var 0 internally

        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv);
            exit(0);

        } else if (strncmp(argv[i], "-", 1) == 0) {
            printf("ERROR! unknown flag %s\n", argv[i]);
            exit(0);

        } else
            argv[j++] = argv[i];
    }
    argc = j;


    printf("This is MiniSat 2.0 beta\n");
#if defined(__linux__)
    fpu_control_t oldcw, newcw;
    _FPU_GETCW(oldcw);
    newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
    _FPU_SETCW(newcw);
    if (S.verbosity >= 1) printf("WARNING: for repeatability, setting FPU to use double precision\n");
#endif
    double cpu_time = cpuTime();

    solver = &S;
    signal(SIGINT,SIGINT_handler);
    //signal(SIGHUP,SIGINT_handler);

    if (argc == 1)
        printf("Reading from standard input... Use '-h' or '--help' for help.\n");

    gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
    if (in == NULL)
        printf("ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]), exit(1);

    if (S.verbosity >= 1) {
        printf("============================[ Problem Statistics ]=============================\n");
        printf("|                                                                             |\n");
    }

    S.startClauseAdding();
    parse_DIMACS(in, S);
    if (permutateClauses) S.permutateClauses();
    gzclose(in);
    FILE* res = (argc >= 3) ? fopen(argv[2], "wb") : NULL;

    double parse_time = cpuTime() - cpu_time;
    if (S.verbosity >= 1) printf("|  Parsing time:         %-12.2f s                                       |\n", parse_time);

    if (!S.simplify()) {
        S.endFirstSimplify();
        printf("Solved by unit propagation\n");
        if (res != NULL) fprintf(res, "UNSAT\n"), fclose(res);
        printf("UNSATISFIABLE\n");
        exit(20);
    }
    S.endFirstSimplify();

    bool ret = S.solve();
    if (S.verbosity >= 1) printStats(S);
    printf("\n");
    printf(ret ? "SATISFIABLE\n" : "UNSATISFIABLE\n");
    if (res != NULL) {
        if (ret) {
            fprintf(res, "SAT\n");
            for (int i = 0; i < S.nVars(); i++)
                if (S.model[i] != l_Undef)
                    fprintf(res, "%s%s%d", (i==0)?"":" ", (S.model[i]==l_True)?"":"-", i+1);
            fprintf(res, " 0\n");
        } else
            fprintf(res, "UNSAT\n");
        fclose(res);
    }

#ifdef NDEBUG
    exit(ret ? 10 : 20);     // (faster than "return", which will invoke the destructor for 'Solver')
#endif
}
