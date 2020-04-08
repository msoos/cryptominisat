/*
Copyright (c) 2010-2015 Mate Soos

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
*/

#if defined(__GNUC__) && defined(__linux__)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fenv.h>
#endif

#include <ctime>
#include <cstring>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <fstream>
#include <sys/stat.h>
#include <string.h>
#include <list>
#include <array>
#include <thread>

#include "main.h"
#include "main_common.h"
#include "time_mem.h"
#include "dimacsparser.h"
#include "cryptominisat5/cryptominisat.h"
#include "signalcode.h"

#include <boost/lexical_cast.hpp>
using namespace CMSat;
using boost::lexical_cast;

using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;
using std::list;
using std::map;

struct WrongParam
{
    WrongParam(string _param, string _msg) :
        param(_param)
        , msg(_msg)
    {}

    const string& getMsg() const
    {
        return msg;
    }

    const string& getParam() const
    {
        return param;
    }

    string param;
    string msg;
};

bool fileExists(const string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}


Main::Main(int _argc, char** _argv) :
    argc(_argc)
    , argv(_argv)
    , fileNamePresent (false)
{
}

void Main::readInAFile(SATSolver* solver2, const string& filename)
{
    solver2->add_sql_tag("filename", filename);
    if (conf.verbosity) {
        cout << "c Reading file '" << filename << "'" << endl;
    }
    #ifndef USE_ZLIB
    FILE * in = fopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<FILE*, FN> > parser(solver2, &debugLib, conf.verbosity);
    #else
    gzFile in = gzopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<gzFile, GZ> > parser(solver2, &debugLib, conf.verbosity);
    #endif

    if (in == NULL) {
        std::cerr
        << "ERROR! Could not open file '"
        << filename
        << "' for reading: " << strerror(errno) << endl;

        std::exit(1);
    }

    bool strict_header = conf.preprocess;
    if (!parser.parse_DIMACS(in, strict_header)) {
        exit(-1);
    }

    if (!sampling_vars_str.empty() && !parser.sampling_vars.empty()) {
        cerr << "ERROR! Sampling vars set in console but also in CNF." << endl;
        exit(-1);
    }

    if (!sampling_vars_str.empty()) {
        assert(sampling_vars.empty());

        std::stringstream ss(sampling_vars_str);
        uint32_t i;
        while (ss >> i)
        {
            const uint32_t var = i-1;
            sampling_vars.push_back(var);

            if (ss.peek() == ',' || ss.peek() == ' ')
                ss.ignore();
        }
    } else {
        sampling_vars.swap(parser.sampling_vars);
    }

    if (sampling_vars.empty()) {
        if (only_sampling_solution) {
            cout << "ERROR: only sampling vars are requested in the solution, but no sampling vars have been set!" << endl;
            exit(-1);
        }
    } else {
        solver2->set_sampling_vars(&sampling_vars);
        if (sampling_vars.size() > 100) {
            cout
            << "c Sampling var set contains over 100 variables, not displaying"
            << endl;
        } else {
            cout << "c Sampling vars set (total num: "
            << sampling_vars.size() << " ) : ";
            for(size_t i = 0; i < sampling_vars.size(); i++) {
                const uint32_t v = sampling_vars[i];
                cout << v+1;
                if (i+1 != sampling_vars.size())
                    cout << ",";
            }
            cout << endl;
        }
    }
    call_after_parse();

    #ifndef USE_ZLIB
        fclose(in);
    #else
        gzclose(in);
    #endif
}

void Main::readInStandardInput(SATSolver* solver2)
{
    if (conf.verbosity) {
        cout
        << "c Reading from standard input... Use '-h' or '--help' for help."
        << endl;
    }

    #ifndef USE_ZLIB
    FILE * in = stdin;
    #else
    gzFile in = gzdopen(0, "rb"); //opens stdin, which is 0
    #endif

    if (in == NULL) {
        std::cerr << "ERROR! Could not open standard input for reading" << endl;
        std::exit(1);
    }

    #ifndef USE_ZLIB
    DimacsParser<StreamBuffer<FILE*, FN> > parser(solver2, &debugLib, conf.verbosity);
    #else
    DimacsParser<StreamBuffer<gzFile, GZ> > parser(solver2, &debugLib, conf.verbosity);
    #endif

    if (!parser.parse_DIMACS(in, false)) {
        exit(-1);
    }

    #ifdef USE_ZLIB
        gzclose(in);
    #endif
}

void Main::parseInAllFiles(SATSolver* solver2)
{
    const double myTimeTotal = cpuTimeTotal();
    const double myTime = cpuTime();

    //First read normal extra files
    if (!debugLib.empty() && filesToRead.size() > 1) {
        cout
        << "ERROR: debugLib must be OFF"
        << " to parse in more than one file"
        << endl;

        std::exit(-1);
    }

    for (const string& fname: filesToRead) {
        readInAFile(solver2, fname.c_str());
    }

    solver->add_sql_tag("stdin", fileNamePresent ? "False" : "True");
    if (!fileNamePresent) {
        readInStandardInput(solver2);
    }

    if (conf.verbosity) {
        if (num_threads > 1) {
            cout
            << "c Sum parsing time among all threads (wall time will differ): "
            << std::fixed << std::setprecision(2)
            << (cpuTimeTotal() - myTimeTotal)
            << " s" << endl;
        } else {
            cout
            << "c Parsing time: "
            << std::fixed << std::setprecision(2)
            << (cpuTime() - myTime)
            << " s" << endl;
        }
    }
}

void Main::printResultFunc(
    std::ostream* os
    , const bool toFile
    , const lbool ret
) {
    if (ret == l_True) {
        if(toFile) {
            *os << "SAT" << endl;
        }
        else if (!printResult) *os << "s SATISFIABLE" << endl;
        else                   *os << "s SATISFIABLE" << endl;
     } else if (ret == l_False) {
        if(toFile) {
            *os << "UNSAT" << endl;
        }
        else if (!printResult) *os << "s UNSATISFIABLE" << endl;
        else                   *os << "s UNSATISFIABLE" << endl;
    } else {
        *os << "s INDETERMINATE" << endl;
    }

    if (ret == l_True && (printResult || toFile)) {
        if (toFile) {
            auto fun = [&](uint32_t var) {
                if (solver->get_model()[var] != l_Undef) {
                    *os << ((solver->get_model()[var] == l_True)? "" : "-") << var+1 << " ";
                }
            };

            if (sampling_vars.empty() || !only_sampling_solution) {
                for (uint32_t var = 0; var < solver->nVars(); var++) {
                    fun(var);
                }

            } else {
                for (uint32_t var: sampling_vars) {
                    fun(var);
                }
            }
            *os << "0" << endl;
        } else {
            uint32_t num_undef;
            if (sampling_vars.empty() || !only_sampling_solution) {
                num_undef = print_model(solver, os);
            } else {
                num_undef = print_model(solver, os, &sampling_vars);
            }
            if (num_undef && !toFile && conf.verbosity) {
                cout << "c NOTE: " << num_undef << " variables are NOT set." << endl;
            }
        }
    }
}

