/****************************************************************************
Copyright (c) 2006 - 2015, Armin Biere, Johannes Kepler University.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
****************************************************************************/

#ifndef picosat_h_INCLUDED
#define picosat_h_INCLUDED

/*------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

/*------------------------------------------------------------------------*/
/* The following macros allows for users to distiguish between different
 * versions of the API.  The first 'PICOSAT_REENTRANT_API' is defined for
 * the new reentrant API which allows to generate multiple instances of
 * PicoSAT in one process.  The second 'PICOSAT_API_VERSION' defines the
 * (smallest) version of PicoSAT to which this API conforms.
 */
#define PICOSAT_REENTRANT_API
#define PICOSAT_API_VERSION 953		/* API version */

/*------------------------------------------------------------------------*/
/* These are the return values for 'picosat_sat' as for instance
 * standardized by the output format of the SAT competition.
 */
#define PICOSAT_UNKNOWN         0
#define PICOSAT_SATISFIABLE     10
#define PICOSAT_UNSATISFIABLE   20

/*------------------------------------------------------------------------*/

typedef struct PicoSAT PicoSAT;

/*------------------------------------------------------------------------*/

const char *picosat_version (void);
const char *picosat_config (void);
const char *picosat_copyright (void);

/*------------------------------------------------------------------------*/
/* You can make PicoSAT use an external memory manager instead of the one
 * provided by LIBC. But then you need to call these three function before
 * 'picosat_init'.  The memory manager functions here all have an additional
 * first argument which is a pointer to the memory manager, but otherwise
 * are supposed to work as their LIBC counter parts 'malloc', 'realloc' and
 * 'free'.  As exception the 'resize' and 'delete' function have as third
 * argument the number of bytes of the block given as second argument.
 */

typedef void * (*picosat_malloc)(void *, size_t);
typedef void * (*picosat_realloc)(void*, void *, size_t, size_t);
typedef void (*picosat_free)(void*, void*, size_t);

/*------------------------------------------------------------------------*/

PicoSAT * picosat_init (void);          /* constructor */

PicoSAT * picosat_minit (void * state,
			 picosat_malloc,
			 picosat_realloc,
			 picosat_free);

void picosat_reset (PicoSAT *);         /* destructor */

/*------------------------------------------------------------------------*/
/* The following five functions are essentially parameters to 'init', and
 * thus should be called right after 'picosat_init' before doing anything
 * else.  You should not call any of them after adding a literal.
 */

/* Set output file, default is 'stdout'.
 */
void picosat_set_output (PicoSAT *, FILE *);

/* Measure all time spent in all calls in the solver.  By default only the
 * time spent in 'picosat_sat' is measured.  Enabling this function might
 * for instance triple the time needed to add large CNFs, since every call
 * to 'picosat_add' will trigger a call to 'getrusage'.
 */
void picosat_measure_all_calls (PicoSAT *);

/* Set the prefix used for printing verbose messages and statistics.
 * Default is "c ".
 */
void picosat_set_prefix (PicoSAT *, const char *);

/* Set verbosity level.  A verbosity level of 1 and above prints more and
 * more detailed progress reports on the output file, set by
 * 'picosat_set_output'.  Verbose messages are prefixed with the string set
 * by 'picosat_set_prefix'.
 */
void picosat_set_verbosity (PicoSAT *, int new_verbosity_level);

/* Disable/Enable all pre-processing, currently only failed literal probing.
 *
 *  new_plain_value != 0    only 'plain' solving, so no preprocessing
 *  new_plain_value == 0    allow preprocessing
 */
void picosat_set_plain (PicoSAT *, int new_plain_value);

/* Set default initial phase: 
 *
 *   0 = false
 *   1 = true
 *   2 = Jeroslow-Wang (default)
 *   3 = random initial phase
 *
 * After a variable has been assigned the first time, it will always
 * be assigned the previous value if it is picked as decision variable.
 * The initial assignment can be chosen with this function.
 */
void picosat_set_global_default_phase (PicoSAT *, int);

/* Set next/initial phase of a particular variable if picked as decision
 * variable.  Second argument 'phase' has the following meaning:
 *
 *   negative = next value if picked as decision variable is false
 *
 *   positive = next value if picked as decision variable is true
 *
 *   0        = use global default phase as next value and
 *              assume 'lit' was never assigned
 *
 * Again if 'lit' is assigned afterwards through a forced assignment,
 * then this forced assignment is the next phase if this variable is
 * used as decision variable.
 */
