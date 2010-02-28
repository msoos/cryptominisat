/**************************************************************************************************

Main.C -- (C) Niklas Een 2005

Read a DIMACS or BCNF file and apply the SAT-solver to it.

**************************************************************************************************/

#include "Main.h"
#include "Solver.h"
#include <ctime>
#include <unistd.h>
#include <signal.h>
#include "File.h"
#include "Profile.h"

//=================================================================================================
// Options:


static cchar* doc =
    "SatELite <input CNF> [OUTPUTS: <pre-proc. CNF> [<var.map> [<elim. clauses>]]]\n"
    "  options: {+,-}{cs1,csk,ve,s0,s1,s2,r,(de,ud,h1)=ve+,pl,as,all,det, pre,ext,mod}\n"
    "      and: --{help,defaults,verbosity=<num>}\n"
;


bool opt_confl_1sub   = true;
bool opt_confl_ksub   = false;
bool opt_var_elim     = true;
bool opt_0sub         = true;
bool opt_1sub         = true;
bool opt_2sub         = false;
bool opt_repeated_sub = false;
bool opt_def_elim     = false;
bool opt_unit_def     = false;
bool opt_hyper1_res   = false;
bool opt_pure_literal = false;
bool opt_asym_branch  = false;
bool opt_keep_all     = false;
bool opt_no_random    = false;
bool opt_pre_sat      = false;
bool opt_ext_sat      = false;

bool opt_niver        = false;

cchar* input_file    = NULL;
cchar* output_file   = NULL;    // (doubles as result input from MiniSat)
cchar* varmap_file   = NULL;
cchar* elimed_file   = NULL;
cchar* model_file    = NULL;
int    verbosity     = 1;