void Main::add_supported_options()
{
    // Declare the supported options.
    generalOptions.add_options()
    ("help,h", "Print simple help")
    ("hhelp", "Print extensive help")
    ("version,v", "Print version info")
    ("verb", po::value(&conf.verbosity)->default_value(conf.verbosity)
        , "[0-10] Verbosity of solver. 0 = only solution")
    ("random,r", po::value(&conf.origSeed)->default_value(conf.origSeed)
        , "[0..] Random seed")
    ("threads,t", po::value(&num_threads)->default_value(1)
        ,"Number of threads")
    ("maxtime", po::value(&maxtime),
        "Stop solving after this much time (s)")
    ("maxconfl", po::value(&maxconfl),
        "Stop solving after this many conflicts")
//     ("undef", po::value(&conf.greedy_undef)->default_value(conf.greedy_undef)
//         , "Set as many variables in solution to UNDEF as possible if solution is SAT")
    ("mult,m", po::value(&conf.orig_global_timeout_multiplier)->default_value(conf.orig_global_timeout_multiplier)
        , "Time multiplier for all simplification cutoffs")
    ("memoutmult", po::value(&conf.var_and_mem_out_mult)->default_value(conf.var_and_mem_out_mult)
        , "Multiplier for memory-out checks on variables and clause-link-in, etc. Useful when you have limited memory.")
    ("preproc,p", po::value(&conf.preprocess)->default_value(conf.preprocess)
        , "0 = normal run, 1 = preprocess and dump, 2 = read back dump and solution to produce final solution")
    ("polar", po::value<string>()->default_value("auto")
        , "{true,false,rnd,auto} Selects polarity mode. 'true' -> selects only positive polarity when branching. 'false' -> selects only negative polarity when branching. 'auto' -> selects last polarity used (also called 'caching')")
    #ifdef STATS_NEEDED
    ("clid", po::bool_switch(&clause_ID_needed)
        , "Add clause IDs to DRAT output")
    #endif
    //("greedyunbound", po::bool_switch(&conf.greedyUnbound)
    //    , "Greedily unbound variables that are not needed for SAT")
    ;

    std::ostringstream s_blocking_multip;
    s_blocking_multip << std::setprecision(4) << conf.blocking_restart_multip;

    std::ostringstream s_local_glue_multiplier;
    s_local_glue_multiplier << std::setprecision(4) << conf.local_glue_multiplier;

    po::options_description restartOptions("Restart options");
    restartOptions.add_options()
    ("restart", po::value<string>()
        , "{geom, glue, luby}  Restart strategy to follow.")
    ("gluehist", po::value(&conf.shortTermHistorySize)->default_value(conf.shortTermHistorySize)
        , "The size of the moving window for short-term glue history of redundant clauses. If higher, the minimal number of conflicts between restarts is longer")
    ("blkrest", po::value(&conf.do_blocking_restart)->default_value(conf.do_blocking_restart)
        , "Perform blocking restarts as per Glucose 3.0")
    ("blkrestlen", po::value(&conf.blocking_restart_trail_hist_length)->default_value(conf.blocking_restart_trail_hist_length)
        , "Length of the long term trail size for blocking restart")
    ("blkrestmultip", po::value(&conf.blocking_restart_multip)->default_value(conf.blocking_restart_multip, s_blocking_multip.str())
        , "Multiplier used for blocking restart cut-off (called 'R' in Glucose 3.0)")
    ("lwrbndblkrest", po::value(&conf.lower_bound_for_blocking_restart)->default_value(conf.lower_bound_for_blocking_restart)
        , "Lower bound on blocking restart -- don't block before this many conflicts")
    ("locgmult" , po::value(&conf.local_glue_multiplier)->default_value(conf.local_glue_multiplier, s_local_glue_multiplier.str())
        , "The multiplier used to determine if we should restart during glue-based restart")
    ("ratiogluegeom", po::value(&conf.ratio_glue_geom)->default_value(conf.ratio_glue_geom)
        , "Ratio of glue vs geometric restarts -- more is more glue")
    ;

    std::ostringstream s_incclean;

    std::ostringstream s_adjust_low;
    s_adjust_low << std::setprecision(2) << conf.adjust_glue_if_too_many_low;

    po::options_description reduceDBOptions("Redundant clause options");
    reduceDBOptions.add_options()
    ("gluecut0", po::value(&conf.glue_put_lev0_if_below_or_eq)->default_value(conf.glue_put_lev0_if_below_or_eq)
        , "Glue value for lev 0 ('keep') cut")
    ("gluecut1", po::value(&conf.glue_put_lev1_if_below_or_eq)->default_value(conf.glue_put_lev1_if_below_or_eq)
        , "Glue value for lev 1 cut ('give another shot'")
    ("adjustglue", po::value(&conf.adjust_glue_if_too_many_low)->default_value(conf.adjust_glue_if_too_many_low, s_adjust_low.str())
        , "If more than this % of clauses is LOW glue (level 0) then lower the glue cutoff by 1 -- once and never again")
    ("everylev1", po::value(&conf.every_lev1_reduce)->default_value(conf.every_lev1_reduce)
        , "Reduce lev1 clauses every N")
    ("everylev2", po::value(&conf.every_lev2_reduce)->default_value(conf.every_lev2_reduce)
        , "Reduce lev2 clauses every N")
    ("lev1usewithin", po::value(&conf.must_touch_lev1_within)->default_value(conf.must_touch_lev1_within)
        , "Learnt clause must be used in lev1 within this timeframe or be dropped to lev2")
    ;

    po::options_description red_cl_dump_opts("Clause dumping after problem finishing");
    reduceDBOptions.add_options()
    ("dumpred", po::value(&dump_red_fname)->default_value(dump_red_fname)
        , "Dump redundant clauses of gluecut0&1 to this filename")
    ("dumpredmaxlen", po::value(&dump_red_max_len)->default_value(dump_red_max_len)
        , "When dumping redundant clauses, only dump clauses at most this long")
    ("dumpredmaxglue", po::value(&dump_red_max_len)->default_value(dump_red_max_glue)
        , "When dumping redundant clauses, only dump clauses with at most this large glue")
    ;

    std::ostringstream s_random_var_freq;
    s_random_var_freq << std::setprecision(5) << conf.random_var_freq;

    std::ostringstream s_var_decay_vsids_start;
    s_var_decay_vsids_start << std::setprecision(5) << conf.var_decay_vsids_start;

    std::ostringstream s_var_decay_vsids_max;
    s_var_decay_vsids_max << std::setprecision(5) << conf.var_decay_vsids_max;

    po::options_description varPickOptions("Variable branching options");
    varPickOptions.add_options()
    ("vardecaystart", po::value(&conf.var_decay_vsids_start)->default_value(conf.var_decay_vsids_start, s_var_decay_vsids_start.str())
        , "variable activity increase divider (MUST be smaller than multiplier)")
    ("vardecaymax", po::value(&conf.var_decay_vsids_max)->default_value(conf.var_decay_vsids_max, s_var_decay_vsids_max.str())
        , "variable activity increase divider (MUST be smaller than multiplier)")
    ("vincstart", po::value(&conf.var_inc_vsids_start)->default_value(conf.var_inc_vsids_start)
        , "variable activity increase starts with this value. Make sure that this multiplied by multiplier and divided by divider is larger than itself")
    ("vsidsalternate", po::value(&conf.alternate_vsids)->default_value(conf.alternate_vsids)
         , "Alternate VSIDS values between iterations")
    ("vsidsalterval1", po::value(&conf.alternate_vsids_decay_rate1)->default_value(conf.alternate_vsids_decay_rate1)
         , "Alternate value 1 of VSIDS decay rate -- active when alternation is ON")
    ("vsidsalterval2", po::value(&conf.alternate_vsids_decay_rate2)->default_value(conf.alternate_vsids_decay_rate2)
        , "Alternate value 2 of VSIDS decay rate -- active when alternation is ON")
    ("freq", po::value(&conf.random_var_freq)->default_value(conf.random_var_freq, s_random_var_freq.str())
        , "[0 - 1] freq. of picking var at random")
    ("maple", po::value(&conf.maple)->default_value(conf.maple)
        , "Use maple-type variable picking sometimes")
    ("maplemod", po::value(&conf.modulo_maple_iter)->default_value(conf.modulo_maple_iter)
        , "Use maple N-1 of N rounds. Normally, N is 2, so used every other round. Set to 3 so it will use maple 2/3rds of the time.")
    ("maplemorebump", po::value(&conf.more_maple_bump_high_glue)->default_value(conf.more_maple_bump_high_glue)
        , "Bump variable usefulness more when glue is HIGH")
    ("maplealternate", po::value(&conf.alternate_maple)->default_value(conf.alternate_maple)
         , "Alternate maple values between iterations")
    ("maplealterval1", po::value(&conf.alternate_maple_decay_rate1)->default_value(conf.alternate_maple_decay_rate1)
         , "Alternate value 1 of maple decay rate -- active when alternation is ON")
    ("maplealterval2", po::value(&conf.alternate_maple_decay_rate2)->default_value(conf.alternate_maple_decay_rate2)
        , "Alternate value 2 of maple decay rate -- active when alternation is ON")
    ;


    po::options_description iterativeOptions("Iterative solve options");
    iterativeOptions.add_options()
    ("maxsol", po::value(&max_nr_of_solutions)->default_value(max_nr_of_solutions)
        , "Search for given amount of solutions. Thanks to Jannis Harder for the decision-based banning idea")
    ("debuglib", po::value<string>(&debugLib)
        , "MainSolver at specific 'solve()' points in CNF file")
    ;

    po::options_description sls_options("Stochastic Local Search options");
    sls_options.add_options()
    ("sls", po::value(&conf.doSLS)->default_value(conf.doSLS)
        , "Run SLS during simplification")
    ("slstype", po::value(&conf.which_sls)->default_value(conf.which_sls)
        , "Which SLS to run. Allowed values: walksat, yalsat, ccnr")
    ("slsmaxmem", po::value(&conf.sls_memoutMB)->default_value(conf.sls_memoutMB)
        , "Maximum number of MB to give to SLS solver. Doesn't run SLS solver if the memory usage would be more than this.")
    ("slseveryn", po::value(&conf.sls_every_n)->default_value(conf.sls_every_n)
        , "Run SLS solver every N simplifications only")
    ("yalsatmems", po::value(&conf.yalsat_max_mems)->default_value(conf.yalsat_max_mems)
        , "Run Yalsat with this many mems*million timeout. Limits time of yalsat run")
    ("walksatruns", po::value(&conf.walksat_max_runs)->default_value(conf.walksat_max_runs)
        , "Max 'runs' for WalkSAT. Limits time of WalkSAT run")
    ("slsgetphase", po::value(&conf.sls_get_phase)->default_value(conf.sls_get_phase)
        , "Get phase from SLS solver, set as new phase for CDCL")
    ("slstobump", po::value(&conf.sls_how_many_to_bump)->default_value(conf.sls_how_many_to_bump)
        , "How many variables to bump in CCNR")
    ;

    po::options_description probeOptions("Probing options");
    probeOptions.add_options()
    ("bothprop", po::value(&conf.doBothProp)->default_value(conf.doBothProp)
        , "Do propagations solely to propagate the same value twice")
    ("probe", po::value(&conf.doProbe)->default_value(conf.doProbe)
        , "Carry out probing")
    ("probemaxm", po::value(&conf.probe_bogoprops_time_limitM)->default_value(conf.probe_bogoprops_time_limitM)
      , "Time in mega-bogoprops to perform probing")
    ("transred", po::value(&conf.doTransRed)->default_value(conf.doTransRed)
        , "Remove useless binary clauses (transitive reduction)")
    ("intree", po::value(&conf.doIntreeProbe)->default_value(conf.doIntreeProbe)
        , "Carry out intree-based probing")
    ("intreemaxm", po::value(&conf.intree_time_limitM)->default_value(conf.intree_time_limitM)
      , "Time in mega-bogoprops to perform intree probing")
    ;

    std::ostringstream ssERatio;
    ssERatio << std::setprecision(4) << conf.varElimRatioPerIter;

    std::ostringstream s_num_conflicts_of_search_inc;
    s_num_conflicts_of_search_inc << std::setprecision(4) << conf.num_conflicts_of_search_inc;

    po::options_description simp_schedules("Simplification schedules");
    simp_schedules.add_options()
    ("schedsimp", po::value(&conf.do_simplify_problem)->default_value(conf.do_simplify_problem)
        , "Perform simplification rounds. If 0, we never perform any.")
    ("presimp", po::value(&conf.simplify_at_startup)->default_value(conf.simplify_at_startup)
        , "Perform simplification at the very start")
    ("allpresimp", po::value(&conf.simplify_at_every_startup)->default_value(conf.simplify_at_every_startup)
        , "Perform simplification at EVERY start -- only matters in library mode")
    ("nonstop,n", po::value(&conf.never_stop_search)->default_value(conf.never_stop_search)
        , "Never stop the search() process in class SATSolver")
    ("maxnumsimppersolve", po::value(&conf.max_num_simplify_per_solve_call)->default_value(conf.max_num_simplify_per_solve_call)
        , "Maximum number of simplifiactions to perform for every solve() call. After this, no more inprocessing will take place.")

    ("schedule", po::value(&conf.simplify_schedule_nonstartup)
        , "Schedule for simplification during run")
    ("preschedule", po::value(&conf.simplify_schedule_startup)
        , "Schedule for simplification at startup")

    ("occsimp", po::value(&conf.perform_occur_based_simp)->default_value(conf.perform_occur_based_simp)
        , "Perform occurrence-list-based optimisations (variable elimination, subsumption, bounded variable addition...)")


    ("confbtwsimp", po::value(&conf.num_conflicts_of_search)->default_value(conf.num_conflicts_of_search)
        , "Start first simplification after this many conflicts")
    ("confbtwsimpinc", po::value(&conf.num_conflicts_of_search_inc)->default_value(conf.num_conflicts_of_search_inc, s_num_conflicts_of_search_inc.str())
        , "Simp rounds increment by this power of N")
    ;

    std::ostringstream tern_keep;
    tern_keep << std::setprecision(2) << conf.ternary_keep_mult;

    std::ostringstream tern_max_create;
    tern_max_create << std::setprecision(2) << conf.ternary_max_create;

    po::options_description tern_res_options("Ternary resolution");
    tern_res_options.add_options()
    ("tern", po::value(&conf.doTernary)->default_value(conf.doTernary)
        , "Perform Ternary resolution'")
    ("terntimelim", po::value(&conf.ternary_res_time_limitM)->default_value(conf.ternary_res_time_limitM)
        , "Time-out in bogoprops M of ternary resolution as per paper 'Look-Ahead Versus Look-Back for Satisfiability Problems'")
    ("ternkeep", po::value(&conf.ternary_keep_mult)->default_value(conf.ternary_keep_mult, tern_keep.str())
        , "Keep ternary clauses only if they are touched within this multiple of 'lev1usewithin'")
    ("terncreate", po::value(&conf.ternary_max_create)->default_value(conf.ternary_max_create, tern_max_create.str())
        , "Create only this multiple (of linked in cls) ternary clauses per simp run")
    ;

    po::options_description occ_mem_limits("Occ-based simplification memory limits");
    occ_mem_limits.add_options()
    ("occredmax", po::value(&conf.maxRedLinkInSize)->default_value(conf.maxRedLinkInSize)
        , "Don't add to occur list any redundant clause larger than this")
    ("occredmaxmb", po::value(&conf.maxOccurRedMB)->default_value(conf.maxOccurRedMB)
        , "Don't allow redundant occur size to be beyond this many MB")
    ("occirredmaxmb", po::value(&conf.maxOccurIrredMB)->default_value(conf.maxOccurIrredMB)
        , "Don't allow irredundant occur size to be beyond this many MB")
    ;

    po::options_description sub_str_time_limits("Occ-based subsumption and strengthening time limits");
    sub_str_time_limits.add_options()
    ("strengthen", po::value(&conf.do_strengthen_with_occur)->default_value(conf.do_strengthen_with_occur)
        , "Perform clause contraction through self-subsuming resolution as part of the occurrence-subsumption system")
    ("substimelim", po::value(&conf.subsumption_time_limitM)->default_value(conf.subsumption_time_limitM)
        , "Time-out in bogoprops M of subsumption of long clauses with long clauses, after computing occur")
    ("substimelimbinratio", po::value(&conf.subsumption_time_limit_ratio_sub_str_w_bin)->default_value(conf.subsumption_time_limit_ratio_sub_str_w_bin)
        , "Ratio of subsumption time limit to spend on sub&str long clauses with bin")
    ("substimelimlongratio", po::value(&conf.subsumption_time_limit_ratio_sub_w_long)->default_value(conf.subsumption_time_limit_ratio_sub_w_long)
        , "Ratio of subsumption time limit to spend on sub long clauses with long")
    ("strstimelim", po::value(&conf.strengthening_time_limitM)->default_value(conf.strengthening_time_limitM)
        , "Time-out in bogoprops M of strengthening of long clauses with long clauses, after computing occur")
    ("sublonggothrough", po::value(&conf.subsume_gothrough_multip)->default_value(conf.subsume_gothrough_multip)
        , "How many times go through subsume")
    ;

    po::options_description bva_options("BVA options");
    bva_options.add_options()
    ("bva", po::value(&conf.do_bva)->default_value(conf.do_bva)
        , "Perform bounded variable addition")
    ("bvaeveryn", po::value(&conf.bva_every_n)->default_value(conf.bva_every_n)
        , "Perform BVA only every N occ-simplify calls")
    ("bvalim", po::value(&conf.bva_limit_per_call)->default_value(conf.bva_limit_per_call)
        , "Maximum number of variables to add by BVA per call")
    ("bva2lit", po::value(&conf.bva_also_twolit_diff)->default_value(conf.bva_also_twolit_diff)
        , "BVA with 2-lit difference hack, too. Beware, this reduces the effectiveness of 1-lit diff")
    ("bvato", po::value(&conf.bva_time_limitM)->default_value(conf.bva_time_limitM)
        , "BVA time limit in bogoprops M")
    ;

    po::options_description bve_options("BVE options");
    bve_options.add_options()
    ("varelim", po::value(&conf.doVarElim)->default_value(conf.doVarElim)
        , "Perform variable elimination as per Een and Biere")
    ("varelimto", po::value(&conf.varelim_time_limitM)->default_value(conf.varelim_time_limitM)
        , "Var elimination bogoprops M time limit")
    ("varelimover", po::value(&conf.min_bva_gain)->default_value(conf.min_bva_gain)
        , "Do BVE until the resulting no. of clause increase is less than X. Only power of 2 makes sense, i.e. 2,4,8...")
    ("emptyelim", po::value(&conf.do_empty_varelim)->default_value(conf.do_empty_varelim)
        , "Perform empty resolvent elimination using bit-map trick")
    ("varelimmaxmb", po::value(&conf.var_linkin_limit_MB)->default_value(conf.var_linkin_limit_MB)
        , "Maximum extra MB of memory to use for new clauses during varelim")
    ("eratio", po::value(&conf.varElimRatioPerIter)->default_value(conf.varElimRatioPerIter, ssERatio.str())
        , "Eliminate this ratio of free variables at most per variable elimination iteration")
    ("skipresol", po::value(&conf.skip_some_bve_resolvents)->default_value(conf.skip_some_bve_resolvents)
        , "Skip BVE resolvents in case they belong to a gate")
    ;

    po::options_description xorOptions("XOR-related options");
    xorOptions.add_options()
    ("xor", po::value(&conf.doFindXors)->default_value(conf.doFindXors)
        , "Discover long XORs")
    ("maxxorsize", po::value(&conf.maxXorToFind)->default_value(conf.maxXorToFind)
        , "Maximum XOR size to find")
    ("xorcache", po::value(&conf.useCacheWhenFindingXors)->default_value(conf.useCacheWhenFindingXors)
        , "Use cache when finding XORs. Finds a LOT more XORs, but takes a lot more time")
    ("varsperxorcut", po::value(&conf.xor_var_per_cut)->default_value(conf.xor_var_per_cut)
        , "Number of _real_ variables per XOR when cutting them. So 2 will have XORs of size 4 because 1 = connecting to previous, 1 = connecting to next, 2 in the midde. If the XOR is 4 long, it will be just one 4-long XOR, no connectors")
    ("maxxormat", po::value(&conf.maxXORMatrix)->default_value(conf.maxXORMatrix)
        , "Maximum matrix size (=num elements) that we should try to echelonize")
    //Not implemented yet
    //("mix", po::value(&conf.doMixXorAndGates)->default_value(conf.doMixXorAndGates)
    //    , "Mix XORs and OrGates for new truths")
    ;

    po::options_description eqLitOpts("Equivalent literal options");
    eqLitOpts.add_options()
    ("scc", po::value(&conf.doFindAndReplaceEqLits)->default_value(conf.doFindAndReplaceEqLits)
        , "Find equivalent literals through SCC and replace them")
    ("extscc", po::value(&conf.doExtendedSCC)->default_value(conf.doExtendedSCC)
        , "Perform SCC using cache")
    ;

    po::options_description gateOptions("Gate-related options");
    gateOptions.add_options()
    ("gates", po::value(&conf.doGateFind)->default_value(conf.doGateFind)
        , "Find gates. Disables all sub-options below")
    ("gorshort", po::value(&conf.doShortenWithOrGates)->default_value(conf.doShortenWithOrGates)
        , "Shorten clauses with OR gates")
    ("gandrem", po::value(&conf.doRemClWithAndGates)->default_value(conf.doRemClWithAndGates)
        , "Remove clauses with AND gates")
    ("gateeqlit", po::value(&conf.doFindEqLitsWithGates)->default_value(conf.doFindEqLitsWithGates)
        , "Find equivalent literals using gates")
    /*("maxgatesz", po::value(&conf.maxGateSize)->default_value(conf.maxGateSize)
        , "Maximum gate size to discover")*/
    ("printgatedot", po::value(&conf.doPrintGateDot)->default_value(conf.doPrintGateDot)
        , "Print gate structure regularly to file 'gatesX.dot'")
    ("gatefindto", po::value(&conf.gatefinder_time_limitM)->default_value(conf.gatefinder_time_limitM)
        , "Max time in bogoprops M to find gates")
    ("shortwithgatesto", po::value(&conf.shorten_with_gates_time_limitM)->default_value(conf.shorten_with_gates_time_limitM)
        , "Max time to shorten with gates, bogoprops M")
    ("remwithgatesto", po::value(&conf.remove_cl_with_gates_time_limitM)->default_value(conf.remove_cl_with_gates_time_limitM)
        , "Max time to remove with gates, bogoprops M")
    ;

    po::options_description conflOptions("Conflict options");
    conflOptions.add_options()
    ("recur", po::value(&conf.doRecursiveMinim)->default_value(conf.doRecursiveMinim)
        , "Perform recursive minimisation")
    ("moreminim", po::value(&conf.doMinimRedMore)->default_value(conf.doMinimRedMore)
        , "Perform strong minimisation at conflict gen.")
    ("moremoreminim", po::value(&conf.doMinimRedMoreMore)->default_value(conf.doMinimRedMoreMore)
        , "Perform even stronger minimisation at conflict gen.")
    ("moremorecachelimit", po::value(&conf.more_red_minim_limit_cache)->default_value(conf.more_red_minim_limit_cache)
        , "Time-out in microsteps for each more minimisation with cache. Only active if 'moreminim' is on")
    ("moremorestamp", po::value(&conf.more_more_with_stamp)->default_value(conf.more_more_with_stamp)
        , "Use cache for otf more minim of learnt clauses")
    ("moremorealways", po::value(&conf.doAlwaysFMinim)->default_value(conf.doAlwaysFMinim)
        , "Always strong-minimise clause")
    ("decbased", po::value(&conf.do_decision_based_cl)->default_value(conf.do_decision_based_cl)
        , "Create decision-based conflict clauses when the UIP clause is too large")
    ("decbasemaxlev", po::value(&conf.decision_based_cl_max_levels)->default_value(conf.decision_based_cl_max_levels)
        , "Create decision-based conflict if the maximum level is below or equal to this")
    ("decbaseminsz", po::value(&conf.decision_based_cl_min_learned_size)->default_value(conf.decision_based_cl_min_learned_size)
        , "Create decision-based conflict if the learnt clause is larger than this")
    ;

    po::options_description propOptions("Propagation options");
    propOptions.add_options()
    ("updateglueonanalysis", po::value(&conf.update_glues_on_analyze)->default_value(conf.update_glues_on_analyze)
        , "Update glues while analyzing")
    ("otfhyper", po::value(&conf.otfHyperbin)->default_value(conf.otfHyperbin)
        , "Perform hyper-binary resolution at dec. level 1 after every restart and during probing")
    ;

    po::options_description chrono_bt_opts("Propagation options");
    chrono_bt_opts.add_options()
    ("diffdeclevelchrono", po::value(&conf.diff_declev_for_chrono)->default_value(conf.diff_declev_for_chrono)
        , "Difference in decision level is more than this, perform chonological backtracking instead of non-chronological backtracking. Giving -1 means it is never turned on (overrides '--confltochrono -1' in this case).")
    ;


    po::options_description stampOptions("Stamping options");
    stampOptions.add_options()
    ("stamp", po::value(&conf.doStamp)->default_value(conf.doStamp)
        , "Use time stamping as per Heule&Jarvisalo&Biere paper")
    ("cache", po::value(&conf.doCache)->default_value(conf.doCache)
        , "Use implication cache -- may use a lot of memory")
    ("cachesize", po::value(&conf.maxCacheSizeMB)->default_value(conf.maxCacheSizeMB)
        , "Maximum size of the implication cache in MB. It may temporarily reach higher usage, but will be deleted&disabled if this limit is reached.")
    ("cachecutoff", po::value(&conf.cacheUpdateCutoff)->default_value(conf.cacheUpdateCutoff)
        , "If the number of literals propagated by a literal is more than this, it's not included into the implication cache")
    ;

    po::options_description sqlOptions("SQL options");
    sqlOptions.add_options()
    ("sql", po::value(&sql)->default_value(0)
        , "Write to SQL. 0 = no SQL, 1 or 2 = sqlite")
    ("sqlitedb", po::value(&sqlite_filename)
        , "Where to put the SQLite database")
    ("cldatadumpratio", po::value(&conf.dump_individual_cldata_ratio)->default_value(conf.dump_individual_cldata_ratio)
        , "Only dump this ratio of clauses' data, randomly selected. Since machine learning doesn't need that much data, this can reduce the data you have to deal with.")
    ;

    po::options_description printOptions("Printing options");
    printOptions.add_options()
    ("verbstat", po::value(&conf.verbStats)->default_value(conf.verbStats)
        , "Change verbosity of statistics at the end of the solving [0..2]")
    ("verbrestart", po::value(&conf.print_full_restart_stat)->default_value(conf.print_full_restart_stat)
        , "Print more thorough, but different stats")
    ("verballrestarts", po::value(&conf.print_all_restarts)->default_value(conf.print_all_restarts)
        , "Print a line for every restart")
    ("printsol,s", po::value(&printResult)->default_value(printResult)
        , "Print assignment if solution is SAT")
    ("restartprint", po::value(&conf.print_restart_line_every_n_confl)->default_value(conf.print_restart_line_every_n_confl)
        , "Print restart status lines at least every N conflicts")
    ("dumpresult", po::value(&resultFilename)
        , "Write solution(s) to this file")
    ;

    po::options_description componentOptions("Component options");
    componentOptions.add_options()
    ("comps", po::value(&conf.doCompHandler)->default_value(conf.doCompHandler)
        , "Perform component-finding and separate handling")
    ("compsfrom", po::value(&conf.handlerFromSimpNum)->default_value(conf.handlerFromSimpNum)
        , "Component finding only after this many simplification rounds")
    ("compsvar", po::value(&conf.compVarLimit)->default_value(conf.compVarLimit)
        , "Only use components in case the number of variables is below this limit")
    ("compslimit", po::value(&conf.comp_find_time_limitM)->default_value(conf.comp_find_time_limitM)
        , "Limit how much time is spent in component-finding");

    po::options_description distillOptions("Distill options");
    distillOptions.add_options()
    //("noparts", "Don't find&solve subproblems with subsolvers")
    ("distill", po::value(&conf.do_distill_clauses)->default_value(conf.do_distill_clauses)
        , "Regularly execute clause distillation")
    ("distillmaxm", po::value(&conf.distill_long_cls_time_limitM)->default_value(conf.distill_long_cls_time_limitM)
        , "Maximum number of Mega-bogoprops(~time) to spend on vivifying/distilling long cls by enqueueing and propagating")
    ("distillto", po::value(&conf.distill_time_limitM)->default_value(conf.distill_time_limitM)
        , "Maximum time in bogoprops M for distillation")
    ("distillincconf", po::value(&conf.distill_increase_conf_ratio)->default_value(conf.distill_increase_conf_ratio)
        , "Multiplier for current number of conflicts OTF distill")
    ("distillminconf", po::value(&conf.distill_min_confl)->default_value(conf.distill_min_confl)
        , "Minimum number of conflicts between OTF distill")
    ("distilltier1ratio", po::value(&conf.distill_red_tier1_ratio)->default_value(conf.distill_red_tier1_ratio)
        , "How much of tier 1 to distill")
    ;

    po::options_description mem_save_opts("Memory saving options");
    mem_save_opts.add_options()
    ("renumber", po::value(&conf.doRenumberVars)->default_value(conf.doRenumberVars)
        , "Renumber variables to increase CPU cache efficiency")
    ("savemem", po::value(&conf.doSaveMem)->default_value(conf.doSaveMem)
        , "Save memory by deallocating variable space after renumbering. Only works if renumbering is active.")
    ("mustrenumber", po::value(&conf.must_renumber)->default_value(conf.must_renumber)
        , "Treat all 'renumber' strategies as 'must-renumber'")
    ("fullwatchconseveryn", po::value(&conf.full_watch_consolidate_every_n_confl)->default_value(conf.full_watch_consolidate_every_n_confl)
        , "Consolidate watchlists fully once every N conflicts. Scheduled during simplification rounds.")
    ("consolidatestaticorder", po::value(&conf.static_mem_consolidate_order)->default_value(conf.static_mem_consolidate_order)
        , "Consolidate clause memory in static order. If set to 0, it's consolidated in activity order")
    ;

    po::options_description miscOptions("Misc options");
    miscOptions.add_options()
    //("noparts", "Don't find&solve subproblems with subsolvers")
    ("strcachemaxm", po::value(&conf.watch_cache_stamp_based_str_time_limitM)->default_value(conf.watch_cache_stamp_based_str_time_limitM)
        , "Maximum number of Mega-bogoprops(~time) to spend on vivifying long irred cls through watches, cache and stamps")


    ("implicitmanip", po::value(&conf.doStrSubImplicit)->default_value(conf.doStrSubImplicit)
        , "Subsume and strengthen implicit clauses with each other")
    ("implsubsto", po::value(&conf.subsume_implicit_time_limitM)->default_value(conf.subsume_implicit_time_limitM)
        , "Timeout (in bogoprop Millions) of implicit subsumption")
    ("implstrto", po::value(&conf.distill_implicit_with_implicit_time_limitM)->default_value(conf.distill_implicit_with_implicit_time_limitM)
        , "Timeout (in bogoprop Millions) of implicit strengthening")
    ;

    po::options_description reconfOptions("Reconf options");
    reconfOptions.add_options()
    ("reconfat", po::value(&conf.reconfigure_at)->default_value(conf.reconfigure_at)
        , "Reconfigure after this many simplifications")
    ("reconf", po::value(&conf.reconfigure_val)->default_value(conf.reconfigure_val)
        , "Reconfigure after some time to this solver configuration [3,4,6,7,12,13,14,15,16]")
    ;

    hiddenOptions.add_options()
    ("sync", po::value(&conf.sync_every_confl)->default_value(conf.sync_every_confl)
        , "Sync threads every N conflicts")
    ("dratdebug", po::bool_switch(&dratDebug)
        , "Output DRAT verification into the console. Helpful to see where DRAT fails -- use in conjunction with --verb 20")
    ("clearinter", po::value(&need_clean_exit)->default_value(0)
        , "Interrupt threads cleanly, all the time")
    ("zero-exit-status", po::bool_switch(&zero_exit_status)
        , "Exit with status zero in case the solving has finished without an issue")
    ("printtimes", po::value(&conf.do_print_times)->default_value(conf.do_print_times)
        , "Print time it took for each simplification run. If set to 0, logs are easier to compare")
    ("savedstate", po::value(&conf.saved_state_file)->default_value(conf.saved_state_file)
        , "The file to save the saved state of the solver")
    ("maxsccdepth", po::value(&conf.max_scc_depth)->default_value(conf.max_scc_depth)
        , "The maximum for scc search depth")
    ("simdrat", po::value(&conf.simulate_drat)->default_value(conf.simulate_drat)
        , "Simulate DRAT")
    ("dumpdecformodel", po::value(&decisions_for_model_fname)->default_value(decisions_for_model_fname)
        , "Decisions for model will be dumped here")
    ("sampling", po::value(&sampling_vars_str)->default_value(sampling_vars_str)
        , "Sampling vars, separated by comma")
    ("onlysampling", po::bool_switch(&only_sampling_solution)
        , "Print and ban(!) solutions only in terms of variables declared in 'c ind' or as --sampling '...'")

    //these a kind of special and determine positional options' meanings
    ("input", po::value< vector<string> >(), "file(s) to read")
    ("drat,d", po::value(&dratfilname)
        , "Put DRAT verification information into this file")
    ;

#ifdef USE_GAUSS
    po::options_description gaussOptions("Gauss options");
    gaussOptions.add_options()
    ("maxgaussdepth", po::value(&conf.gaussconf.decision_until)->default_value(conf.gaussconf.decision_until)
        , "Only run Gaussian Elimination until this depth. You may want this depth to be relatively small, such as 120-30, though it depends on the complexity of the problem. It is best to experiment.")
     ("maxmatrixrows", po::value(&conf.gaussconf.max_matrix_rows)->default_value(conf.gaussconf.max_matrix_rows)
        , "Set maximum no. of rows for gaussian matrix. Too large matrixes"
        "should bee discarded for reasons of efficiency")
    ("autodisablegauss", po::value(&conf.gaussconf.autodisable)->default_value(conf.gaussconf.autodisable)
        , "Automatically disable gauss when performing badly")
    ("minmatrixrows", po::value(&conf.gaussconf.min_matrix_rows)->default_value(conf.gaussconf.min_matrix_rows)
        , "Set minimum no. of rows for gaussian matrix. Normally, too small"
        " matrixes are discarded for reasons of efficiency")
    ("maxnummatrixes", po::value(&conf.gaussconf.max_num_matrixes)->default_value(conf.gaussconf.max_num_matrixes)
        , "Maximum number of matrixes to treat.")
    ;
#endif //USE_GAUSS

    help_options_complicated
    .add(generalOptions)
    #if defined(USE_SQLITE3)
    .add(sqlOptions)
    #endif
    .add(restartOptions)
    .add(printOptions)
    .add(propOptions)
    .add(chrono_bt_opts)
    .add(reduceDBOptions)
    .add(red_cl_dump_opts)
    .add(varPickOptions)
    .add(conflOptions)
    .add(iterativeOptions)
    .add(probeOptions)
    .add(sls_options)
    .add(stampOptions)
    .add(simp_schedules)
    .add(occ_mem_limits)
    .add(tern_res_options)
    .add(sub_str_time_limits)
    .add(bve_options)
    .add(bva_options)
    .add(eqLitOpts)
    .add(componentOptions)
    .add(mem_save_opts)
    #if defined(USE_M4RI) || defined(USE_GAUSS)
    .add(xorOptions)
    #endif
    .add(gateOptions)
    #ifdef USE_GAUSS
    .add(gaussOptions)
    #endif
    .add(distillOptions)
    .add(reconfOptions)
    .add(miscOptions)
    ;
}