void picosat_set_default_phase_lit (PicoSAT *, int lit, int phase);

/* You can reset all phases by the following function.
 */
void picosat_reset_phases (PicoSAT *);

/* Scores can be erased as well.  Note, however, that even after erasing 
 * scores and phases, learned clauses are kept.  In addition head tail
 * pointers for literals are not moved either.  So expect a difference
 * between calling the solver in incremental mode or with a fresh copy of
 * the CNF.
 */
void picosat_reset_scores (PicoSAT *);

/* Reset assignment if in SAT state and then remove the given percentage of
 * less active (large) learned clauses.  If you specify 100% all large
 * learned clauses are removed.
 */
void picosat_remove_learned (PicoSAT *, unsigned percentage);

/* Set some variables to be more important than others.  These variables are
 * always used as decisions before other variables are used.  Dually there
 * is a set of variables that is used last.  The default is
 * to mark all variables as being indifferent only.
 */
void picosat_set_more_important_lit (PicoSAT *, int lit);
void picosat_set_less_important_lit (PicoSAT *, int lit);

/* Allows to print to internal 'out' file from client.
 */
void picosat_message (PicoSAT *, int verbosity_level, const char * fmt, ...);

/* Set a seed for the random number generator.  The random number generator
 * is currently just used for generating random decisions.  In our
 * experiments having random decisions did not really help on industrial
 * examples, but was rather helpful to randomize the solver in order to
 * do proper benchmarking of different internal parameter sets.
 */
void picosat_set_seed (PicoSAT *, unsigned random_number_generator_seed);

/* If you ever want to extract cores or proof traces with the current
 * instance of PicoSAT initialized with 'picosat_init', then make sure to
 * call 'picosat_enable_trace_generation' right after 'picosat_init'.   This
 * is not necessary if you only use 'picosat_set_incremental_rup_file'.
 *
 * NOTE, trace generation code is not necessarily included, e.g. if you
 * configure PicoSAT with full optimzation as './configure.sh -O' or with
 
 * you do not get any results by trying to generate traces.
 *
 * The return value is non-zero if code for generating traces is included
 * and it is zero if traces can not be generated.
 */
int picosat_enable_trace_generation (PicoSAT *);

/* You can dump proof traces in RUP format incrementally even without
 * keeping the proof trace in memory.  The advantage is a reduction of
 * memory usage, but the dumped clauses do not necessarily belong to the
 * clausal core.  Beside the file the additional parameters denotes the
 * maximal number of variables and the number of original clauses.
 */
void picosat_set_incremental_rup_file (PicoSAT *, FILE * file, int m, int n);

/* Save original clauses for 'picosat_deref_partial'.  See comments to that
 * function further down.
 */
void picosat_save_original_clauses (PicoSAT *);

/* Add a call back which is checked regularly to notify the SAT solver
 * to terminate earlier.  This is useful for setting external time limits
 * or terminate early in say a portfolio style parallel SAT solver.
 */
void picosat_set_interrupt (PicoSAT *,
                            void * external_state,
			    int (*interrupted)(void * external_state));

/*------------------------------------------------------------------------*/
/* This function returns the next available unused variable index and
 * allocates a variable for it even though this variable does not occur as
 * assumption, nor in a clause or any other constraints.  In future calls to
 * 'picosat_sat', 'picosat_deref' and particularly for 'picosat_changed',
 * this variable is treated as if it had been used.
 */
int picosat_inc_max_var (PicoSAT *);

/*------------------------------------------------------------------------*/
/* Push and pop semantics for PicoSAT.   'picosat_push' opens up a new
 * context.  All clauses added in this context are attached to it and
 * discarded when the context is closed with 'picosat_pop'.  It is also
 * possible to nest contexts.
 *
 * The current implementation uses a new internal variable for each context.
 * However, the indices for these internal variables are shared with
 * ordinary external variables.  This means that after any call to
 * 'picosat_push', new variable indices should be obtained with
 * 'picosat_inc_max_var' and not just by incrementing the largest variable
 * index used so far.
 *
 * The return value is the index of the literal that assumes this context.
 * This literal can only be used for 'picosat_failed_context' otherwise
 * it will lead to an API usage error.
 */
