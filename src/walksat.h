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
#include "solvertypes.h"
#include "MersenneTwister.h"

namespace CMSat {

class Solver;
class WalkSAT {
public:
    lbool main();
    WalkSAT(Solver* _solver);
    ~WalkSAT();

private:
    Solver* solver;

    /************************************/
    /* Main                             */
    /************************************/
    void flipvar(uint32_t toflip);

    /************************************/
    /* Initialization                   */
    /************************************/
    void parse_parameters();
    void init_for_round();
    bool init_problem();

    enum class add_cl_ret {added_cl, skipped_cl, unsat};
    template<class T>
    add_cl_ret add_this_clause(const T& cl, uint32_t& i, uint32_t& storeused);
    bool cl_shortening_triggered = false;

    ////////// adaptive //////////
    bool adaptive = true;
    uint32_t last_adaptive_objective; /* number of unsat clauses last time noise was adaptively updated */
    static constexpr double adaptive_phi = 0.20;
    static constexpr double adaptive_theta = 0.20;
    uint32_t stagnation_timer; /* number of remaining flips until stagnation check is performed */

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

    /*******************************************************/
    /* Utility Functions                                   */
    /*******************************************************/
    uint32_t countunsat();
    uint32_t RANDMOD(uint32_t x);
    void check_num_occurs();
    void check_make_break();

    /****************************************************************/
    /*                  Heuristics                                  */
    /****************************************************************/
    uint32_t pickrnovelty();

    /************************************/
    /* Main data structures             */
    /************************************/

    /* Atoms start at 1 */
    /* Not a is recorded as -1 * a */
    /* One dimensional arrays are statically allocated. */
    /* Two dimensional arrays are dynamically allocated in */
    /* the second dimension only.  */

    uint32_t numvars;     /* number of vars */
    uint32_t numclauses;   /* number of clauses */
    uint32_t numliterals; /* number of instances of literals across all clauses */
    uint32_t numfalse;   /* number of false clauses */

    /* Data structures for clauses */

    Lit *storebase = NULL; //all the literals of all the clauses
    Lit **clause = NULL; /* clauses to be satisfied */
    /* indexed as clause[clause_num][literal_num] */
    uint32_t *clsize = NULL;       /* length of each clause */
    uint32_t *false_cls = NULL;     /* clauses which are false */
    uint32_t *map_cl_to_false_cls = NULL; /* where each clause is listed in false */
    uint32_t *numtruelit = NULL; /* number of true literals in each clause */
    uint32_t longestclause;

    /* Data structures for vars: arrays of size numvars indexed by var */

    lbool *assigns = NULL;         /* value of each var */
    lbool *best_assigns = NULL;
    uint32_t *breakcount = NULL;   /* number of clauses that become unsat if var if flipped */
    uint32_t *makecount = NULL;    /* number of clauses that become sat if var if flipped */

    /* Data structures literals: arrays of size 2*numvars, indexed by literal+numvars */

    ///TODO make this half the size by using offsets
    /** where each literal occurs, size 2*numvars
       indexed as occurrence[literal+numvars][occurrence_num] */
    uint32_t **occurrence = NULL;
    uint32_t* occur_list_alloc = NULL;

    /** number of times each literal occurs, size 2*numvars
        indexed as numoccurrence[literal+numvars]              */
    uint32_t *numoccurrence = NULL;

    /* Data structures for lists of clauses used in heuristics */
    int64_t *changed = NULL;   /* step at which variable was last flipped */

    /************************************/
    /* Global flags and parameters      */
    /************************************/
    uint32_t numerator; /* make random flip with numerator/denominator frequency */
    double walk_probability = 0.5;
    uint64_t numflip;        /* number of changes so far */
    static constexpr uint64_t cutoff = 100000;
    static constexpr uint32_t denominator = 100000; /* denominator used in fractions to represent probabilities */
    static constexpr uint32_t one_percent = 1000;   /* ONE_PERCENT / denominator = 0.01 */
    uint32_t numtry = 0;   /* total attempts at solutions */

    /* Histogram of tail */
    uint32_t tail = 10;
    uint32_t tail_start_flip;

    /* Statistics */

    double startTime;
    double totalTime = 0;
    int64_t flips_this_solution;
    uint32_t lowbad;                  /* lowest number of bad clauses during try */
    int64_t totalflip = 0;        /* total number of flips in all tries so far */
    int64_t totalsuccessflip = 0; /* total number of flips in all tries which succeeded so far */
    bool found_solution = false;       /* total found solutions */
    int64_t x;
    int64_t integer_sum_x = 0;
    double sum_x = 0.0;
    double seconds_per_flip;
    int r;
    int sum_r = 0;
    double avgfalse;
    double sumfalse;
    double sample_size;
    double sum_avgfalse = 0.0;
    double mean_avgfalse;
    int number_sampled_runs = 0;
    double suc_sum_avgfalse = 0.0;
    double suc_mean_avgfalse;
    int suc_number_sampled_runs = 0;
    double nonsuc_sum_avgfalse = 0.0;
    double nonsuc_mean_avgfalse;
    int nonsuc_number_sampled_runs = 0;
    uint32_t lowestbad;
    MTRand mtrand;

    //helpers
    lbool value(const uint32_t var) const {
        return assigns[var];
    }
    lbool value(const Lit l) const {
        return assigns[l.var()] ^ l.sign();
    }
};

}

#endif //WALKSAT_H