string remove_last_comma_if_exists(std::string s)
{
    std::string s2 = s;
    if (s[s.length()-1] == ',')
        s2.resize(s2.length()-1);
    return s2;
}

void Main::check_options_correctness()
{
    try {
        po::store(po::command_line_parser(argc, argv).options(all_options).positional(p).run(), vm);
        if (vm.count("hhelp"))
        {
            cout
            << "A universal, fast SAT solver with XOR and Gaussian Elimination support. " << endl
            << "Input "
            #ifndef USE_ZLIB
            << "must be plain"
            #else
            << "can be either plain or gzipped"
            #endif
            << " DIMACS with XOR extension" << endl << endl;

            cout
            << "cryptominisat5 [options] inputfile [drat-trim-file]" << endl << endl;

            cout << "Preprocessor usage:" << endl
            << "  cryptominisat5 --preproc 1 [options] inputfile simplified-cnf-file" << endl << endl
            << "  cryptominisat5 --preproc 2 [options] solution-file" << endl;

            cout << help_options_complicated << endl;
            cout << "Normal run schedules:" << endl;
            cout << "  Default schedule: "
            << remove_last_comma_if_exists(conf.simplify_schedule_nonstartup) << endl<< endl;
            cout << "  Schedule at startup: "
            << remove_last_comma_if_exists(conf.simplify_schedule_startup) << endl << endl;

            cout << "Preproc run schedule:" << endl;
            cout << "  "
            << remove_last_comma_if_exists(conf.simplify_schedule_preproc) << endl;
            std::exit(0);
        }

        if (vm.count("help"))
        {
            cout
            << "USAGE 1: " << argv[0] << " [options] inputfile [drat-trim-file]" << endl
            << "USAGE 2: " << argv[0] << " --preproc 1 [options] inputfile simplified-cnf-file" << endl
            << "USAGE 2: " << argv[0] << " --preproc 2 [options] solution-file" << endl

            << " where input is "
            #ifndef USE_ZLIB
            << "plain"
            #else
            << "plain or gzipped"
            #endif
            << " DIMACS." << endl;

            cout << help_options_simple << endl;
            std::exit(0);
        }

        po::notify(vm);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::unknown_option> >& c
    ) {
        cerr
        << "ERROR: Some option you gave was wrong. Please give '--help' to get help" << endl
        << "       Unknown option: " << c.what() << endl;
        std::exit(-1);
    } catch (boost::bad_any_cast &e) {
        std::cerr
        << "ERROR! You probably gave a wrong argument type" << endl
        << "       Bad cast: " << e.what()
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::invalid_option_value> >& what
    ) {
        cerr
        << "ERROR: Invalid value '" << what.what() << "'" << endl
        << "       given to option '" << what.get_option_name() << "'"
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::multiple_occurrences> >& what
    ) {
        cerr
        << "ERROR: " << what.what() << " of option '"
        << what.get_option_name() << "'"
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::required_option> >& what
    ) {
        cerr
        << "ERROR: You forgot to give a required option '"
        << what.get_option_name() << "'"
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::too_many_positional_options_error> >& what
    ) {
        cerr
        << "ERROR: You gave too many positional arguments. Only at most two can be given:" << endl
        << "       the 1st the CNF file input, and optionally, the 2nd the DRAT file output" << endl
        << "    OR (pre-processing)  1st for the input CNF, 2nd for the simplified CNF" << endl
        << "    OR (post-processing) 1st for the solution file" << endl
        ;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::ambiguous_option> >& what
    ) {
        cerr
        << "ERROR: The option you gave was not fully written and matches" << endl
        << "       more than one option. Please give the full option name." << endl
        << "       The option you gave: '" << what.get_option_name() << "'" <<endl
        << "       The alternatives are: ";
        for(size_t i = 0; i < what.alternatives().size(); i++) {
            cout << what.alternatives()[i];
            if (i+1 < what.alternatives().size()) {
                cout << ", ";
            }
        }
        cout << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::invalid_command_line_syntax> >& what
    ) {
        cerr
        << "ERROR: The option you gave is missing the argument or the" << endl
        << "       argument is given with space between the equal sign." << endl
        << "       detailed error message: " << what.what() << endl
        ;
        std::exit(-1);
    }
}