int picosat_push (PicoSAT *);

/* This is as 'picosat_failed_assumption', but only for internal variables
 * generated by 'picosat_push'.
 */
int picosat_failed_context (PicoSAT *, int lit);

/* Returns the literal that assumes the current context or zero if the
 * outer context has been reached.
 */
int picosat_context (PicoSAT *);	

/* Closes the current context and recycles the literal generated for
 * assuming this context.  The return value is the literal for the new
 * outer context or zero if the outer most context has been reached.
 */
int picosat_pop (PicoSAT *);

/* Force immmediate removal of all satisfied clauses and clauses that are
 * added or generated in closed contexts.  This function is called
 * internally if enough units are learned or after a certain number of
 * contexts have been closed.  This number is fixed at compile time
 * and defined as MAXCILS in 'picosat.c'.
 *
 * Note that learned clauses which only involve outer contexts are kept.
 */
void picosat_simplify (PicoSAT *);

/*------------------------------------------------------------------------*/
/* If you know a good estimate on how many variables you are going to use
 * then calling this function before adding literals will result in less
 * resizing of the variable table.  But this is just a minor optimization.
 * Beside exactly allocating enough variables it has the same effect as
 * calling 'picosat_inc_max_var'.
 */
void picosat_adjust (PicoSAT *, int max_idx);

/*------------------------------------------------------------------------*/
/* Statistics.
 */
int picosat_variables (PicoSAT *);                      /* p cnf <m> n */
int picosat_added_original_clauses (PicoSAT *);         /* p cnf m <n> */
size_t picosat_max_bytes_allocated (PicoSAT *);
double picosat_time_stamp (void);                       /* ... in process */
void picosat_stats (PicoSAT *);                         /* > output file */
unsigned long long picosat_propagations (PicoSAT *);	/* #propagations */
unsigned long long picosat_decisions (PicoSAT *);	/* #decisions */
unsigned long long picosat_visits (PicoSAT *);		/* #visits */

/* The time spent in calls to the library or in 'picosat_sat' respectively.
 * The former is returned if, right after initialization
 * 'picosat_measure_all_calls' is called.
 */
double picosat_seconds (PicoSAT *);

/*------------------------------------------------------------------------*/
/* Add a literal of the next clause.  A zero terminates the clause.  The
 * solver is incremental.  Adding a new literal will reset the previous
 * assignment.   The return value is the original clause index to which
 * this literal respectively the trailing zero belong starting at 0.
 */
int picosat_add (PicoSAT *, int lit);

/* As the previous function, but allows to add a full clause at once with an
 * at compiled time known size.  The list of argument literals has to be
 * terminated with a zero literal.  Literals beyond the first zero literal
 * are discarded.
 */
int picosat_add_arg (PicoSAT *, ...);

/* As the previous function but with an at compile time unknown size.
 */
int picosat_add_lits (PicoSAT *, int * lits);

/* Print the CNF to the given file in DIMACS format.
 */
void picosat_print (PicoSAT *, FILE *);