void parseOptions(int argc, char** argv)
{
    vec<char*>  args;   // Non-options

    for (int i = 1; i < argc; i++){
        char*   arg = argv[i];
        if (arg[0] == '-' || arg[0] == '+'){
            if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
                fprintf(stderr, "%s", doc), exit(0);

            else if (strncmp(arg, "--verbosity=", 12) == 0) verbosity = atoi(arg+12);
            else if (strcmp(arg, "-v0") == 0)               verbosity = 0;
            else if (strcmp(arg, "-v1") == 0)               verbosity = 1;
            else if (strcmp(arg, "-v2") == 0)               verbosity = 2;

            else if (strcmp(arg, "--niver") == 0){
                opt_var_elim     = false;
                opt_0sub         = false;
                opt_1sub         = false;
                opt_pre_sat      = true;
                opt_var_elim     = true;
                opt_niver        = true;
            }

            else if (strcmp(arg, "--plain") == 0 || strcmp(arg, "-") == 0){
                opt_confl_1sub   = false;
                opt_confl_ksub   = false;
                opt_var_elim     = false;
                opt_0sub         = false;
                opt_1sub         = false;
                opt_2sub         = false;
                opt_repeated_sub = false;
                opt_def_elim     = false;
                opt_unit_def     = false;
                opt_hyper1_res   = false;
                opt_pure_literal = false;
                opt_asym_branch  = false;
                opt_keep_all     = false;
                opt_no_random    = false;
                opt_pre_sat      = false;
                opt_ext_sat      = false;
            }

            else if (strcmp(arg, "--defaults") == 0 || strcmp(arg, "-d") == 0)
                printf("%c""cs1" " (confl_1sub  )\n"
                       "%c""csk" " (confl_ksub  )\n"
                       "%c""ve " " (var_elim    )\n"
                       "%c""s0 " " (0sub        )\n"
                       "%c""s1 " " (1sub        )\n"
                       "%c""s2 " " (2sub        )\n"
                       "%c""r  " " (repeated_sub)\n"
                       "%c""de " " (def_elim    )\n"
                       "%c""ud " " (unit_def    )\n"
                       "%c""h1 " " (hyper1_res  )\n"
                       "%c""pl " " (pure_literal)\n"
                       "%c""as " " (asym_branch )\n"
                       "%c""all" " (keep_all    )\n"
                       "%c""det" " (no_random   )\n"
                       "%c""pre" " (pre_sat     )\n"
                       "%c""ext" " (ext_sat     )\n"
                       , opt_confl_1sub   ? '+' : '-'
                       , opt_confl_ksub   ? '+' : '-'
                       , opt_var_elim     ? '+' : '-'
                       , opt_0sub         ? '+' : '-'
                       , opt_1sub         ? '+' : '-'
                       , opt_2sub         ? '+' : '-'
                       , opt_repeated_sub ? '+' : '-'
                       , opt_def_elim     ? '+' : '-'
                       , opt_unit_def     ? '+' : '-'
                       , opt_hyper1_res   ? '+' : '-'
                       , opt_asym_branch  ? '+' : '-'
                       , opt_pure_literal ? '+' : '-'
                       , opt_keep_all     ? '+' : '-'
                       , opt_no_random    ? '+' : '-'
                       , opt_pre_sat      ? '+' : '-'
                       , opt_ext_sat      ? '+' : '-'
                       ),
                       exit(0);

            else if (strcmp(arg+1, "cs1") == 0) opt_confl_1sub   = (arg[0] != '-');
            else if (strcmp(arg+1, "csk") == 0) opt_confl_ksub   = (arg[0] != '-');
            else if (strcmp(arg+1, "ve" ) == 0) opt_var_elim     = (arg[0] != '-');
            else if (strcmp(arg+1, "s0" ) == 0) opt_0sub         = (arg[0] != '-');
            else if (strcmp(arg+1, "s1" ) == 0) opt_1sub         = (arg[0] != '-');
            else if (strcmp(arg+1, "s2" ) == 0) opt_2sub         = (arg[0] != '-');
            else if (strcmp(arg+1, "r"  ) == 0) opt_repeated_sub = (arg[0] != '-');
            else if (strcmp(arg+1, "de" ) == 0) opt_def_elim     = (arg[0] != '-');
            else if (strcmp(arg+1, "ud" ) == 0) opt_unit_def     = (arg[0] != '-');
            else if (strcmp(arg+1, "h1" ) == 0) opt_hyper1_res   = (arg[0] != '-');
            else if (strcmp(arg+1, "as" ) == 0) opt_asym_branch  = (arg[0] != '-');
            else if (strcmp(arg+1, "pl" ) == 0) opt_pure_literal = (arg[0] != '-');
            else if (strcmp(arg+1, "all") == 0) opt_keep_all     = (arg[0] != '-');
            else if (strcmp(arg+1, "det") == 0) opt_no_random    = (arg[0] != '-');
            else if (strcmp(arg+1, "pre") == 0) opt_pre_sat      = (arg[0] != '-');
            else if (strcmp(arg+1, "ext") == 0) opt_ext_sat      = (arg[0] != '-');

            else if (strcmp(arg+1, "ve+") == 0) opt_var_elim = opt_def_elim = opt_unit_def = opt_hyper1_res = true;

            else if (strncmp(arg, "+pre=", 5) == 0){
                opt_pre_sat = true;
                output_file = arg + 5; }

            else if (strncmp(arg, "+mod=", 5) == 0){
                model_file = arg + 5; }

            else
                fprintf(stderr, "ERROR! Invalid command line option: %s\n", argv[i]), exit(1);

        }else
            args.push(arg);
    }

    if (args.size() >= 1)
        input_file = args[0];
    if (args.size() >= 2){
        opt_pre_sat = true;
        if (output_file != NULL) fprintf(stderr, "ERROR! Only one output file can be specified.\n"), exit(1);
        output_file = args[1]; }
    if (args.size() >= 3)
        varmap_file = args[2];
    if (args.size() >= 4)
        elimed_file = args[3];
    else if (args.size() > 4)
        fprintf(stderr, "ERROR! Too many files specified.\n"), exit(1);
}


//=================================================================================================
// BCNF Parser:


#define CHUNK_LIMIT 1048576

static void parse_BCNF(cchar* filename, Solver& S)
{
    FILE*   in = fopen(filename, "rb");
    if (in == NULL) fprintf(stderr, "ERROR! Could not open file: %s\n", filename), exit(1);

    char    header[16];
    fread(header, 1, 16, in);
    if (strncmp(header, "BCNF", 4) != 0) fprintf(stderr, "ERROR! Not a BCNF file: %s\n", filename), exit(1);
    if (*(int*)(header+4) != 0x01020304) fprintf(stderr, "ERROR! BCNF file in unsupported byte-order: %s\n", filename), exit(1);

    int      n_vars    = *(int*)(header+ 8);
//  int      n_clauses = *(int*)(header+12);
    int*     buf       = xmalloc<int>(CHUNK_LIMIT);
    int      buf_sz;
    vec<Lit> c;

    for (int i = 0; i < n_vars; i++) S.newVar();
    //S.setVars(n_vars);

    for(;;){
        int n = fread(&buf_sz, 4, 1, in);
        if (n != 1) break;
        assert(buf_sz <= CHUNK_LIMIT);
        fread(buf, 4, buf_sz, in);

        int* p = buf;
        while (*p != -1){
            int size = *p++;
            c.clear();
            c.growTo(size);
            for (int i = 0; i < size; i++)
                c[i] = toLit(p[i]);
            p += size;

            S.addClause(c);     // Add clause.
        }
    }

    xfree(buf);
    fclose(in);
    //**/printf("MEM USED: %lld\n", memUsed());
}


//=================================================================================================
// DIMACS Parser:


class FileBuffer {
    File    in;
    int     next;
public:
    FileBuffer(cchar* input_file) : in(input_file, "rb") {
        if (in.null())  fprintf(stderr, "ERROR! Could not open file for reading: %s\n", input_file), exit(1);
        next = in.getCharQ(); }
   ~FileBuffer() {}
    int  operator *  () { return next; }
    void operator ++ () { next = in.getCharQ(); }
};


