/******************************************
Copyright (c) 2018, Henry Kautz <henry.kautz@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

/************************************/
/* Compilation flags                */
/************************************/

/********************************************************************/
/* Following tests set exactly one of the following flags to 1:     */
/*    BSD:   BSD Unix                                               */
/*    OSX:   Apple OS X                                           */
/*    LINUX: Linux Unix                                           */
/*    WINDOWS: Windows and DOS. Linking requires -l Winmm.lib       */
/*    POSIX: Other POSIX OS                                         */
/* Platform dependent differences:                                  */
/*    WINDOWS and POSIX use rand() instead of random()              */
/*    Clock ticks per second determined by sysconf(_SC_CLK_TCK)     */
/*        for BSD, OSX, and LINUX                                   */
/*    Clock ticks per second fixed at 1000 for Windows              */
/*    Clock ticks per second fixed at 1 for POSIX                   */
/********************************************************************/

#include <cstdint>
#include "time_mem.h"

#if __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || _SYSTYPE_BSD
#define BSD 1
#elif __APPLE__ && __MACH__
#define OSX 1
#elif __unix__ || __unix || unix || __gnu_linux__ || linux || __linux
#define LINUX 1
#elif _WIN64 || _WIN23 || _WIN16 || __MSDOS__ || MSDOS || _MSDOS || __DOS__
#define NT 1
#else
#define POSIX 1
#endif

/* Define int64_t to be the type used for the "cutoff" variable.
   Under gcc "long long int" gives a 64 bit integer.
   Under Windows __int64 gives a 64 bit integer.
   No way under POSIX to guarantee long int is 64 bits.
   Program will still function using a 32-bit value, but it
   limit size of cutoffs that can be specified. */

#if BSD || OSX || LINUX
#define BIGFORMAT "lli"
#elif WINDOWS
#define BIGFORMAT "I64d"
#endif

/************************************/
/* Standard includes                */
/************************************/

#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if POSIX || WINDOWS
#define random() rand()
#define srandom(seed) srand(seed)
#endif

/************************************/
/* Constant parameters              */
/************************************/

#define MAXFILENAME 2048
#define BIG 1000000000 /* a number bigger that the possible number of violated clauses */

#define HISTMAX 64         /* length of histogram of tail */
#define MAXATTEMPT 10      /* max number of times to attempt to find a non-tabu variable to flip */
#define denominator 100000 /* denominator used in fractions to represent probabilities */
#define ONE_PERCENT 1000   /* ONE_PERCENT / denominator = 0.01 */

/* #define DEBUG */

/************************************/
/* Main data structures             */
/************************************/

/* Atoms start at 1 */
/* Not a is recorded as -1 * a */
/* One dimensional arrays are statically allocated. */
/* Two dimensional arrays are dynamically allocated in */
/* the second dimension only.  */

int numvars;     /* number of atoms */
int numclauses;   /* number of clauses */
int numliterals; /* number of instances of literals across all clauses */

int numfalse;   /* number of false clauses */
int numfreebie; /* number of freebies */

/* Data structures for clauses */

int **clause; /* clauses to be satisfied */
/* indexed as clause[clause_num][literal_num] */
int *clsize;       /* length of each clause */
int * false_cls;     /* clauses which are false */
int *lowfalse;   /* clauses that are false in the best solution found so far */
int *wherefalse; /* where each clause is listed in false */
int *numtruelit; /* number of true literals in each clause */
int longestclause;

/* Data structures for atoms: arrays of size numvars+1 indexed by atom */

int *atom;         /* value of each atom */
int *lowatom;      /* value of best state found so far */
int *solution;     /* value of solution */
int64_t *changed;   /* step at which atom was last flipped */
int *breakcount;   /* number of clauses that become unsat if var if flipped */
int *makecount;    /* number of clauses that become sat if var if flipped */
int *freebielist;  /* list of freebies */
int *wherefreebie; /* where atom appears in freebies list, -1 if it does not appear */

/* Data structures literals: arrays of size 2*numvars+1, indexed by literal+numvars */

int **occurrence; /* where each literal occurs, size 2*numvars+1            */
/* indexed as occurrence[literal+numvars][occurrence_num] */

int *numoccurrence; /* number of times each literal occurs, size 2*numvars+1  */
/* indexed as numoccurrence[literal+numvars]              */

/* Data structures for lists of clauses used in heuristics */

int *best;
int *besttabu;
int *any;

/************************************/
/* Global flags and parameters      */
/************************************/

/* Options */

FILE *cnfStream;
int status_flag = 0; /* value returned from main procedure */
int abort_flag;
int heuristic; /* heuristic to be used */

int numerator; /* make random flip with numerator/denominator frequency */
double walk_probability = 0.5;
int64_t numflip;        /* number of changes so far */
int numrun = 10;
int64_t cutoff = 100000;
int64_t base_cutoff = 100000;
int target = 0;
int numtry = 0;   /* total attempts at solutions */
int numsol = BIG; /* stop after this many tries succeeds */
int superlinear = false;
int makeflag = false; /* set to true by heuristics that require the make values to be calculated */
char initfile[MAXFILENAME] = {0};
int initoptions = false;