/* You can add arbitrary many assumptions before the next 'picosat_sat'
 * call.  This is similar to the using assumptions in MiniSAT, except that
 * for PicoSAT you do not have to collect all your assumptions in a vector
 * yourself.  In PicoSAT you can add one after the other, to be used in the 
 * next call to 'picosat_sat'.
 *
 * These assumptions can be interpreted as adding unit clauses with those
 * assumptions as literals.  However these assumption clauses are only valid
 * for exactly the next call to 'picosat_sat', and will be removed
 * afterwards, e.g. in following future calls to 'picosat_sat' after the
 * next 'picosat_sat' call, unless they are assumed again trough
 * 'picosat_assume'.
 *
 * More precisely, assumptions actually remain valid even after the next
 * call to 'picosat_sat' has returned.  Valid means they remain 'assumed'
 * internally until a call to 'picosat_add', 'picosat_assume', or a second
 * 'picosat_sat', following the first 'picosat_sat'.  The reason for keeping
 * them valid is to allow 'picosat_failed_assumption' to return correct
 * values.  
 *
 * Example:
 *
 *   picosat_assume (1);        // assume unit clause '1 0'
 *   picosat_assume (-2);       // additionally assume clause '-2 0'
 *   res = picosat_sat (1000);  // assumes 1 and -2 to hold
 *                              // 1000 decisions max.
 *
 *   if (res == PICOSAT_UNSATISFIABLE) 
 *     {
 *       if (picosat_failed_assumption (1))
 *         // unit clause '1 0' was necessary to derive UNSAT
 *
 *       if (picosat_failed_assumption (-2))
 *         // unit clause '-2 0' was necessary to derive UNSAT
 *
 *       // at least one but also both could be necessary
 *
 *       picosat_assume (17);  // previous assumptions are removed
 *                             // now assume unit clause '17 0' for
 *                             // the next call to 'picosat_sat'
 *
 *       // adding a new clause, actually the first literal of
 *       // a clause would also make the assumptions used in the previous
 *       // call to 'picosat_sat' invalid.
 *
 *       // The first two assumptions above are not assumed anymore.  Only
 *       // the assumptions, since the last call to 'picosat_sat' returned
 *       // are assumed, e.g. the unit clause '17 0'.
 *
 *       res = picosat_sat (-1);
 *     }
 *   else if (res == PICOSAT_SATISFIABLE)
 *     {
 *       // now the assignment is valid and we can call 'picosat_deref'
 *
 *       assert (picosat_deref (1) == 1));
 *       assert (picosat_deref (-2) == 1));
 *
 *       val = picosat_deref (15);
 *
 *       // previous two assumptions are still valid
 *
 *       // would become invalid if 'picosat_add' or 'picosat_assume' is
 *       // called here, but we immediately call 'picosat_sat'.  Now when
 *       // entering 'picosat_sat' the solver knows that the previous call
 *       // returned SAT and it can safely reset the previous assumptions
 *
 *       res = picosat_sat (-1);
 *     }
 *   else
 *     {
 *       assert (res == PICOSAT_UNKNOWN);
 *
 *       // assumptions valid, but assignment invalid
 *       // except for top level assigned literals which
 *       // necessarily need to have this value if the formula is SAT
 *
 *       // as above the solver nows that the previous call returned UNKWOWN
 *       // and will before doing anything else reset assumptions
 *
 *       picosat_sat (-1);
 *     }
 */
void picosat_assume (PicoSAT *, int lit);

/*------------------------------------------------------------------------*/
/* This is an experimental feature for handling 'all different constraints'
 * (ADC).  Currently only one global ADC can be handled.  The bit-width of
 * all the bit-vectors entered in this ADC (stored in 'all different
 * objects' or ADOs) has to be identical.
 *
 * TODO: also handle top level assigned literals here.
 */
void picosat_add_ado_lit (PicoSAT *, int);

/*------------------------------------------------------------------------*/
/* Call the main SAT routine.  A negative decision limit sets no limit on
 * the number of decisions.  The return values are as above, e.g.
 * 'PICOSAT_UNSATISFIABLE', 'PICOSAT_SATISFIABLE', or 'PICOSAT_UNKNOWN'.
 */
int picosat_sat (PicoSAT *, int decision_limit);

/* As alternative to a decision limit you can use the number of propagations
 * as limit.  This is more linearly related to execution time. This has to
 * be called after 'picosat_init' and before 'picosat_sat'.
 */
void picosat_set_propagation_limit (PicoSAT *, unsigned long long limit);

/* Return last result of calling 'picosat_sat' or '0' if not called.
 */
int picosat_res (PicoSAT *);

/* After 'picosat_sat' was called and returned 'PICOSAT_SATISFIABLE', then
 * the satisfying assignment can be obtained by 'dereferencing' literals.
 * The value of the literal is return as '1' for 'true',  '-1' for 'false'
 * and '0' for an unknown value.
 */
int picosat_deref (PicoSAT *, int lit);

/* Same as before but just returns true resp. false if the literals is
 * forced to this assignment at the top level.  This function does not
 * require that 'picosat_sat' was called and also does not internally reset
 * incremental usage.
 */
int picosat_deref_toplevel (PicoSAT *, int lit);

/* After 'picosat_sat' was called and returned 'PICOSAT_SATISFIABLE' a
 * partial satisfying assignment can be obtained as well.  It satisfies all
 * original clauses.  The value of the literal is return as '1' for 'true',
 * '-1' for 'false' and '0' for an unknown value.  In order to make this
 * work all original clauses have to be saved internally, which has to be
 * enabled by 'picosat_save_original_clauses' right after initialization.
 */