void Main::parse_restart_type()
{
    if (vm.count("restart")) {
        string type = vm["restart"].as<string>();
        if (type == "geom")
            conf.restartType = Restart::geom;
        else if (type == "luby")
            conf.restartType = Restart::luby;
        else if (type == "glue")
            conf.restartType = Restart::glue;
        else throw WrongParam("restart", "unknown restart type");
    }
}

void Main::parse_polarity_type()
{
    if (vm.count("polar")) {
        string mode = vm["polar"].as<string>();

        if (mode == "true") conf.polarity_mode = PolarityMode::polarmode_pos;
        else if (mode == "false") conf.polarity_mode = PolarityMode::polarmode_neg;
        else if (mode == "rnd") conf.polarity_mode = PolarityMode::polarmode_rnd;
        else if (mode == "auto") conf.polarity_mode = PolarityMode::polarmode_automatic;
        else throw WrongParam(mode, "unknown polarity-mode");
    }
}

void Main::manually_parse_some_options()
{
    if (conf.which_sls != "yalsat" && conf.which_sls != "walksat" && conf.which_sls != "ccnr") {
        cout << "ERROR: you gave '" << conf.which_sls << " for SLS with the option '--slstype'."
        << " This is incorrect, we only accept 'yalsat' and 'walksat'"
        << endl;
        exit(-1);
    }

    if (conf.yalsat_max_mems < 1) {
        cout << "ERROR: '--walkmems' must be at least 1" << endl;
        exit(-1);
    }

    if (conf.sls_every_n < 1) {
        cout << "ERROR: '--walkeveryn' must be at least 1" << endl;
        exit(-1);
    }

    if (conf.maxXorToFind > MAX_XOR_RECOVER_SIZE) {
        cout << "ERROR: The '--maxxorsize' parameter cannot be larger than " << MAX_XOR_RECOVER_SIZE << endl;
        exit(-1);
    }

    if (conf.shortTermHistorySize <= 0) {
        cout
        << "You MUST give a short term history size (\"--gluehist\")" << endl
        << "  greater than 0!"
        << endl;

        std::exit(-1);
    }

    if (!decisions_for_model_fname.empty() && max_nr_of_solutions > 1) {
        std::cerr << "ERROR: dumping decisions for multi-solution makes no sense. Exiting." << endl;
        std::exit(-1);
    }

    if (!decisions_for_model_fname.empty()) {
        conf.need_decisions_reaching = true;
    }

    if (max_nr_of_solutions > 1) {
        conf.need_decisions_reaching = true;
    }

    if (conf.random_var_freq < 0 || conf.random_var_freq > 1) {
        throw WrongParam(lexical_cast<string>(conf.random_var_freq), "Illegal random var frequency ");
    }

    if (conf.preprocess != 0) {
        conf.simplify_at_startup = 1;
        conf.varelim_time_limitM *= 5;
        conf.orig_global_timeout_multiplier *= 1.5;
        if (conf.doCompHandler) {
            conf.doCompHandler = false;
            if (conf.verbosity) {
                cout << "c Cannot handle components when preprocessing. Turning it off." << endl;
            }
        }

        if (num_threads > 1) {
            num_threads = 1;
            cout << "c Cannot handle multiple threads for preprocessing. Setting to 1." << endl;
        }


        if (!redDumpFname.empty()
            || !irredDumpFname.empty()
        ) {
            std::cerr << "ERROR: dumping clauses with preprocessing makes no sense. Exiting" << endl;
            std::exit(-1);
        }

        if (max_nr_of_solutions > 1) {
            std::cerr << "ERROR: multi-solutions make no sense with preprocessing. Exiting." << endl;
            std::exit(-1);
        }

        if (!decisions_for_model_fname.empty()) {
            std::cerr << "ERROR: dumping decisions for model make no sense with preprocessing. Exiting." << endl;
            std::exit(-1);
        }

        if (!filesToRead.empty()) {
            assert(false && "we should never reach this place, filesToRead has not been populated yet");
            exit(-1);
        }

        if (!debugLib.empty()) {
            std::cerr << "ERROR: debugLib makes no sense with preprocessing. Exiting." << endl;
            std::exit(-1);
        }

        if (vm.count("schedule")) {
            std::cerr << "ERROR: Pleaase adjust the --preschedule not the --schedule when preprocessing. Exiting." << endl;
            std::exit(-1);
        }

        if (vm.count("occschedule")) {
            std::cerr << "ERROR: Pleaase adjust the --preoccschedule not the --occschedule when preprocessing. Exiting." << endl;
            std::exit(-1);
        }

        if (!vm.count("preschedule")) {
            conf.simplify_schedule_startup = conf.simplify_schedule_preproc;
        }

        if (!vm.count("eratio")) {
            conf.varElimRatioPerIter = 2.0;
        }
    } else {
        if (!vm["savedstate"].defaulted()) {
            cout << "ERROR: It does not make sense to have --savedstate passed and not use preprocessing" << endl;
            exit(-1);
        }
    }

    if (vm.count("dumpresult")) {
        resultfile = new std::ofstream;
        resultfile->open(resultFilename.c_str());
        if (!(*resultfile)) {
            cout
            << "ERROR: Couldn't open file '"
            << resultFilename
            << "' for writing result!"
            << endl;
            std::exit(-1);
        }
    }

    parse_polarity_type();

    //Conflict
    if (vm.count("maxdump") && redDumpFname.empty()) {
        throw WrongParam("maxdump", "--dumpred <filename> must be activated if issuing --maxdump <size>");
    }

    parse_restart_type();

    if (conf.preprocess == 2) {
        if (vm.count("input") == 0) {
            cout << "ERROR: When post-processing you must give the solution as the positional argument"
            << endl;
            std::exit(-1);
        }

        vector<string> solution = vm["input"].as<vector<string> >();
        if (solution.size() > 1) {
            cout << "ERROR: When post-processing you must give only the solution as the positional argument."
            << endl
            << "The saved state must be given as the argument '--savedsate X'"
            << endl;
            std::exit(-1);
        }
        conf.solution_file = solution[0];
    } else if (vm.count("input")) {
        filesToRead = vm["input"].as<vector<string> >();
        if (conf.preprocess == 1) {
            filesToRead.resize(1);
        }

        if (!vm.count("sqlitedb")) {
            sqlite_filename = filesToRead[0] + ".sqlite";
        } else {
            sqlite_filename = vm["sqlitedb"].as<string>();
        }
        fileNamePresent = true;
    } else {
        fileNamePresent = false;
    }

    if (conf.preprocess == 1) {
        if (vm["input"].as<vector<string> >().size() + vm.count("drat") > 2) {
            cout << "ERROR: When preprocessing, you must give the simplified file name as 2nd argument" << endl;
            cout << "You gave this many inputs: "
                << vm["input"].as<vector<string> >().size()+vm.count("drat")
                << endl;

            for(string s: vm["input"].as<vector<string> >()) {
                cout << " --> " << s << endl;
            }
            if (vm.count("drat")) {
                cout << " --> " << vm["drat"].as<string>() << endl;
            }
            exit(-1);
        }
        if (vm["input"].as<vector<string> >().size() > 1) {
            conf.simplified_cnf = vm["input"].as<vector<string> >()[1];
        } else {
            conf.simplified_cnf = vm["drat"].as<string>();
        }
    }

    if (conf.preprocess == 2) {
        if (vm.count("drat")) {
            cout << "ERROR: When postprocessing, you must NOT give a 2nd argument" << endl;
            std::exit(-1);
        }
    }

    if (conf.preprocess == 0 &&
        (vm.count("drat") || conf.simulate_drat)
    ) {
        handle_drat_option();
    }

    if (conf.verbosity) {
        cout << "c Outputting solution to console" << endl;
    }
}