int nofreebie = false;
int maxfreebie = false;
int freebienoise = 0;
double freebienoise_prob = 0.0;

int alternate_greedy = -1;
int alternate_walk = -1;
int alternate_greedy_state = false;
int alternate_run_remaining = 0;

int adaptive = false;        /* update noise level adaptively during search */
int stagnation_timer;        /* number of remaining flips until stagnation check is performed */
int last_adaptive_objective; /* number of unsat clauses last time noise was adaptively updated */
double adaptive_phi;
double adaptive_theta;

/* Random seed */

unsigned int seed; /* Sometimes defined as an unsigned long int */

/* Histogram of tail */

int64_t tailhist[HISTMAX]; /* histogram of num unsat in tail of run */
long histtotal;
int tail = 10;
int tail_start_flip;
int undo_age = 1;
int64_t undo_count;

/* Printing options */

int printonlysol = false;
int printsolcnf = false;
int printhist = false;
int printtrace = false;
int trace_assign = false;
char outfile[MAXFILENAME] = {0};

/* Statistics */

double expertime;
int64_t flips_this_solution;
int lowbad;                  /* lowest number of bad clauses during try */
int64_t totalflip = 0;        /* total number of flips in all tries so far */
int64_t totalsuccessflip = 0; /* total number of flips in all tries which succeeded so far */
int numsuccesstry = 0;       /* total found solutions */
int64_t x;
int64_t integer_sum_x = 0;
double sum_x = 0.0;
double sum_x_squared = 0.0;
double mean_x;
double second_moment_x;
double variance_x;
double std_dev_x;
double std_error_mean_x;
double seconds_per_flip;
int r;
int sum_r = 0;
double sum_r_squared = 0.0;
double mean_r;
double variance_r;
double std_dev_r;
double std_error_mean_r;
double avgfalse;
double sumfalse;
double sumfalse_squared;
double second_moment_avgfalse, variance_avgfalse, std_dev_avgfalse, ratio_avgfalse;
double f;
double sample_size;
double sum_avgfalse = 0.0;
double sum_std_dev_avgfalse = 0.0;
double mean_avgfalse;
double mean_std_dev_avgfalse;
int number_sampled_runs = 0;
double ratio_mean_avgfalse;
double suc_sum_avgfalse = 0.0;
double suc_sum_std_dev_avgfalse = 0.0;
double suc_mean_avgfalse;
double suc_mean_std_dev_avgfalse;
int suc_number_sampled_runs = 0;
double suc_ratio_mean_avgfalse;
double nonsuc_sum_avgfalse = 0.0;
double nonsuc_sum_std_dev_avgfalse = 0.0;
double nonsuc_mean_avgfalse;
double nonsuc_mean_std_dev_avgfalse;
int nonsuc_number_sampled_runs = 0;
double nonsuc_ratio_mean_avgfalse;

/**************************************/
/* Inline utility functions           */
/**************************************/

static inline int ABS(int x)
{
    return x < 0 ? -x : x;
}

static inline int RANDMOD(int x)
{
#if OSX || BSD
    return x > 1 ? random() % x : 0;
#else
    return x > 1 ? random() % x : 0;
#endif
}

static inline int MAX(int x, int y)
{
    return x > y ? x : y;
}

static inline int onfreebielist(int v)
{
    return wherefreebie[v] != -1;
}

static inline void addtofreebielist(int v)
{
    freebielist[numfreebie] = v;
    wherefreebie[v] = numfreebie++;
}

static inline void removefromfreebielist(int v)
{
    int swapv;
    int wherev;

    if (numfreebie < 1 || wherefreebie[v] < 0) {
        fprintf(stderr, "Freebie list error!\n");
        exit(-1);
    }
    numfreebie--;
    wherev = wherefreebie[v];
    wherefreebie[v] = -1;
    if (wherev == numfreebie)
        return;
    swapv = freebielist[numfreebie];
    freebielist[wherev] = swapv;
    wherefreebie[swapv] = wherev;
}

/************************************/
/* Forward declarations             */
/************************************/

void parse_parameters(int argc, char *argv[]);
void print_parameters(int argc, char *argv[]);

int pickrandom(void);
int pickbest(void);
int pickalternate(void);

enum heuristics { RANDOM, BEST};
static int (*pickcode[])(void) = {pickrandom,   pickbest};

int countunsat(void);
void scanone(int argc, char *argv[], int i, int *varptr);
void scanonell(int argc, char *argv[], int i, int64_t *varptr);
void scanoned(int argc, char *argv[], int i, double *varptr);
void init(void);
void initprob(void);
void flipatom(int toflip);
void save_solution(void);
void print_current_assign(void);
void handle_interrupt(int sig);
long super(int i);
void print_sol_file(char *filename);
void print_statistics_header(void);
void initialize_statistics(void);
void update_statistics_start_try(void);
void print_statistics_start_flip(void);
void update_and_print_statistics_end_try(void);
void update_statistics_end_flip(void);
void print_statistics_final(void);
void print_sol_cnf(void);