class StringBuffer {
    cchar*  ptr;
    cchar*  last;
public:
    StringBuffer(cchar* text, int size) { ptr = text; last = ptr + size; }
   ~StringBuffer() {}
    int  operator *  () { return (ptr >= last) ? EOF : *ptr; }
    void operator ++ () { ++ptr; }
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<class B>
static void skipWhitespace(B& in) {
    while ((*in >= 9 && *in <= 13) || *in == 32)
        ++in; }

template<class B>
static void skipLine(B& in) {
    for (;;){
        if (*in == EOF) return;
        if (*in == '\n') { ++in; return; }
        ++in; } }

template<class B>
static int parseInt(B& in) {
    int     val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if(*in == EOF) return 0;
    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9') fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
        ++in;
    return neg ? -val : val; }

template<class B>
static void readClause(B& in, Solver& S, vec<Lit>& lits) {
    int     parsed_lit, var;
    lits.clear();
    for (;;){
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        while (var >= S.nVars()) S.newVar();
        lits.push( (parsed_lit > 0) ? Lit(var) : ~Lit(var) );
    }
}

template<class B>
static bool match(B& in, char* str) {
    for (; *str != 0; ++str, ++in)
        if (*str != *in)
            return false;
    return true;
}


template<class B>
static void parse_DIMACS_main(B& in, Solver& S) {
    vec<Lit> lits;
    for (;;){
        skipWhitespace(in);
        if (*in == EOF)
            break;
	else if (*in == 'p'){
	  if (match(in, "p cnf")){
	    int vars    = parseInt(in);
	    int clauses = parseInt(in);
	    if(clauses>4800000) {
	      printf("c num clauses = %d\n",clauses);
	      printf("c too many clauses .. no preprocessing\n");
	      exit(11);
	    } 
	  }else{
	    reportf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
	  }
        }
        else if (*in == 'c' || *in == 'p')
	  skipLine(in);
        else
	  readClause(in, S, lits),
            S.addClause(lits);
    }
}

// Inserts problem into solver. Returns FALSE upon immediate conflict.
//
static void parse_DIMACS(FILE* fptr, Solver& S) {
    int   size;
    char* text = readFile(fptr, &size);
    StringBuffer in(text, size);
    parse_DIMACS_main(in, S);
    free(text); }

// Faster and more memory efficient parsing from a file.
//
static void parse_DIMACS(cchar* input_file, Solver& S) {
    FileBuffer in(input_file);
    parse_DIMACS_main(in, S); }


//=================================================================================================
// Main


static void SIGINT_signalHandler(int signum) {
    printf("\n*** INTERRUPTED ***\n"); exit(-1); }


void printStats(SolverStats& stats, double cpu_time)
{
    reportf("starts                : %8"I64_fmt"\n", stats.starts);
    reportf("conflicts             : %8"I64_fmt"   (%.0f /sec)\n", stats.conflicts   , stats.conflicts   /cpu_time);
    reportf("reduce learnts        : %8"I64_fmt"   (%.0f /sec)\n", stats.reduceDBs   , stats.reduceDBs   /cpu_time);
    reportf("decisions             : %8"I64_fmt"   (%.0f /sec)\n", stats.decisions   , stats.decisions   /cpu_time);
    reportf("propagations          : %8"I64_fmt"   (%.0f /sec)\n", stats.propagations, stats.propagations/cpu_time);
    reportf("inspects              : %8"I64_fmt"   (%.0f /sec)\n", stats.inspects    , stats.inspects    /cpu_time);
    reportf("memory used           : %8g MB\n", memUsed() / (1024*1024.0));
    reportf("CPU time              : %8g s\n", cpu_time);
    if (tsum() != 0.0)
        reportf("Profile time          : %8g seconds\n", tsum());
}


int main(int argc, char** argv)
{
    signal(SIGINT, SIGINT_signalHandler);     // (handle Control-C)
    signal(SIGHUP, SIGINT_signalHandler);

    parseOptions(argc, argv);
    Solver      S(occ_Off, (opt_ext_sat) ? NULL :elimed_file);
    bool        st;

    if (!opt_ext_sat){
        // STANDARD MODE:
        //
        reportf("Parsing...\n");
        if (input_file == NULL)
            parse_DIMACS(stdin, S);
        else{
            cchar*  name = input_file;
            int     len  = strlen(name);

            if (strcmp(name+len-5, ".bcnf") == 0){
                parse_BCNF(name, S);

            }else{
                char*   tmp  = NULL;
                int     stat = 0;
                if (len >= 3 && strcmp(name+len-3, ".gz") == 0){
                    tmp = xstrdup("./tmp_CNF__XXXXXX");
                    int fd = mkstemp(tmp);
                    if (fd == -1)
                        fprintf(stderr, "ERROR! Could not create temporary file for unpacking problem.\n"),
                        exit(1);
                    else
                        close(fd);
                    stat = system(sFree(nsprintf("zcat %s  > %s", name, tmp)));
                    if (stat != 0) fprintf(stderr, "ERROR! Could not open file: %s\n", name), exit(1);
                    parse_DIMACS(tmp, S);
                }else
                    parse_DIMACS(name, S);
                if (tmp != NULL)
                    remove(tmp);
            }
        }
        S.setOccurMode(occ_Permanent);

        S.verbosity = verbosity;
        st = S.solve();
        printStats(S.stats, cpuTime());
        reportf("\n");
      #ifdef SAT_LIVE
        printf(st ? "s SATISFIABLE\n" : "s UNSATISFIABLE\n");
      #else
        reportf(st ? "SATISFIABLE\n" : "UNSATISFIABLE\n");
      #endif

    }else{
        // EXTEND A MODEL FROM MINISAT: [HACK!]
        //
        if (output_file == NULL) fprintf(stderr, "ERROR! Result file from external SAT solver missing when using '+ext'.\n"), exit(1);
        if (varmap_file == NULL) fprintf(stderr, "ERROR! VarMap file from external SAT solver missing when using '+ext'.\n"), exit(1);
        if (elimed_file == NULL) fprintf(stderr, "ERROR! Elimed file from external SAT solver missing when using '+ext'.\n"), exit(1);

        // Open result file:
        int     result_size;
        cchar*  result = readFile(output_file, &result_size);
        if (result_size == 6 && strcmp(result, "UNSAT\n") == 0){
            printf("s UNSATISFIABLE\n");
            exit(20);
        }else if (result_size < 4 || strncmp(result, "SAT\n", 4) != 0)
            fprintf(stderr, "ERROR! Invalid result file from external SAT solver.\n"), exit(1);

        // SATISFIABLE:

        // Read variable map: (including some unit clauses derived during pre-processing)
        int      varmap_size;
        cchar*   varmap = readFile(varmap_file, &varmap_size);
        vec<Var> vmap;
        int      n_vars, val;
        StringBuffer map(varmap, varmap_size);
        n_vars = parseInt(map); skipLine(map);
        while (S.nVars() < n_vars) S.newVar();

        for(;;){
            val = parseInt(map);
            if (val == 0) break;
            S.addUnit( (val > 0) ? Lit(val-1) : ~Lit(-val-1) ); }
        for(;;){
            val = parseInt(map);
            if (val == 0) break;
            vmap.push(val-1); }

        // Read model:
        StringBuffer res(result + 4, result_size - 4);
        bool    neg;
        Lit     p;
        for(;;){
            val = parseInt(res);
            if (val == 0) break;
            if (val < 0) val = -val, neg = true;
            else         neg = false;
            val--;
            if (val >= vmap.size()) fprintf(stderr, "ERROR! Too few variables in variable map file: %s\n", varmap_file), exit(1);
            p = Lit(vmap[val], neg);
            S.addUnit(p);
        }

        // Extend model:
        FILE*    in = fopen(elimed_file, "rb"); if (in == NULL) fprintf(stderr, "ERROR! Could not open file for reading: %s\n", elimed_file), exit(1);
        vec<Lit> io_tmp;
        int      n, size;
        for(;;){
            n = fread(&p, 4, 1, in);
            if (n == 0) break;
            size = index(p);

            io_tmp.clear();
            io_tmp.growTo(size);
            fread((Lit*)io_tmp, 4, size, in);

            S.addClause(io_tmp); /*DEBUG*/if (!S.ok) reportf("PANIC! False clause read back: "), dump(S, io_tmp);
            assert(S.ok);
        }
        fclose(in);

        st = S.solve(); assert(st);
    }

    if (st){
      #ifdef SAT_LIVE
        FILE*   out = stdout;
        printf("s SATISFIABLE\n");
        printf("v ");
      #else
        FILE*   out;
        if (model_file != NULL) {
            out = fopen(model_file, "wb");
            if (out == NULL) fprintf(stderr, "WARNING! Could not write model to: %s\n", model_file);
        }else{
#ifdef VERIFY_MODEL
	  out = createTmpFile("./tmp_model__", "wb", (char*)model_file);
#else
	  out = NULL;
#endif
        }
#endif
        if (out != NULL){
	  for (int i = 0; i < S.model.size(); i++)
	    if (S.model[i] != l_Undef)
	      fprintf(out, "%s%s%d", (i==0)?"":" ", (S.model[i]==l_True)?"":"-", i+1);
        }
        fprintf(out, " 0\n");
#ifndef SAT_LIVE
        if (out != NULL) fflush(out);
      #endif
	
#ifdef VERIFY_MODEL
        if (input_file != NULL)
	  reportf("Verifying model...\n"),
            verifyModel(input_file, model_file),
            reportf("OK!\n");
#endif
    }
    
    exit(st ? 10 : 20);     // (faster than "return", which will invoke the destructor for 'Solver')
    
}