void Main::parseCommandLine()
{
    need_clean_exit = 0;

    //Reconstruct the command line so we can emit it later if needed
    for(int i = 0; i < argc; i++) {
        commandLine += string(argv[i]);
        if (i+1 < argc) {
            commandLine += " ";
        }
    }

    add_supported_options();
    p.add("input", 1);
    p.add("drat", 1);
    all_options.add(help_options_complicated);
    all_options.add(hiddenOptions);

    help_options_simple
    .add(generalOptions)
    ;

    check_options_correctness();
    if (vm.count("version")) {
        printVersionInfo();
        std::exit(0);
    }

    try {
        manually_parse_some_options();
    } catch(WrongParam& wp) {
        cerr << "ERROR: " << wp.getMsg() << endl;
        exit(-1);
    }
}

void Main::check_num_threads_sanity(const unsigned thread_num) const
{
    const unsigned num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) {
        //Library doesn't know much, we can't do any checks.
        return;
    }

    if (thread_num > num_cores && conf.verbosity) {
        std::cout
        << "c WARNING: Number of threads requested is more than the number of"
        << " cores reported by the system.\n"
        << "c WARNING: This is not a good idea in general. It's best to set the"
        << " number of threads to the number of real cores" << endl;
    }
}

void Main::dump_red_file()
{
    if (dump_red_fname.length() == 0)
        return;

    std::ofstream* dumpfile = new std::ofstream;
    dumpfile->open(dump_red_fname.c_str());
    if (!(*dumpfile)) {
        cout
        << "ERROR: Couldn't open file '"
        << resultFilename
        << "' for writing redundant clauses!"
        << endl;
        std::exit(-1);
    }

    bool ret = true;
    vector<Lit> lits;
    solver->start_getting_small_clauses(dump_red_max_len, dump_red_max_glue);
    while(ret) {
        ret = solver->get_next_small_clause(lits);
        if (ret) {
            *dumpfile << lits << " " << 0 << endl;
        }
    }
    solver->end_getting_small_clauses();

    delete dumpfile;
}

