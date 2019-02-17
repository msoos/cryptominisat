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

#ifndef WALKSAT_H
#define WALKSAT_H

#include <cstdint>
#include <cstdio>

namespace CMSat {

class WalkSAT {
public:
    int main();

private:
    /************************************/
    /* Main                             */
    /************************************/
    void flipvar(int toflip);

    /************************************/
    /* Initialization                   */
    /************************************/
    void parse_parameters();
    void init();
    void initprob();

    /************************************/
    /* Printing and Statistics          */
    /************************************/
    void print_parameters();
    void initialize_statistics();
    void print_statistics_header();
    void update_statistics_start_try();
    void update_statistics_end_flip();
    void update_and_print_statistics_end_try();
    void print_statistics_final();
    void print_sol_cnf();

    /*******************************************************/
    /* Utility Functions                                   */
    /*******************************************************/
    long super(int i);
    int countunsat();

    /****************************************************************/
    /*                  Heuristics                                  */
    /****************************************************************/
    int pickbest();

    /************************************/
    /* Main data structures             */
    /************************************/

    /* Atoms start at 1 */
    /* Not a is recorded as -1 * a */
    /* One dimensional arrays are statically allocated. */
    /* Two dimensional arrays are dynamically allocated in */
    /* the second dimension only.  */

    int numvars;     /* number of vars */
    int numclauses;   /* number of clauses */
    int numliterals; /* number of instances of literals across all clauses */
    int numfalse;   /* number of false clauses */

    /* Data structures for clauses */

    int **clause; /* clauses to be satisfied */
    /* indexed as clause[clause_num][literal_num] */
    int *clsize;       /* length of each clause */
    int * false_cls;     /* clauses which are false */
    int *wherefalse; /* where each clause is listed in false */
    int *numtruelit; /* number of true literals in each clause */
    int longestclause;

    /* Data structures for vars: arrays of size numvars+1 indexed by var */

    int *assigns;         /* value of each var */
    int *breakcount;   /* number of clauses that become unsat if var if flipped */
    int *makecount;    /* number of clauses that become sat if var if flipped */

    /* Data structures literals: arrays of size 2*numvars+1, indexed by literal+numvars */

    int **occurrence; /* where each literal occurs, size 2*numvars+1            */
    /* indexed as occurrence[literal+numvars][occurrence_num] */

    int *numoccurrence; /* number of times each literal occurs, size 2*numvars+1  */
    /* indexed as numoccurrence[literal+numvars]              */

    /* Data structures for lists of clauses used in heuristics */
    int *best;

    /************************************/
    /* Global flags and parameters      */
    /************************************/

    /* Options */
    FILE *cnfStream;

    int numerator; /* make random flip with numerator/denominator frequency */
    double walk_probability = 0.5;
    int64_t numflip;        /* number of changes so far */
    int numrun = 10;
    int64_t cutoff = 100000;
    int64_t base_cutoff = 100000;
    int numtry = 0;   /* total attempts at solutions */

    int freebienoise = 0;

    /* Random seed */
    unsigned int seed; /* Sometimes defined as an unsigned long int */

    /* Histogram of tail */
    static const int HISTMAX=64;         /* length of histogram of tail */
    long histtotal;
    int tail = 10;
    int tail_start_flip;
    int undo_age = 1;

    /* Statistics */

    double expertime;
    int64_t flips_this_solution;
    int lowbad;                  /* lowest number of bad clauses during try */
    int64_t totalflip = 0;        /* total number of flips in all tries so far */
    int64_t totalsuccessflip = 0; /* total number of flips in all tries which succeeded so far */
    bool found_solution = 0;       /* total found solutions */
    int64_t x;
    int64_t integer_sum_x = 0;
    double sum_x = 0.0;
    double mean_x;
    double seconds_per_flip;
    int r;
    int sum_r = 0;
    double mean_r;
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
};

}

#endif //WALKSAT_H