/************************************/
/* Main                             */
/************************************/

int main(int argc, char *argv[])
{
    int a;

    seed = 0;
    parse_parameters(argc, argv);
    srandom(seed);
    print_parameters(argc, argv);
    initprob();
    initialize_statistics();
    print_statistics_header();
    signal(SIGINT, handle_interrupt);
    abort_flag = false;

    while (!abort_flag && numsuccesstry < numsol && numtry < numrun) {
        numtry++;
        init();
        update_statistics_start_try();
        numflip = 0;
        if (superlinear)
            cutoff = base_cutoff * super(numtry);

        while ((numfalse > target) && (numflip < cutoff)) {
            print_statistics_start_flip();
            numflip++;

            if (maxfreebie && numfreebie > 0 &&
                (freebienoise == 0 || RANDMOD(denominator) > freebienoise))
                a = freebielist[RANDMOD(numfreebie)];
            else
                a = (pickcode[heuristic])();
            flipatom(a);
            update_statistics_end_flip();
        }
        update_and_print_statistics_end_try();
    }
    expertime = cpuTime();
    print_statistics_final();
    return status_flag;
}

void flipatom(int toflip)
{
    int i, j;
    int toenforce;
    int cli;
    int lit;
    int numocc;
    int sz;
    int *litptr;
    int *occptr;
    int temp;

    if (numflip - changed[toflip] <= undo_age)
        undo_count++;

    changed[toflip] = numflip;
    if (atom[toflip] > 0)
        toenforce = -toflip;
    else
        toenforce = toflip;

    atom[toflip] = 1 - atom[toflip];

    numocc = numoccurrence[numvars - toenforce];
    occptr = occurrence[numvars - toenforce];
    for (i = 0; i < numocc; i++) {
        /* cli = occurrence[numvars-toenforce][i]; */
        cli = *(occptr++);

        if (--numtruelit[cli] == 0) {
            false_cls[numfalse] = cli;
            wherefalse[cli] = numfalse;
            numfalse++;
            /* Decrement toflip's breakcount */
            breakcount[toflip]--;

            if (maxfreebie) {
                if (breakcount[toflip] == 0 && makecount[toflip] > 0 && !onfreebielist(toflip))
                    addtofreebielist(toflip);
            }

            if (makeflag) {
                /* Increment the makecount of all vars in the clause */
                sz = clsize[cli];
                litptr = clause[cli];
                for (j = 0; j < sz; j++) {
                    /* lit = clause[cli][j]; */
                    lit = *(litptr++);
                    makecount[ABS(lit)]++;
                    if (maxfreebie) {
                        if (breakcount[ABS(lit)] == 0 && !onfreebielist(ABS(lit)))
                            addtofreebielist(ABS(lit));
                    }
                }
            }
        } else if (numtruelit[cli] == 1) {
            /* Find the lit in this clause that makes it true, and inc its breakcount */
            litptr = clause[cli];
            while (1) {
                /* lit = clause[cli][j]; */
                lit = *(litptr++);
                if ((lit > 0) == atom[ABS(lit)]) {
                    breakcount[ABS(lit)]++;

                    if (maxfreebie) {
                        if (onfreebielist(ABS(lit)))
                            removefromfreebielist(ABS(lit));
                    }

                    /* Swap lit into first position in clause */
                    if ((--litptr) != clause[cli]) {
                        temp = clause[cli][0];
                        clause[cli][0] = *(litptr);
                        *(litptr) = temp;
                    }
                    break;
                }
            }
        }
    }

    numocc = numoccurrence[numvars + toenforce];

    occptr = occurrence[numvars + toenforce];
    for (i = 0; i < numocc; i++) {
        /* cli = occurrence[numvars+toenforce][i]; */
        cli = *(occptr++);

        if (++numtruelit[cli] == 1) {
            numfalse--;
            false_cls[wherefalse[cli]] = false_cls[numfalse];
            wherefalse[false_cls[numfalse]] = wherefalse[cli];
            /* Increment toflip's breakcount */
            breakcount[toflip]++;

            if (maxfreebie) {
                if (onfreebielist(toflip))
                    removefromfreebielist(toflip);
            }

            if (makeflag) {
                /* Decrement the makecount of all vars in the clause */
                sz = clsize[cli];
                litptr = clause[cli];
                for (j = 0; j < sz; j++) {
                    /* lit = clause[cli][j]; */
                    lit = *(litptr++);
                    makecount[ABS(lit)]--;

                    if (maxfreebie) {
                        if (onfreebielist(ABS(lit)) && makecount[ABS(lit)] == 0)
                            removefromfreebielist(ABS(lit));
                    }
                }
            }
        } else if (numtruelit[cli] == 2) {
            /* Find the lit in this clause other than toflip that makes it true,
		 and decrement its breakcount */
            litptr = clause[cli];
            while (1) {
                /* lit = clause[cli][j]; */
                lit = *(litptr++);
                if (((lit > 0) == atom[ABS(lit)]) && (toflip != ABS(lit))) {
                    breakcount[ABS(lit)]--;

                    if (maxfreebie) {
                        if (breakcount[ABS(lit)] == 0 && makecount[ABS(lit)] > 0 &&
                            !onfreebielist(ABS(lit)))
                            addtofreebielist(ABS(lit));
                    }
                    break;
                }
            }
        }
    }
}