int picosat_deref_partial (PicoSAT *, int lit);

/* Returns non zero if the CNF is unsatisfiable because an empty clause was
 * added or derived.
 */
int picosat_inconsistent  (PicoSAT *);

/* Returns non zero if the literal is a failed assumption, which is defined
 * as an assumption used to derive unsatisfiability.  This is as accurate as
 * generating core literals, but still of course is an overapproximation of
 * the set of assumptions really necessary.  The technique does not need
 * clausal core generation nor tracing to be enabled and thus can be much
 * more effective.  The function can only be called as long the current
 * assumptions are valid.  See 'picosat_assume' for more details.
 */
int picosat_failed_assumption (PicoSAT *, int lit);

/* Returns a zero terminated list of failed assumption in the last call to
 * 'picosat_sat'.  The pointer is valid until the next call to
 * 'picosat_sat' or 'picosat_failed_assumptions'.  It only makes sense if the
 * last call to 'picosat_sat' returned 'PICOSAT_UNSATISFIABLE'.
 */
const int * picosat_failed_assumptions (PicoSAT *);

/* Returns a zero terminated minimized list of failed assumption for the last
 * call to 'picosat_sat'.  The pointer is valid until the next call to this
 * function or 'picosat_sat' or 'picosat_mus_assumptions'.  It only makes sense
 * if the last call to 'picosat_sat' returned 'PICOSAT_UNSATISFIABLE'.
 *
 * The call back function is called for all successful simplification
 * attempts.  The first argument of the call back function is the state
 * given as first argument to 'picosat_mus_assumptions'.  The second
 * argument to the call back function is the new reduced list of failed
 * assumptions.
 *
 * This function will call 'picosat_assume' and 'picosat_sat' internally but
 * before returning reestablish a proper UNSAT state, e.g.
 * 'picosat_failed_assumption' will work afterwards as expected.
 *
 * The last argument if non zero fixes assumptions.  In particular, if an
 * assumption can not be removed it is permanently assigned true, otherwise
 * if it turns out to be redundant it is permanently assumed to be false.
 */
const int * picosat_mus_assumptions (PicoSAT *, void *,
                                     void(*)(void*,const int*),int);

/* Compute one maximal subset of satisfiable assumptions.  You need to set
 * the assumptions, call 'picosat_sat' and check for 'picosat_inconsistent',
 * before calling this function.  The result is a zero terminated array of
 * assumptions that consistently can be asserted at the same time.  Before
 * returing the library 'reassumes' all assumptions.
 *
 * It could be beneficial to set the default phase of assumptions
 * to true (positive).  This can speed up the computation.
 */
const int * picosat_maximal_satisfiable_subset_of_assumptions (PicoSAT *);

/* This function assumes that you have set up all assumptions with
 * 'picosat_assume'.  Then it calls 'picosat_sat' internally unless the
 * formula is already inconsistent without assumptions, i.e.  it contains
 * the empty clause.  After that it extracts a maximal satisfiable subset of
 * assumptions.
 *
 * The result is a zero terminated maximal subset of consistent assumptions
 * or a zero pointer if the formula contains the empty clause and thus no
 * more maximal consistent subsets of assumptions can be extracted.  In the
 * first case, before returning, a blocking clause is added, that rules out
 * the result for the next call.
 *
 * NOTE: adding the blocking clause changes the CNF.
 *
 * So the following idiom
 *
 * const int * mss;
 * picosat_assume (a1);
 * picosat_assume (a2);
 * picosat_assume (a3);
 * picosat_assume (a4);
 * while ((mss = picosat_next_maximal_satisfiable_subset_of_assumptions ()))
 *   process_mss (mss);
 *
 * can be used to iterate over all maximal consistent subsets of
 * the set of assumptions {a1,a2,a3,a4}.
 *
 * It could be beneficial to set the default phase of assumptions
 * to true (positive).  This might speed up the computation.
 */
const int * 
picosat_next_maximal_satisfiable_subset_of_assumptions (PicoSAT *);

/* Similarly we can iterate over all minimal correcting assumption sets.
 * See the CAMUS literature [M. Liffiton, K. Sakallah JAR 2008].
 *
 * The result contains each assumed literal only once, even if it
 * was assumed multiple times (in contrast to the maximal consistent
 * subset functions above).
 *
 * It could be beneficial to set the default phase of assumptions
 * to true (positive).  This might speed up the computation.
 */
