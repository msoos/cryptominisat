/******************************************
Copyright (c) 2018, Henry Kautz <henry.kautz@gmail.com>
Copyright (c) 2018, Mate Soos <soos.mate@gmail.com>

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

#include "time_mem.h"
#include <limits>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "walksat.h"

#define denominator 100000 /* denominator used in fractions to represent probabilities */
using namespace CMSat;

uint32_t WalkSAT::RANDMOD(uint32_t d)
{
    return d > 1 ? mtrand.randInt(d-1) : 0;
}

static inline int MAX(int x, int y)
{
    return x > y ? x : y;
}

WalkSAT::WalkSAT()
{
    //test
}

WalkSAT::~WalkSAT()
{
    free(storebase);
    free(clause);
    free(clsize);

    free(false_cls);
    free(map_cl_to_false_cls);
    free(numtruelit);

    free(occur_list_alloc);
    free(occurrence);
    free(numoccurrence);
    free(assigns);
    free(breakcount);
    free(best);
}


int WalkSAT::main()
{
    parse_parameters();
    mtrand.seed(1U);
    print_parameters();
    init_problem();
    initialize_statistics();
    print_statistics_header();

    while (!found_solution && numtry < numrun) {
        numtry++;
        init();
        update_statistics_start_try();
        numflip = 0;

        while ((numfalse > 0) && (numflip < cutoff)) {
            numflip++;

            uint32_t var = pickbest();
            flipvar(var);
            update_statistics_end_flip();
        }
        update_and_print_statistics_end_try();
    }
    expertime = cpuTime();
    print_statistics_final();
    return found_solution;
}

void WalkSAT::WalkSAT::flipvar(uint32_t toflip)
{
    Lit toenforce;
    uint32_t numocc;

    if (assigns[toflip] == l_True)
        toenforce = Lit(toflip, true);
    else
        toenforce = Lit(toflip, false);

    assert(value(toflip) != l_Undef);
    assigns[toflip] = assigns[toflip] ^ true;

    //True made into False
    numocc = numoccurrence[(~toenforce).toInt()];
    for (uint32_t i = 0; i < numocc; i++) {
        uint32_t cli = occurrence[(~toenforce).toInt()][i];

        assert(numtruelit[cli] > 0);
        numtruelit[cli]--;
        if (numtruelit[cli] == 0) {
            false_cls[numfalse] = cli;
            map_cl_to_false_cls[cli] = numfalse;
            numfalse++;
            /* Decrement toflip's breakcount */
            breakcount[toflip]--;
        } else if (numtruelit[cli] == 1) {
            /* Find the lit in this clause that makes it true, and inc its breakcount */
            Lit *litptr = clause[cli];
            while (1) {
                /* lit = clause[cli][j]; */
                Lit lit = *(litptr++);
                if (value(lit) == l_True) {
                    breakcount[lit.var()]++;

                    /* Swap lit into first position in clause */
                    if ((--litptr) != clause[cli]) {
                        Lit temp = clause[cli][0];
                        clause[cli][0] = *(litptr);
                        *(litptr) = temp;
                    }
                    break;
                }
            }
        }
    }

    //made into TRUE
    numocc = numoccurrence[toenforce.toInt()];
    for (uint32_t i = 0; i < numocc; i++) {
        uint32_t cli = occurrence[toenforce.toInt()][i];

        numtruelit[cli]++;
        if (numtruelit[cli] == 1) {
            const uint32_t last_false_cl = false_cls[numfalse-1];
            uint32_t position_in_false_cls = map_cl_to_false_cls[cli];
            numfalse--;

            //the postiion in false_cls where this clause was is now replaced with
            //the one at the end
            false_cls[position_in_false_cls] = last_false_cl;

            //update map_cl_to_false_cls of the clause
            map_cl_to_false_cls[last_false_cl] = position_in_false_cls;

            /* Increment toflip's breakcount */
            breakcount[toflip]++;
        } else if (numtruelit[cli] == 2) {
            /* Find the lit in this clause other than toflip that makes it true,
             * and decrement its breakcount */
            Lit *litptr = clause[cli];
            while (1) {
                /* lit = clause[cli][j]; */
                Lit lit = *(litptr++);
                if (value(lit) == l_True && (toflip != lit.var())) {
                    assert(breakcount[lit.var()] > 0);
                    breakcount[lit.var()]--;
                    break;
                }
            }
        }
    }
}