/************************************/
/* Initialization                   */
/************************************/

void parse_parameters(int argc, char *argv[])
{
    heuristic = BEST;
    cnfStream = stdin;
    base_cutoff = cutoff;
    if (numsol > numrun)
        numsol = numrun;
    numerator = (int)(walk_probability * denominator);
}

void init(void)
{
    int i;
    int j;
    int var;
    int thetruelit;
    FILE *infile;
    int lit;

    alternate_run_remaining = 0;
    alternate_greedy_state = false;
    if (adaptive) {
        walk_probability = 0.0;
        numerator = (int)(walk_probability * denominator);
        stagnation_timer = (int)(numclauses * adaptive_theta);
        last_adaptive_objective = BIG;
    }

    /* initialize truth assignment and changed time */
    for (i = 0; i < numclauses; i++)
        numtruelit[i] = 0;
    numfalse = 0;
    for (i = 1; i < numvars + 1; i++) {
        changed[i] =
            -i - 1000; /* ties in age between unchanged variables broken for lowest-numbered */
        breakcount[i] = 0;
        makecount[i] = 0;
        atom[i] = RANDMOD(2);
    }

    if (initfile[0]) {
        if ((infile = fopen(initfile, "r")) == NULL) {
            fprintf(stderr, "Cannot open %s\n", initfile);
            exit(1);
        }
        i = 0;
        while (fscanf(infile, " %i", &lit) == 1) {
            i++;
            if (ABS(lit) > numvars) {
                fprintf(stderr, "Bad init file %s\n", initfile);
                exit(1);
            }
            if (lit < 0)
                atom[-lit] = 0;
            else
                atom[lit] = 1;
        }
        if (i == 0) {
            fprintf(stderr, "Bad init file %s\n", initfile);
            exit(1);
        }
        fclose(infile);
    }

    /* Initialize breakcount and makecount */
    for (i = 0; i < numclauses; i++) {
        for (j = 0; j < clsize[i]; j++) {
            if ((clause[i][j] > 0) == atom[ABS(clause[i][j])]) {
                numtruelit[i]++;
                thetruelit = clause[i][j];
            }
        }
        if (numtruelit[i] == 0) {
            wherefalse[i] = numfalse;
            false_cls[numfalse] = i;
            numfalse++;
            for (j = 0; j < clsize[i]; j++) {
                makecount[ABS(clause[i][j])]++;
            }
        } else if (numtruelit[i] == 1) {
            breakcount[ABS(thetruelit)]++;
        }
    }

    /* Create freebie list */
    numfreebie = 0;
    for (var = 1; var <= numvars; var++)
        wherefreebie[var] = -1;
    for (var = 1; var <= numvars; var++) {
        if (makecount[var] > 0 && breakcount[var] == 0) {
            wherefreebie[var] = numfreebie;
            freebielist[numfreebie++] = var;
        }
    }

#ifdef DEBUG
    for (i = 0; i < numfreebie; i++)
        printf(" %d at %d \n", freebielist[i], wherefreebie[freebielist[i]]);
#endif
}