int Main::solve()
{
    solver = new SATSolver((void*)&conf);
    solverToInterrupt = solver;
    if (dratf) {
        solver->set_drat(dratf, clause_ID_needed);
    }
    if (vm.count("maxtime")) {
        solver->set_max_time(maxtime);
    }
    if (vm.count("maxconfl")) {
        solver->set_max_confl(maxconfl);
    }

    check_num_threads_sanity(num_threads);
    solver->set_num_threads(num_threads);
    if (sql != 0) {
        solver->set_sqlite(sqlite_filename);
    }

    //Print command line used to execute the solver: for options and inputs
    if (conf.verbosity) {
        printVersionInfo();
        cout
        << "c Executed with command line: "
        << commandLine
        << endl;
    }

    solver->add_sql_tag("commandline", commandLine);
    solver->add_sql_tag("verbosity", lexical_cast<string>(conf.verbosity));
    solver->add_sql_tag("threads", lexical_cast<string>(num_threads));
    solver->add_sql_tag("version", solver->get_version());
    solver->add_sql_tag("SHA-revision", solver->get_version_sha1());
    solver->add_sql_tag("env", solver->get_compilation_env());
    #ifdef __GNUC__
    solver->add_sql_tag("compiler", "gcc-" __VERSION__);
    #else
    solver->add_sql_tag("compiler", "non-gcc");
    #endif

    //Parse in DIMACS (maybe gzipped) files
    //solver->log_to_file("mydump.cnf");
    if (conf.preprocess != 2) {
        parseInAllFiles(solver);
    }

    lbool ret = multi_solutions();

    if (conf.preprocess != 1) {
        if (ret == l_Undef && conf.verbosity) {
            cout
            << "c Not finished running -- signal caught or some maximum reached"
            << endl;
        }
        if (conf.verbosity) {
            solver->print_stats();
        }
        if (ret == l_True) {
            dump_red_file();
        }
    }
    printResultFunc(&cout, false, ret);
    if (resultfile) {
        printResultFunc(resultfile, true, ret);
    }

    return correctReturnValue(ret);
}

