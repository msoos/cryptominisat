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
#include "solver.h"

#define denominator 100000 /* denominator used in fractions to represent probabilities */
using namespace CMSat;

uint32_t WalkSAT::RANDMOD(uint32_t d)
{
    return d > 1 ? solver->mtrand.randInt(d-1) : 0;
}

WalkSAT::WalkSAT(Solver* _solver) :
    solver(_solver)
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

lbool WalkSAT::main()
{
    parse_parameters();
    mtrand.seed(1U);
    print_parameters();
    init_problem();
    initialize_statistics();
    print_statistics_header();
    startTime = cpuTime();

    while (!found_solution && numtry < max_runs) {
        numtry++;
        init();
        update_statistics_start_try();
        numflip = 0;

        while (!found_solution && (numfalse > 0) && (numflip < cutoff)) {
            numflip++;

            uint32_t var = pickbest();
            flipvar(var);
            update_statistics_end_flip();
        }
        update_and_print_statistics_end_try();
    }
    print_statistics_final();
    if (found_solution)
        return l_True;
    else
        return l_Undef;
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
    base_cutoff = cutoff;
    numerator = (int)(walk_probability * denominator);
}

void WalkSAT::init()
{
    assert(solver->decisionLevel() == 0);
    assert(solver->okay());

    /* initialize truth assignment and changed time */
    for (uint32_t i = 0; i < numclauses; i++)
        numtruelit[i] = 0;

    numfalse = 0;
    for (uint32_t i = 0; i < numvars; i++) {
        breakcount[i] = 0;
        if (solver->value(i) != l_Undef) {
            assigns[i] = solver->value(i);
        } else {
            assigns[i] = solver->varData[i].polarity ? l_True: l_False;
        }
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

uint64_t WalkSAT::mem_needed()
{
    uint64_t needed = 0;

    //LIT storage (all clause data)
    needed += (solver->litStats.irredLits+solver->binTri.irredBins*2)*sizeof(Lit);

    //NOTE: this is underreporting here, but by VERY little
    //best -> longestclause = ??
    //needed += sizeof(uint32_t) * longestclause;

    //clause
    needed += sizeof(Lit *) * numclauses;
    //clsize
    needed += sizeof(uint32_t) * numclauses;

    //false_cls
    needed += sizeof(uint32_t) * numclauses;
    //map_cl_to_false_cls
    needed += sizeof(uint32_t) * numclauses;
    //numtruelit
    needed += sizeof(uint32_t) * numclauses;

    //occurrence
    needed += sizeof(uint32_t *) * (2 * numvars);
    //numoccurrence
    needed += sizeof(uint32_t) * (2 * numvars);
    //assigns
    needed += sizeof(lbool) * numvars;
    //breakcount
    needed += sizeof(uint32_t) * numvars;

    //occur_list_alloc
    needed += sizeof(uint32_t) * numliterals;

    return needed;
}

void WalkSAT::init_problem()
{
    uint32_t i;
    uint32_t j;
    //TODO simplify by the assumptions!
    //Then we will automatically get the right solution if we get one :)

    numvars = solver->nVars();
    numclauses = solver->longIrredCls.size() + solver->binTri.irredBins;

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
    uint32_t storeused = 0;
    for (i = 0; i < 2 * numvars; i++)
        numoccurrence[i] = 0;

    //where all clauses' literals are
    solver->check_stats();
    const uint32_t storesize = solver->litStats.irredLits+solver->binTri.irredBins*2;
    storebase = (Lit *)malloc(storesize*sizeof(Lit));
    i = 0;
    for(ClOffset offs: solver->longIrredCls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
        assert(!cl->getRemoved());
        assert(storeused+cl->size() <= storesize);

        clause[i] = storebase + storeused;
        clsize[i] = cl->size();
        numliterals += cl->size();

        for(Lit l: *cl) {
            storebase[storeused++] = l;
            numoccurrence[l.toInt()]++;
        }
        longestclause = std::max(longestclause, clsize[i]);
        i++;
    }
    for(size_t i2 = 0; i2 < solver->nVars()*2; i2++) {
        Lit lit = Lit::toLit(i2);
        for(const Watched& w: solver->watches[lit]) {
            if (w.isBin() && !w.red() && lit < w.lit2()) {
                assert(storeused+2 <= storesize);

                clause[i] = storebase + storeused;
                clsize[i] = 2;
                numliterals += 2;

                storebase[storeused++] = lit;
                storebase[storeused++] = w.lit2();
                numoccurrence[lit.toInt()]++;
                numoccurrence[w.lit2().toInt()]++;
                longestclause = std::max(longestclause, clsize[i]);
                i++;
            }
        }
    }
    assert(storeused == storesize);
    assert(i == numclauses);

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
    assert(i == numliterals);

    /* Third, fill in the occurence lists */
    for (i = 0; i < numclauses; i++) {
        for (j = 0; j < clsize[i]; j++) {
            const Lit lit = clause[i][j];
            assert(lit.var() < numvars);

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
    cout << "c [walksat] WALKSAT v56" << endl;
    cout << "c [walksat] cutoff = %" << cutoff << endl;
    cout << "c [walksat] tries = " << max_runs << endl;
    cout << "c [walksat] walk probabability = "
    << std::fixed << std::setprecision(2) << walk_probability << endl;
}

void WalkSAT::initialize_statistics()
{
    x = 0;
    r = 0;
    tail_start_flip = tail * numvars;
    cout << "c [walksat] tail starts after flip = " << tail_start_flip << endl;
}

void WalkSAT::print_statistics_header()
{
    cout << "c [walksat] numvars = " << numvars << ", numclauses = "
    << numclauses << ", numliterals = " << numliterals << endl;

    cout <<
        "c [walksat]     lowbad     unsat       avg   std dev    sd/avg     flips      undo              "
        "length       flips" << endl;
    cout <<
        "c [walksat]       this       end     unsat       avg     ratio      this      flip   success   "
        "success       until" << endl;
    cout <<
        "c [walksat]        try       try      tail     unsat      tail       try  fraction      rate     "
        "tries      assign" << endl;
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
    << "c [walksat] "
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
}

void WalkSAT::print_statistics_final()
{
    totalTime = cpuTime() - startTime;
    seconds_per_flip = ratio_for_stat(totalTime, totalflip);
    cout << "c [walksat] total elapsed seconds = " <<  totalTime << endl;
    cout << "c [walksat] num tries: " <<  numtry  << endl;
    cout << "c [walksat] avg flips per second = " << ratio_for_stat(totalflip, totalTime) << endl;
    cout << "c [walksat] number solutions found = " << found_solution << endl;
    cout << "c [walksat] final success rate = " << stats_line_percent(found_solution, numtry)  << endl;
    cout << "c [walksat] avg length successful tries = %" <<
           ratio_for_stat(totalsuccessflip,found_solution) << endl;
    if (found_solution) {
        cout << "c [walksat] avg flips per assign (over all runs) = " <<
               ratio_for_stat(totalflip, found_solution) << endl;
        cout << "c [walksat] avg seconds per assign (over all runs) = " <<
               ratio_for_stat(totalflip, found_solution) * seconds_per_flip << endl;
        cout << "c [walksat] mean flips until assign = " << mean_x << endl;
        cout << "c [walksat] mean seconds until assign = " << mean_x * seconds_per_flip << endl;
        cout << "c [walksat] mean restarts until assign = " << mean_r << endl;
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

        cout << "c [walksat] final numbad level statistics"  << endl;
        cout << "c [walksat]     statistics over all runs:"  << endl;
        cout << "c [walksat]       overall mean avg numbad = " << mean_avgfalse << endl;
        cout << "c [walksat]       overall mean meanbad std deviation = " << mean_std_dev_avgfalse << endl;
        cout << "c [walksat]       overall ratio mean numbad to mean std dev = " << ratio_mean_avgfalse << endl;
        cout << "c [walksat]     statistics on successful runs:"  << endl;
        cout << "c [walksat]       successful mean avg numbad = " << suc_mean_avgfalse << endl;
        cout << "c [walksat]       successful mean numbad std deviation = " << suc_mean_std_dev_avgfalse << endl;
        cout << "c [walksat]       successful ratio mean numbad to mean std dev = " <<
               suc_ratio_mean_avgfalse  << endl;
        cout << "c [walksat]     statistics on nonsuccessful runs:"  << endl;
        cout << "c [walksat]       nonsuccessful mean avg numbad level = " << nonsuc_mean_avgfalse  << endl;
        cout << "c [walksat]       nonsuccessful mean numbad std deviation = " <<
               nonsuc_mean_std_dev_avgfalse  << endl;
        cout << "c [walksat]       nonsuccessful ratio mean numbad to mean std dev = " <<
               nonsuc_ratio_mean_avgfalse  << endl;
    }

    if (found_solution) {
        cout << "c [walksat] ASSIGNMENT FOUND"  << endl;

        //TODO: assumptions!! -- we have removed them from the CNF
        //TODO: so we must now re-add them as 1+ level decisions.
        assert(solver->decisionLevel() == 0);
        for(size_t i = 0; i < solver->nVars(); i++) {
            //this will get set automatically anyway, skip
            if (solver->varData[i].removed != Removed::none) {
                continue;
            }
            if (solver->value(i) != l_Undef) {
                assert(solver->value(i) == value(i));
                continue;
            }

            solver->new_decision_level();
            solver->enqueue(Lit(i, value(i) == l_False));
        }
    } else
        cout << "c [walksat] ASSIGNMENT NOT FOUND"  << endl;
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
        if (bad) {
            unsat++;
        }
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