void initprob(void)
{
    int i;
    int j;
    int lastc;
    int nextc;
    int lit;
    int *storebase;
    int storesize;
    int storeused;
    int *storeptr;

    while ((lastc = getc(cnfStream)) == 'c') {
        while ((nextc = getc(cnfStream)) != EOF && nextc != '\n')
            ;
    }
    ungetc(lastc, cnfStream);
    if (fscanf(cnfStream, "p cnf %i %i", &numvars, &numclauses) != 2) {
        fprintf(stderr, "Bad input file\n");
        exit(-1);
    }

    clause = (int **)calloc(sizeof(int *), (numclauses + 1));
    clsize = (int *)calloc(sizeof(int), (numclauses + 1));
    false_cls = (int *)calloc(sizeof(int), (numclauses + 1));
    lowfalse = (int *)calloc(sizeof(int), (numclauses + 1));
    wherefalse = (int *)calloc(sizeof(int), (numclauses + 1));
    numtruelit = (int *)calloc(sizeof(int), (numclauses + 1));

    occurrence = (int **)calloc(sizeof(int *), (2 * numvars + 1));
    numoccurrence = (int *)calloc(sizeof(int), (2 * numvars + 1));
    atom = (int *)calloc(sizeof(int), (numvars + 1));
    lowatom = (int *)calloc(sizeof(int), (numvars + 1));
    solution = (int *)calloc(sizeof(int), (numvars + 1));
    changed = (int64_t *)calloc(sizeof(int64_t), (numvars + 1));
    breakcount = (int *)calloc(sizeof(int), (numvars + 1));
    makecount = (int *)calloc(sizeof(int), (numvars + 1));
    freebielist = (int *)calloc(sizeof(int), (numvars + 1));
    wherefreebie = (int *)calloc(sizeof(int), (numvars + 1));

    numliterals = 0;
    longestclause = 0;

    /* Read in the clauses and set number of occurrences of each literal */
    storesize = 1024;
    storeused = 0;
    printf("Reading formula\n");
    storebase = (int *)calloc(sizeof(int), 1024);

    for (i = 0; i < 2 * numvars + 1; i++)
        numoccurrence[i] = 0;
    for (i = 0; i < numclauses; i++) {
        clsize[i] = 0;
        do {
            if (fscanf(cnfStream, "%i ", &lit) != 1) {
                fprintf(stderr, "Bad input file\n");
                exit(-1);
            }
            if (lit != 0) {
                if (storeused >= storesize) {
                    storeptr = storebase;
                    storebase = (int *)calloc(sizeof(int), storesize * 2);
                    for (j = 0; j < storesize; j++)
                        storebase[j] = storeptr[j];
                    free((void *)storeptr);
                    storesize *= 2;
                }
                clsize[i]++;
                storebase[storeused++] = lit;
                numliterals++;
                numoccurrence[lit + numvars]++;
            }
        } while (lit != 0);
        if (clsize[i] == 0) {
            fprintf(stderr, "Bad input file\n");
            exit(-1);
        }
        longestclause = MAX(longestclause, clsize[i]);
    }

    printf("Creating data structures\n");

    /* Have to wait to set the clause[i] ptrs to the end, since store might move */
    j = 0;
    for (i = 0; i < numclauses; i++) {
        clause[i] = &(storebase[j]);
        j += clsize[i];
    }

    best = (int*) calloc(sizeof(int), longestclause);
    besttabu = (int*) calloc(sizeof(int), longestclause);
    any = (int*) calloc(sizeof(int), longestclause);

    /* Create the occurence lists for each literal */

    /* First, allocate enough storage for occurrence lists */
    storebase = (int *)calloc(sizeof(int), numliterals);

    /* printf("numliterals = %d\n", numliterals); fflush(stdout); */

    /* Second, allocate occurence lists */
    i = 0;
    for (lit = -numvars; lit <= numvars; lit++) {
        /* printf("lit = %d    i = %d\n", lit, i); fflush(stdout); */

        if (lit != 0) {
            if (i > numliterals) {
                fprintf(stderr, "Code error, allocating occurrence lists\n");
                exit(-1);
            }
            occurrence[lit + numvars] = &(storebase[i]);
            i += numoccurrence[lit + numvars];
            numoccurrence[lit + numvars] = 0;
        }
    }

    /* Third, fill in the occurence lists */
    for (i = 0; i < numclauses; i++) {
        for (j = 0; j < clsize[i]; j++) {
            lit = clause[i][j];
            occurrence[lit + numvars][numoccurrence[lit + numvars]] = i;
            numoccurrence[lit + numvars]++;
        }
    }
}

/************************************/
/* Printing and Statistics          */
/************************************/