void Main::dump_decisions_for_model()
{
    assert(max_nr_of_solutions == 1);
    assert(solver->okay());

    std::ofstream decfile;
    decfile.open(decisions_for_model_fname.c_str());
    if (!(decfile)) {
        cout
        << "ERROR: Couldn't open file '"
        << decisions_for_model_fname
        << "' for writing decisions!"
        << endl;
        std::exit(-1);
    }

    if (!solver->get_decision_reaching_valid()) {
        decfile << "INVALID" << endl;
        return;
    }

    if (conf.verbosity) {
        cout << "c size of get_decisions_reaching_model: "
        << solver->get_decisions_reaching_model().size()
        << endl;
    }
    for(const Lit l: solver->get_decisions_reaching_model()) {
        decfile << l << " 0" << endl;
    }
}

lbool Main::multi_solutions()
{
    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    while(current_nr_of_solutions < max_nr_of_solutions && ret == l_True) {
        ret = solver->solve(NULL, only_sampling_solution);
        current_nr_of_solutions++;
        if (ret == l_True && !decisions_for_model_fname.empty()) {
            dump_decisions_for_model();
            assert(max_nr_of_solutions == 1);
        }

        if (ret == l_True && current_nr_of_solutions < max_nr_of_solutions) {
            printResultFunc(&cout, false, ret);
            if (resultfile) {
                printResultFunc(resultfile, true, ret);
            }

            if (conf.verbosity) {
                cout
                << "c Number of solutions found until now: "
                << std::setw(6) << current_nr_of_solutions
                << endl;
            }
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            solver->print_removed_vars();
            #endif

            //Banning found solution
            vector<Lit> lits;
            if (sampling_vars.empty()) {
                if (solver->get_decision_reaching_valid()) {
                    //only decision vars
                    for (Lit lit: solver->get_decisions_reaching_model()) {
                      lits.push_back(~lit);
                    }
                } else {
                    //all of the solution
                    for (uint32_t var = 0; var < solver->nVars(); var++) {
                        if (solver->get_model()[var] != l_Undef) {
                            lits.push_back( Lit(var, (solver->get_model()[var] == l_True)? true : false) );
                        }
                    }
                }
            } else {
              for (const uint32_t var: sampling_vars) {
                  if (solver->get_model()[var] != l_Undef) {
                      lits.push_back( Lit(var, (solver->get_model()[var] == l_True)? true : false) );
                  }
              }
            }
            solver->add_clause(lits);
        }
    }
    return ret;
}

///////////
// Useful helper functions
///////////

void Main::printVersionInfo()
{
    cout << solver->get_text_version_info();
}

int Main::correctReturnValue(const lbool ret) const
{
    int retval = -1;
    if (ret == l_True) {
        retval = 10;
    } else if (ret == l_False) {
        retval = 20;
    } else if (ret == l_Undef) {
        retval = 15;
    } else {
        std::cerr << "Something is very wrong, output is neither l_Undef, nor l_False, nor l_True" << endl;
        exit(-1);
    }

    if (zero_exit_status) {
        return 0;
    } else {
        return retval;
    }
}