const int *
picosat_next_minimal_correcting_subset_of_assumptions (PicoSAT *);

/* Compute the union of all minmal correcting sets, which is called
 * the 'high level union of all minimal unsatisfiable subset sets'
 * or 'HUMUS' in our papers.
 *
 * It uses 'picosat_next_minimal_correcting_subset_of_assumptions' and
 * the same notes and advices apply.  In particular, this implies that
 * after calling the function once, the current CNF becomes inconsistent,
 * and PicoSAT has to be reset.  So even this function internally uses
 * PicoSAT incrementally, it can not be used incrementally itself at this
 * point.
 *
 * The 'callback' can be used for progress logging and is called after
 * each extracted minimal correcting set if non zero.  The 'nhumus'
 * parameter of 'callback' denotes the number of assumptions found to be
 * part of the HUMUS sofar.
 */
const int *
picosat_humus (PicoSAT *,
               void (*callback)(void * state, int nmcs, int nhumus),
	       void * state);

/*------------------------------------------------------------------------*/
/* Assume that a previous call to 'picosat_sat' in incremental usage,
 * returned 'SATISFIABLE'.  Then a couple of clauses and optionally new
 * variables were added (a new variable is a variable that has an index
 * larger then the maximum variable added so far).  The next call to
 * 'picosat_sat' also returns 'SATISFIABLE'. If this function
 * 'picosat_changed' returns '0', then the assignment to the old variables
 * is guaranteed to not have changed.  Otherwise it might have changed.
 * 
 * The return value to this function is only valid until new clauses are
 * added through 'picosat_add', an assumption is made through
 * 'picosat_assume', or again 'picosat_sat' is called.  This is the same
 * assumption as for 'picosat_deref'.
 *
 * TODO currently this function might also return a non zero value even if
 * the old assignment did not change, because it only checks whether the
 * assignment of at least one old variable was flipped at least once during
 * the search.  In principle it should be possible to be exact in the other
 * direction as well by using a counter of variables that have an odd number
 * of flips.  But this is not implemented yet.
 */
int picosat_changed (PicoSAT *);

/*------------------------------------------------------------------------*/
/* The following six functions internally extract the variable and clausal
 * core and thus require trace generation to be enabled with
 * 'picosat_enable_trace_generation' right after calling 'picosat_init'.
 *
 * TODO: using these functions in incremental mode with failed assumptions
 * has only been tested for 'picosat_corelit' thoroughly.  The others
 * probably only work in non-incremental mode or without using
 * 'picosat_assume'.
 */

/* This function determines whether the i'th added original clause is in the
 * core.  The 'i' is the return value of 'picosat_add', which starts at zero
 * and is incremented by one after a original clause is added (that is after
 * 'picosat_add (0)').  For the index 'i' the following has to hold: 
 *
 *   0 <= i < picosat_added_original_clauses ()
 */
int picosat_coreclause (PicoSAT *, int i);

/* This function gives access to the variable core, which is made up of the
 * variables that were resolved in deriving the empty clause.
 */
int picosat_corelit (PicoSAT *, int lit);

/* Write the clauses that were used in deriving the empty clause to a file
 * in DIMACS format.
 */
void picosat_write_clausal_core (PicoSAT *, FILE * core_file);

/* Write a proof trace in TraceCheck format to a file.
 */
void picosat_write_compact_trace (PicoSAT *, FILE * trace_file);
void picosat_write_extended_trace (PicoSAT *, FILE * trace_file);

/* Write a RUP trace to a file.  This trace file contains only the learned
 * core clauses while this is not necessarily the case for the RUP file
 * obtained with 'picosat_set_incremental_rup_file'.
 */
void picosat_write_rup_trace (PicoSAT *, FILE * trace_file);

/*------------------------------------------------------------------------*/
/* Keeping the proof trace around is not necessary if an over-approximation
 * of the core is enough.  A literal is 'used' if it was involved in a
 * resolution to derive a learned clause.  The core literals are necessarily
 * a subset of the 'used' literals.
 */

int picosat_usedlit (PicoSAT *, int lit);
/*------------------------------------------------------------------------*/
#endif