void print_parameters(int argc, char *argv[])
{
    int i;

    printf("WALKSAT v56");
    printf("command line =");
    for (i = 0; i < argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
    printf("seed = %u\n", seed);
    printf("cutoff = %" BIGFORMAT "\n", cutoff);
    printf("tries = %i\n", numrun);
    printf("walk probabability = %5.3f\n", walk_probability);
    printf("\n");
}

void initialize_statistics(void)
{
    x = 0;
    r = 0;
    tail_start_flip = tail * numvars;
    printf("tail starts after flip = %i\n", tail_start_flip);
}

void print_statistics_header(void)
{
    printf("numvars = %i, numclauses = %i, numliterals = %i\n", numvars, numclauses, numliterals);
    printf("wff read in\n\n");

    printf(
        "    lowbad     unsat       avg   std dev    sd/avg     flips      undo              "
        "length       flips       flips\n");
    printf(
        "      this       end     unsat       avg     ratio      this      flip   success   "
        "success       until         std\n");
    printf(
        "       try       try      tail     unsat      tail       try  fraction      rate     "
        "tries      assign         dev\n\n");

    fflush(stdout);
}

void update_statistics_start_try(void)
{
    int i;

    lowbad = numfalse;
    undo_count = 0;

    sample_size = 0;
    sumfalse = 0.0;
    sumfalse_squared = 0.0;

    for (i = 0; i < HISTMAX; i++)
        tailhist[i] = 0;
    if (tail_start_flip == 0) {
        tailhist[numfalse < HISTMAX ? numfalse : HISTMAX - 1]++;
    }
}

void print_statistics_start_flip(void)
{
    if (printtrace && (numflip % printtrace == 0)) {
        printf(" %9i %9i                     %9" BIGFORMAT "\n", lowbad, numfalse, numflip);
        if (trace_assign)
            print_current_assign();
        fflush(stdout);
    }
}

void update_statistics_end_flip(void)
{
    if (adaptive) {
        /* Reference for adaptie noise option:
	   An Adaptive Noise Mechanism for WalkSAT (Corrected). Holger H. Hoos.
	*/

        if (numfalse < last_adaptive_objective) {
            last_adaptive_objective = numfalse;
            stagnation_timer = (int)(numclauses * adaptive_theta);
            numerator = (int)((1.0 - adaptive_phi / 2.0) * numerator);
        } else {
            stagnation_timer = stagnation_timer - 1;
            if (stagnation_timer <= 0) {
                last_adaptive_objective = numfalse;
                stagnation_timer = (int)(numclauses * adaptive_theta);
                numerator = numerator + (int)((denominator - numerator) * adaptive_phi);
            }
        }
    }

    if (numfalse < lowbad) {
        lowbad = numfalse;
    }
    if (numflip >= tail_start_flip) {
        tailhist[(numfalse < HISTMAX) ? numfalse : (HISTMAX - 1)]++;
        sumfalse += numfalse;
        sumfalse_squared += numfalse * numfalse;
        sample_size++;
    }
}

void update_and_print_statistics_end_try(void)
{
    int i;
    double undo_fraction;

    totalflip += numflip;
    x += numflip;
    r++;

    if (sample_size > 0) {
        avgfalse = sumfalse / sample_size;
        second_moment_avgfalse = sumfalse_squared / sample_size;
        variance_avgfalse = second_moment_avgfalse - (avgfalse * avgfalse);
        if (sample_size > 1) {
            variance_avgfalse = (variance_avgfalse * sample_size) / (sample_size - 1);
        }
        std_dev_avgfalse = sqrt(variance_avgfalse);

        ratio_avgfalse = avgfalse / std_dev_avgfalse;

        sum_avgfalse += avgfalse;
        sum_std_dev_avgfalse += std_dev_avgfalse;
        number_sampled_runs += 1;

        if (numfalse <= target) {
            suc_number_sampled_runs += 1;
            suc_sum_avgfalse += avgfalse;
            suc_sum_std_dev_avgfalse += std_dev_avgfalse;
        } else {
            nonsuc_number_sampled_runs += 1;
            nonsuc_sum_avgfalse += avgfalse;
            nonsuc_sum_std_dev_avgfalse += std_dev_avgfalse;
        }
    } else {
        avgfalse = 0;
        variance_avgfalse = 0;
        std_dev_avgfalse = 0;
        ratio_avgfalse = 0;
    }

    if (numfalse <= target) {
        status_flag = 0;
        save_solution();
        numsuccesstry++;
        totalsuccessflip += numflip;
        integer_sum_x += x;
        sum_x = (double)integer_sum_x;
        sum_x_squared += ((double)x) * ((double)x);
        mean_x = sum_x / numsuccesstry;
        if (numsuccesstry > 1) {
            second_moment_x = sum_x_squared / numsuccesstry;
            variance_x = second_moment_x - (mean_x * mean_x);
            /* Adjustment for small small sample size */
            variance_x = (variance_x * numsuccesstry) / (numsuccesstry - 1);
            std_dev_x = sqrt(variance_x);
            std_error_mean_x = std_dev_x / sqrt((double)numsuccesstry);
        }
        sum_r += r;
        mean_r = ((double)sum_r) / numsuccesstry;
        sum_r_squared += ((double)r) * ((double)r);
        x = 0;
        r = 0;
    }

    undo_fraction = ((double)undo_count) / numflip;

    printf(" %9i %9i %9.2f %9.2f %9.2f %9" BIGFORMAT " %9.6f %9i", lowbad, numfalse, avgfalse,
           std_dev_avgfalse, ratio_avgfalse, numflip, undo_fraction,
           (numsuccesstry * 100) / numtry);
    if (numsuccesstry > 0) {
        printf(" %9" BIGFORMAT, totalsuccessflip / numsuccesstry);
        printf(" %11.2f", mean_x);
        if (numsuccesstry > 1) {
            printf(" %11.2f", std_dev_x);
        }
    }
    printf("\n");

    if (printhist) {
        printf("histogram: ");
        for (i = 0; i < HISTMAX; i++) {
            printf(" %" BIGFORMAT "(%i)", tailhist[i], i);
            if ((i + 1) % 10 == 0)
                printf("\n           ");
        }
        printf("\n");
    }

    if (numfalse == 0 && countunsat() != 0) {
        fprintf(stderr, "Program error, verification of solution fails!\n");
        exit(-1);
    }

    fflush(stdout);
}

void print_statistics_final(void)
{
    seconds_per_flip = expertime / totalflip;
    printf("\ntotal elapsed seconds = %f\n", expertime);
    printf("num tries: %d\n", numtry);
    printf("average flips per second = %f\n", ((double)totalflip) / expertime);
    printf("number solutions found = %i\n", numsuccesstry);
    printf("final success rate = %f\n", ((double)numsuccesstry * 100.0) / numtry);
    printf("average length successful tries = %" BIGFORMAT "\n",
           numsuccesstry ? (totalsuccessflip / numsuccesstry) : 0);
    if (numsuccesstry > 0) {
        printf("average flips per assign (over all runs) = %f\n",
               ((double)totalflip) / numsuccesstry);
        printf("average seconds per assign (over all runs) = %f\n",
               (((double)totalflip) / numsuccesstry) * seconds_per_flip);
        printf("mean flips until assign = %f\n", mean_x);
        if (numsuccesstry > 1) {
            printf("  variance = %f\n", variance_x);
            printf("  standard deviation = %f\n", std_dev_x);
            printf("  standard error of mean = %f\n", std_error_mean_x);
        }
        printf("mean seconds until assign = %f\n", mean_x * seconds_per_flip);
        if (numsuccesstry > 1) {
            printf("  variance = %f\n", variance_x * seconds_per_flip * seconds_per_flip);
            printf("  standard deviation = %f\n", std_dev_x * seconds_per_flip);
            printf("  standard error of mean = %f\n", std_error_mean_x * seconds_per_flip);
        }
        printf("mean restarts until assign = %f\n", mean_r);
        if (numsuccesstry > 1) {
            variance_r = (sum_r_squared / numsuccesstry) - (mean_r * mean_r);
            if (numsuccesstry > 1)
                variance_r = (variance_r * numsuccesstry) / (numsuccesstry - 1);
            std_dev_r = sqrt(variance_r);
            std_error_mean_r = std_dev_r / sqrt((double)numsuccesstry);
            printf("  variance = %f\n", variance_r);
            printf("  standard deviation = %f\n", std_dev_r);
            printf("  standard error of mean = %f\n", std_error_mean_r);
        }
    }

    if (number_sampled_runs) {
        mean_avgfalse = sum_avgfalse / number_sampled_runs;
        mean_std_dev_avgfalse = sum_std_dev_avgfalse / number_sampled_runs;
        ratio_mean_avgfalse = mean_avgfalse / mean_std_dev_avgfalse;

        if (suc_number_sampled_runs) {
            suc_mean_avgfalse = suc_sum_avgfalse / suc_number_sampled_runs;
            suc_mean_std_dev_avgfalse = suc_sum_std_dev_avgfalse / suc_number_sampled_runs;
            suc_ratio_mean_avgfalse = suc_mean_avgfalse / suc_mean_std_dev_avgfalse;
        } else {
            suc_mean_avgfalse = 0;
            suc_mean_std_dev_avgfalse = 0;
            suc_ratio_mean_avgfalse = 0;
        }

        if (nonsuc_number_sampled_runs) {
            nonsuc_mean_avgfalse = nonsuc_sum_avgfalse / nonsuc_number_sampled_runs;
            nonsuc_mean_std_dev_avgfalse = nonsuc_sum_std_dev_avgfalse / nonsuc_number_sampled_runs;
            nonsuc_ratio_mean_avgfalse = nonsuc_mean_avgfalse / nonsuc_mean_std_dev_avgfalse;
        } else {
            nonsuc_mean_avgfalse = 0;
            nonsuc_mean_std_dev_avgfalse = 0;
            nonsuc_ratio_mean_avgfalse = 0;
        }

        printf("final numbad level statistics\n");
        printf("    statistics over all runs:\n");
        printf("      overall mean average numbad = %f\n", mean_avgfalse);
        printf("      overall mean meanbad std deviation = %f\n", mean_std_dev_avgfalse);
        printf("      overall ratio mean numbad to mean std dev = %f\n", ratio_mean_avgfalse);
        printf("    statistics on successful runs:\n");
        printf("      successful mean average numbad = %f\n", suc_mean_avgfalse);
        printf("      successful mean numbad std deviation = %f\n", suc_mean_std_dev_avgfalse);
        printf("      successful ratio mean numbad to mean std dev = %f\n",
               suc_ratio_mean_avgfalse);
        printf("    statistics on nonsuccessful runs:\n");
        printf("      nonsuccessful mean average numbad level = %f\n", nonsuc_mean_avgfalse);
        printf("      nonsuccessful mean numbad std deviation = %f\n",
               nonsuc_mean_std_dev_avgfalse);
        printf("      nonsuccessful ratio mean numbad to mean std dev = %f\n",
               nonsuc_ratio_mean_avgfalse);
    }

    if (numsuccesstry > 0) {
        printf("ASSIGNMENT FOUND\n");
        if (printsolcnf == true)
            print_sol_cnf();
        if (outfile[0])
            print_sol_file(outfile);
    } else
        printf("ASSIGNMENT NOT FOUND\n");
}

void print_sol_cnf(void)
{
    int i;
    for (i = 1; i < numvars + 1; i++)
        printf("v %i\n", solution[i] == 1 ? i : -i);
}

void print_sol_file(char *filename)
{
    FILE *fp;
    int i;

    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "Cannot open output file\n");
        exit(-1);
    }
    for (i = 1; i < numvars + 1; i++) {
        fprintf(fp, " %i", solution[i] == 1 ? i : -i);
        if (i % 10 == 0)
            fprintf(fp, "\n");
    }
    if ((i - 1) % 10 != 0)
        fprintf(fp, "\n");
    fclose(fp);
}