/************************************/
/* Initialization                   */
/************************************/

void WalkSAT::parse_parameters()
{
    cnfStream = stdin;
    base_cutoff = cutoff;
    numerator = (int)(walk_probability * denominator);
}

void WalkSAT::init()
{
    /* initialize truth assignment and changed time */
    for (uint32_t i = 0; i < numclauses; i++)
        numtruelit[i] = 0;

    numfalse = 0;
    for (uint32_t i = 0; i < numvars; i++) {
        breakcount[i] = 0;
        assigns[i] = RANDMOD(2)==0 ? l_False : l_True;
    }

    /* Initialize breakcount  */
    for (uint32_t i = 0; i < numclauses; i++) {
        Lit thetruelit;
        for (uint32_t j = 0; j < clsize[i]; j++) {
            if (value(clause[i][j]) == l_True) {
                numtruelit[i]++;
                thetruelit = clause[i][j];
            }
        }
        if (numtruelit[i] == 0) {
            map_cl_to_false_cls[i] = numfalse;
            false_cls[numfalse] = i;
            numfalse++;
        } else if (numtruelit[i] == 1) {
            breakcount[thetruelit.var()]++;
        }
    }
}

void WalkSAT::init_problem()
{
    uint32_t i;
    uint32_t j;
    int lastc;
    int nextc;
    uint32_t storesize;
    uint32_t storeused;

    //skip header
    while ((lastc = getc(cnfStream)) == 'c') {
        while ((nextc = getc(cnfStream)) != EOF && nextc != '\n')
            ;
    }
    ungetc(lastc, cnfStream);
    if (fscanf(cnfStream, "p cnf %i %i", &numvars, &numclauses) != 2) {
        cout << "Bad input file" << endl;
        exit(-1);
    }

    clause = (Lit **)calloc(sizeof(Lit *), numclauses);
    clsize = (uint32_t *)calloc(sizeof(uint32_t), numclauses);

    //false-true lits
    false_cls = (uint32_t *)calloc(sizeof(uint32_t), numclauses);
    map_cl_to_false_cls = (uint32_t *)calloc(sizeof(uint32_t), numclauses);
    numtruelit = (uint32_t *)calloc(sizeof(uint32_t), numclauses);

    occurrence = (uint32_t **)calloc(sizeof(uint32_t *), (2 * numvars));
    numoccurrence = (uint32_t *)calloc(sizeof(uint32_t), (2 * numvars));
    assigns = (lbool *)calloc(sizeof(lbool), numvars);
    breakcount = (uint32_t *)calloc(sizeof(uint32_t), numvars);

    numliterals = 0;
    longestclause = 0;

    /* Read in the clauses and set number of occurrences of each literal */
    storesize = 1024;
    storeused = 0;
    cout << "Reading formula" << endl;

    for (i = 0; i < 2 * numvars; i++)
        numoccurrence[i] = 0;

    //where all clauses' literals are
    storebase = (Lit *)malloc(1024*sizeof(Lit));
    for (i = 0; i < numclauses; i++) {
        clsize[i] = 0;
        int lit;
        do {
            if (fscanf(cnfStream, "%i ", &lit) != 1) {
                cout << "Bad input file" << endl;
                exit(-1);
            }
            if (lit != 0) {
                if (storeused >= storesize) {
                    storesize *= 2;
                    storebase = (Lit *)realloc(storebase, storesize * 2*sizeof(Lit));
                }
                clsize[i]++;
                const uint32_t var = std::abs(lit)-1;
                Lit real_lit = (lit > 0) ? Lit(var, false) : Lit(var, true);
                storebase[storeused++] = real_lit;
                numliterals++;
                numoccurrence[real_lit.toInt()]++;
            }
        } while (lit != 0);

        if (clsize[i] == 0) {
            cout << "Bad input file" << endl;
            exit(-1);
        }
        longestclause = MAX(longestclause, clsize[i]);
    }

    cout << "Creating data structures" << endl;

    /* Have to wait to set the clause[i] ptrs to the end, since store might move */
    j = 0;
    for (i = 0; i < numclauses; i++) {
        clause[i] = &(storebase[j]);
        j += clsize[i];
    }
    best = (uint32_t*) calloc(sizeof(uint32_t), longestclause);


    /* allocate occurence lists */
    occur_list_alloc = (uint32_t *)calloc(sizeof(uint32_t), numliterals);
    i = 0;
    for (uint32_t i2 = 0; i2 < numvars*2; i2++) {
        const Lit lit = Lit::toLit(i2);
        if (i > numliterals) {
            cout << "Code error, allocating occurrence lists" << endl;
            exit(-1);
        }
        occurrence[lit.toInt()] = &(occur_list_alloc[i]);
        i += numoccurrence[lit.toInt()];
        numoccurrence[lit.toInt()] = 0;
    }

    /* Third, fill in the occurence lists */
    for (i = 0; i < numclauses; i++) {
        for (j = 0; j < clsize[i]; j++) {
            Lit lit = clause[i][j];
            occurrence[lit.toInt()][numoccurrence[lit.toInt()]] = i;
            numoccurrence[lit.toInt()]++;
        }
    }
}