void print_current_assign(void)
{
    int i;

    printf("Begin assign at flip = %" BIGFORMAT "\n", numflip);
    for (i = 1; i <= numvars; i++) {
        printf(" %i", atom[i] == 0 ? -i : i);
        if (i % 10 == 0)
            printf("\n");
    }
    if ((i - 1) % 10 != 0)
        printf("\n");
    printf("End assign\n");
}

void save_solution(void)
{
    int i;

    for (i = 1; i <= numvars; i++)
        solution[i] = atom[i];
}

/*******************************************************/
/* Utility Functions                                   */
/*******************************************************/

long super(int i)
{
    long power;
    int k;

    if (i <= 0) {
        fprintf(stderr, "bad argument super(%i)\n", i);
        exit(1);
    }
    /* let 2^k be the least power of 2 >= (i+1) */
    k = 1;
    power = 2;
    while (power < (i + 1)) {
        k += 1;
        power *= 2;
    }
    if (power == (i + 1))
        return (power / 2);
    return (super(i - (power / 2) + 1));
}

void handle_interrupt(int /*signal*/)
{
    if (abort_flag)
        exit(-1);
    abort_flag = true;
}

void scanone(int argc, char *argv[], int i, int *varptr)
{
    int64_t n;
    scanonell(argc, argv, i, &n);
    *varptr = (int)n;
}

void scanoned(int argc, char *argv[], int i, double *varptr)
{
    if (i >= argc || sscanf(argv[i], "%lf", varptr) != 1) {
        fprintf(stderr, "Bad argument %s\n", i < argc ? argv[i] : argv[argc - 1]);
        exit(-1);
    }
}

void scanonell(int argc, char *argv[], int i, int64_t *varptr)
{
    char buf[25];
    int factor = 1;
    int64_t n;

    if (i >= argc || strlen(argv[i]) > 24) {
        fprintf(stderr, "Bad argument %s\n", i < argc ? argv[i] : argv[argc - 1]);
        exit(-1);
    }
    strcpy(buf, argv[i]);
    switch (buf[strlen(buf) - 1]) {
        case 'K':
            factor = 1000;
            buf[strlen(buf) - 1] = 0;
            break;
        case 'M':
            factor = 1000000;
            buf[strlen(buf) - 1] = 0;
            break;
        case 'B':
            factor = 1000000000;
            buf[strlen(buf) - 1] = 0;
            break;
    }
    if (sscanf(argv[i], "%" BIGFORMAT, &n) != 1) {
        fprintf(stderr, "Bad argument %s\n", i < argc ? argv[i] : argv[argc - 1]);
        exit(-1);
    }
    n = n * factor;
    *varptr = n;
}

int countunsat(void)
{
    int i, j, unsat, bad, lit, sign;

    unsat = 0;
    for (i = 0; i < numclauses; i++) {
        bad = true;
        for (j = 0; j < clsize[i]; j++) {
            lit = clause[i][j];
            sign = lit > 0 ? 1 : 0;
            if (atom[ABS(lit)] == sign) {
                bad = false;
                break;
            }
        }
        if (bad)
            unsat++;
    }
    return unsat;
}

/****************************************************************/
/*                  Heuristics                                  */
/****************************************************************/

int pickrandom(void)
{
    int tofix;

    tofix = false_cls[RANDMOD(numfalse)];
    return ABS(clause[tofix][RANDMOD(tofix)]);
}

int pickbest(void)
{
    int numbreak;
    int tofix;
    int clausesize;
    int i;
    int numbest;
    int bestvalue;
    int var;

    tofix = false_cls[RANDMOD(numfalse)];
    clausesize = clsize[tofix];
    numbest = 0;
    bestvalue = BIG;

    for (i = 0; i < clausesize; i++) {
        var = ABS(clause[tofix][i]);
        numbreak = breakcount[var];
        if (numbreak <= bestvalue) {
            if (numbreak < bestvalue)
                numbest = 0;
            bestvalue = numbreak;
            best[numbest++] = var;
        }
    }

    if ((nofreebie || bestvalue > 0) && (RANDMOD(denominator) < numerator))
        return ABS(clause[tofix][RANDMOD(clausesize)]);
    return ABS(best[RANDMOD(numbest)]);
}