/************************************/
/* Printing and Statistics          */
/************************************/

void WalkSAT::print_parameters()
{
    cout << "WALKSAT v56" << endl;
    cout << "cutoff = %" << cutoff << endl;
    cout << "tries = " << numrun << endl;
    cout << "walk probabability = "
    << std::fixed << std::setprecision(2) << walk_probability << endl;
    cout << endl;
}

void WalkSAT::initialize_statistics()
{
    x = 0;
    r = 0;
    tail_start_flip = tail * numvars;
    cout << "tail starts after flip = " << tail_start_flip << endl;
}

void WalkSAT::print_statistics_header()
{
    cout << "numvars = " << numvars << ", numclauses = "
    << numclauses << ", numliterals = " << numliterals;

    cout << "wff read in\n" << endl;
    cout <<
        "    lowbad     unsat       avg   std dev    sd/avg     flips      undo              "
        "length       flips" << endl;
    cout <<
        "      this       end     unsat       avg     ratio      this      flip   success   "
        "success       until" << endl;
    cout <<
        "       try       try      tail     unsat      tail       try  fraction      rate     "
        "tries      assign" << endl;
    cout << endl;
}

void WalkSAT::update_statistics_start_try()
{
    lowbad = numfalse;
    sample_size = 0;
    sumfalse = 0.0;
    sumfalse_squared = 0.0;
}

void WalkSAT::update_statistics_end_flip()
{
    if (numfalse < lowbad) {
        lowbad = numfalse;
    }
    if (numflip >= tail_start_flip) {
        sumfalse += numfalse;
        sumfalse_squared += numfalse * numfalse;
        sample_size++;
    }
}

void WalkSAT::update_and_print_statistics_end_try()
{
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

        if (numfalse == 0) {
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

    if (numfalse == 0) {
        found_solution = true;
        totalsuccessflip += numflip;
        integer_sum_x += x;
        sum_x = (double)integer_sum_x;
        mean_x = sum_x / found_solution;
        sum_r += r;
        mean_r = ((double)sum_r) / (double)found_solution;
        x = 0;
        r = 0;
    }

    //MSOOS: this has been removed, uses memory, only stats
    double undo_fraction = 0;

    cout
    << std::setw(9) << lowbad
    << std::setw(9) << numfalse
    << std::setw(9+2) << avgfalse
    << std::setw(9+2) << std_dev_avgfalse
    << std::setw(9+2) << ratio_avgfalse
    << std::setw(9) << numflip
    << std::setw(9) << undo_fraction
    << std::setw(9+2) << (((int)found_solution * 100) / numtry);
    if (found_solution) {
        cout << std::setw(9+2) << totalsuccessflip / (int)found_solution;
        cout << std::setw(9+2) << mean_x;
    }
    cout << endl;

    if (numfalse == 0 && countunsat() != 0) {
        cout << "Program error, verification of solution fails!" << endl;
        exit(-1);
    }

    fflush(stdout);
}

void WalkSAT::print_statistics_final()
{
    seconds_per_flip = expertime / totalflip;
    cout << "\ntotal elapsed seconds = " <<  expertime << endl;
    cout << "num tries: " <<  numtry  << endl;
    cout << "average flips per second = " << ((double)totalflip) / expertime << endl;
    cout << "number solutions found = " << found_solution << endl;
    cout << "final success rate = " << ((double)found_solution * 100.0) / numtry  << endl;
    cout << "average length successful tries = %" <<
           (found_solution ? (totalsuccessflip / found_solution) : 0) << endl;
    if (found_solution) {
        cout << "average flips per assign (over all runs) = " <<
               ((double)totalflip) / found_solution << endl;
        cout << "average seconds per assign (over all runs) = " <<
               (((double)totalflip) / found_solution) * seconds_per_flip << endl;
        cout << "mean flips until assign = " << mean_x << endl;
        cout << "mean seconds until assign = " << mean_x * seconds_per_flip << endl;
        cout << "mean restarts until assign = " << mean_r << endl;
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

        cout << "final numbad level statistics"  << endl;
        cout << "    statistics over all runs:"  << endl;
        cout << "      overall mean average numbad = " << mean_avgfalse << endl;
        cout << "      overall mean meanbad std deviation = " << mean_std_dev_avgfalse << endl;
        cout << "      overall ratio mean numbad to mean std dev = " << ratio_mean_avgfalse << endl;
        cout << "    statistics on successful runs:"  << endl;
        cout << "      successful mean average numbad = " << suc_mean_avgfalse << endl;
        cout << "      successful mean numbad std deviation = " << suc_mean_std_dev_avgfalse << endl;
        cout << "      successful ratio mean numbad to mean std dev = " <<
               suc_ratio_mean_avgfalse  << endl;
        cout << "    statistics on nonsuccessful runs:"  << endl;
        cout << "      nonsuccessful mean average numbad level = " << nonsuc_mean_avgfalse  << endl;
        cout << "      nonsuccessful mean numbad std deviation = " <<
               nonsuc_mean_std_dev_avgfalse  << endl;
        cout << "      nonsuccessful ratio mean numbad to mean std dev = " <<
               nonsuc_ratio_mean_avgfalse  << endl;
    }

    if (found_solution) {
        cout << "ASSIGNMENT FOUND"  << endl;
        print_sol_cnf();
    } else
        cout << "ASSIGNMENT NOT FOUND"  << endl;
}

void WalkSAT::print_sol_cnf()
{
    cout << "v ";
    for (uint32_t i = 0; i < numvars; i++) {
         cout << (assigns[i] == l_True? ((int)i+1) : -1*((int)i+1)) << " ";
    }
    cout << "0" << endl;
}

/*******************************************************/
/* Utility Functions                                   */
/*******************************************************/
//ONLY used for checking solution
uint32_t WalkSAT::countunsat()
{
    uint32_t unsat = 0;
    for (uint32_t i = 0; i < numclauses; i++) {
        bool bad = true;
        for (uint32_t j = 0; j < clsize[i]; j++) {
            Lit lit = clause[i][j];
            if (value(lit) == l_True) {
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

uint32_t WalkSAT::pickbest()
{
    uint32_t tofix;
    uint32_t clausesize;

    //pick a random false clause to fix
    tofix = false_cls[RANDMOD(numfalse)];
    clausesize = clsize[tofix];

    //pick the literal to flip in this clause
    uint32_t numbest = 0;
    uint32_t bestvalue = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < clausesize; i++) {
        uint32_t var = clause[tofix][i].var();
        uint32_t numbreak = breakcount[var];
        if (numbreak <= bestvalue) {
            if (numbreak < bestvalue)
                numbest = 0;
            bestvalue = numbreak;
            best[numbest++] = var;
        }
    }

    //in case there is no literal where the best break is 0 (i.e. free flip)
    //then half of the time we pick a random literal to flip

    /* walk probability is 0.5, and
       numerator = (int)(walk_probability * denominator); */
    if ((bestvalue > 0) && (RANDMOD(denominator) < numerator))
        return clause[tofix][RANDMOD(clausesize)].var();

    //pick one of the best (least breaking one) to flip
    return best[RANDMOD(numbest)];
}
