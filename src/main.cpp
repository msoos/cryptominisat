/*
Copyright (C) 2009-2022 Authors of CryptoMiniSat, see AUTHORS file

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

#define DEBUG_DIMACSPARSER_CMS

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
#include "cryptominisat.h"
#include "signalcode.h"
#include "argparse.hpp"

using namespace CMSat;

using std::cout;
using std::cerr;
using std::endl;
using std::map;
double wallclock_time_started = 0.0;

struct WrongParam {
    WrongParam(const string& _param, const string& _msg) : param(_param) , msg(_msg) {}
    const string& getMsg() const { return msg; }
    const string& getParam() const { return param; }
    string param;
    string msg;
};

Main::Main(int _argc, char** _argv) :
    argc(_argc)
    , argv(_argv)
    , fileNamePresent (false)
{
}

void Main::readInAFile(SATSolver* solver2, const string& filename)
{
    solver2->add_sql_tag("filename", filename);
    if (conf.verbosity) cout << "c Reading file '" << filename << "'" << endl;
    #ifndef USE_ZLIB
    FILE * in = fopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<FILE*, FN>, SATSolver> parser(solver2, &debugLib, conf.verbosity);
    #else
    gzFile in = gzopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<gzFile, GZ>, SATSolver> parser(solver2, &debugLib, conf.verbosity);
    #endif

    if (in == NULL) {
        std::cerr
        << "ERROR! Could not open file '"
        << filename
        << "' for reading: " << strerror(errno) << endl;

        std::exit(1);
    }

    bool strict_header = false;
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
        while (ss >> i) {
            const uint32_t var = i-1;
            sampling_vars.push_back(var);
            if (ss.peek() == ',' || ss.peek() == ' ') ss.ignore();
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
            if (conf.verbosity) {
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
    if (conf.verbosity) cout << "c Reading from standard input... Use '-h' or '--help' for help." << endl;

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
    DimacsParser<StreamBuffer<FILE*, FN>, SATSolver> parser(solver2, &debugLib, conf.verbosity);
    #else
    DimacsParser<StreamBuffer<gzFile, GZ>, SATSolver> parser(solver2, &debugLib, conf.verbosity);
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
    solver->add_sql_tag("stdin", fileNamePresent ? "False" : "True");
    if (!fileNamePresent) readInStandardInput(solver2);
    else readInAFile(solver2, input_file);

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
    if (ret == l_True && !printResult && !toFile)
    {
        cout << "c Not printing satisfying assignment. "
        "Use the '--printsol 1' option for that" << endl;
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

/* clang-format off */
void Main::add_supported_options()
{
  // Declare the supported options.
    program.add_argument("-h", "--help")
        .help("Print extensive help")
        .default_value(false);
    program.add_argument("-v", "--version")
        .help("Print version info")
        .flag();
    program.add_argument("--verb")
        .action([&](const auto& a) {conf.verbosity = std::atoi(a.c_str());})
        .default_value(conf.verbosity)
        .help("[0-10] Verbosity of solver. 0 = only solution");
    program.add_argument("--maxtime")
        .help("Stop solving after this much time (s)")
        .scan<'g', double>();
    program.add_argument("--maxconfl")
        .help("Stop solving after this many conflicts")
        .scan<'d', uint64_t>();
    program.add_argument("-r", "--random")
        .action([&](const auto& a) {conf.origSeed = std::atoi(a.c_str());})
        .default_value(conf.origSeed)
        .help("[0..] Random seed");
    program.add_argument("-t", "--threads")
        .default_value(1)
        .action([&](const auto& a) {num_threads = std::atoi(a.c_str());})
        .help("Number of threads");
    program.add_argument("-m", "--mult")
        .action([&](const auto& a) {conf.orig_global_timeout_multiplier = std::atof(a.c_str());})
        .default_value(conf.orig_global_timeout_multiplier)
        .help("Time multiplier for all simplification cutoffs");
    program.add_argument("--nextm")
        .action([&](const auto& a) {conf.global_next_multiplier = std::atof(a.c_str());})
        .default_value(conf.global_next_multiplier)
        .help("Global multiplier when the next inprocessing should take place");
    program.add_argument("--memoutmult")
        .action([&](const auto& a) {conf.var_and_mem_out_mult = std::atof(a.c_str());})
        .default_value(conf.var_and_mem_out_mult)
        .help("Multiplier for memory-out checks on inprocessing functions. It limits things such as clause-link-in. Useful when you have limited memory but still want to do some inprocessing");
    program.add_argument("--maxsol")
        .action([&](const auto& a) {max_nr_of_solutions = std::atoll(a.c_str());})
        .default_value(max_nr_of_solutions)
        .help("Search for given amount of solutions. Thanks to Jannis Harder for the decision-based banning idea");
    program.add_argument("--polar")
        .default_value("auto")
        .help("{true,false,rnd,auto,stable} Selects polarity mode. 'true' -> selects only positive polarity when branching. 'false' -> selects only negative polarity when branching. 'auto' -> selects last polarity used (also called 'caching')");
    program.add_argument("--scc")
        .action([&](const auto& a) {conf.doFindAndReplaceEqLits = std::atoi(a.c_str());})
        .default_value(conf.doFindAndReplaceEqLits)
        .help("Find equivalent literals through SCC and replace them");

    #ifdef STATS_NEEDED
    program.add_argument("--clid").
        .flag()
        .action([&](const auto& a) {clause_ID_needed = true;})
        .help("Add clause IDs to FRAT output")
    #endif

    #ifdef FINAL_PREDICTOR
    po::options_description predictOptions("Predict options");
    predictOptions.add_options()
    program.add_argument("--predloc")
        .action([&](const auto& a) {conf.pred_conf_location = std::atoi(a.c_str());})
         .default_value(conf.pred_conf_location)
        .help("Directory where predictor_short.json, predictor_long.json, predictor_forever.json are")
    program.add_argument("--predtype")
        .action([&](const auto& a) {conf.predictor_type = std::atoi(a.c_str());})
        .default_value(conf.predictor_type)
        .help("Type of predictor. Supported: py, xgb, lgbm")
    program.add_argument("--predtables")
        .action([&](const auto& a) {conf.pred_tables = std::atoi(a.c_str());})
        .default_value(conf.pred_tables)
        .help("000 = normal for all, 111 = ancestor for all")
    program.add_argument("--predbestfeats")
         .action([&](const auto& a) {conf.predict_best_feat_fname = a);})
         .default_value(conf.predict_best_feat_fname)
        .help("Model python file name")

    //size
    program.add_argument("--predshortsize")
        .action([&](const auto& a) {conf.pred_short_size = std::atoi(a.c_str());})
        .default_value(conf.pred_short_size)
        .help("Pred short multiplier")
    program.add_argument("--predlongsize")
        .action([&](const auto& a) {conf.pred_long_size = std::atoi(a.c_str());})
        .default_value(conf.pred_long_size)
        .help("Pred long multiplier")
    program.add_argument("--predforeversize")
        .action([&](const auto& a) {conf.pred_forever_size = std::atoi(a.c_str());})
        .default_value(conf.pred_forever_size)
        .help("Pred forever multiplier")
    program.add_argument("--predforevercutoff")
        .action([&](const auto& a) {conf.pred_forever_cutoff = std::atoi(a.c_str());})
        .default_value(conf.pred_forever_cutoff)
        .help("If non-zero, ONLY this determines what's MOVED to or KEPT IN 'forever'.")
    program.add_argument("--predforeverpow")
        .action([&](const auto& a) {conf.pred_forever_size_pow = std::atof(a.c_str());})
        .default_value(conf.pred_forever_size_pow)
        .help("Pred forever power to raise the conflicts to")
    program.add_argument("--ordertier2by")
        .action([&](const auto& a) {conf.order_tier2_by = std::atoi(a.c_str());})
        .default_value(conf.order_tier2_by)
        .help("Order Tier 2 by Tier 2/1/0 prediction")

    //move or del?
    program.add_argument("--movefromtier0")
        .action([&](const auto& a) {conf.move_from_tier0 = std::atoi(a.c_str());})
        .default_value(conf.move_from_tier0)
        .help("Move from tier0 to tier1? If set to 0, then it's deleted instead of moved.")
    program.add_argument("--movefromtier1")
        .action([&](const auto& a) {conf.move_from_tier1 = std::atoi(a.c_str());})
        .default_value(conf.move_from_tier1)
        .help("Move from tier1 to tier2? If set to 0, then it's deleted instead of moved.")

    //printing
    program.add_argument("--dumppreddistrib")
        .action([&](const auto& a) {conf.dump_pred_distrib = std::atoi(a.c_str());})
        .default_value(conf.dump_pred_distrib)
        .help("Dump predict distirution to pred_distrib.csv")

    //chunk
    program.add_argument("--predlongchunk")
        .action([&](const auto& a) {conf.pred_long_chunk = std::atoi(a.c_str());})
        .default_value(conf.pred_long_chunk)
        help("Pred long chunk multiplier")
    program.add_argument("--predforeverchunk")
        .action([&](const auto& a) {conf.pred_forever_chunk = std::atoi(a.c_str());})
        .default_value(conf.pred_forever_chunk)
        help("Pred forever chunk multiplier")
    program.add_argument("--predforeverchunkmult")
        .action([&](const auto& a) {conf.pred_forever_chunk_mult = std::atof(a.c_str());})
        .default_value(conf.pred_forever_chunk_mult)
        help("Pred forever chunk should be POW multiplied just like forever. 0/1 (i.e. true/false) option")

    //Check intervals for LONG and FOREVER
    program.add_argument("--predlongcheckn")
        .action([&](const auto& a) {conf.pred_long_check_every_n = std::atoi(a.c_str());})
        .default_value(conf.pred_long_check_every_n)
        .help("Pred long check over limit every N")
    program.add_argument("--predforevercheckn")
         .action([&](const auto& a) {conf.pred_forever_check_every_n = std::atoi(a.c_str());})
        .default_value(conf.pred_forever_check_every_n)
        .help("Pred forever check over limit every N")

    // Some old stuff
    program.add_argument("--preddistillsmallgue")
        .action([&](const auto& a) {conf.pred_distill_only_smallgue = std::atoi(a.c_str());})
        .default_value(conf.pred_distill_only_smallgue)
        .help("Only distill small glue clauses")

    // Lock clauses in
    program.add_argument("--preddontmovetime")
        .action([&](const auto& a) {conf.pred_dontmove_until_timeinside = std::atoi(a.c_str());})
        .default_value(conf.pred_dontmove_until_timeinside)
        .help("Don't move clause until it's time has passed. For lev0 and lev1 only. If 1 = half time needs to pass (e.g. if we check every 50k conflicts, it must have been in the solver for 25k or it's force-kept). If 2 = the full time is needed, in the example, 25k.")
    #endif


    /* po::options_description restartOptions("Restart options"); */
    /* restartOptions.add_options() */
    std::ostringstream s_local_glue_multiplier;
    s_local_glue_multiplier << std::setprecision(4) << conf.local_glue_multiplier;
    program.add_argument("--restart")
        .default_value("auto")
        .help("{geom, glue, luby}  Restart strategy to follow.");
    program.add_argument("--rstfirst")
       .action([&](const auto& a) {conf.restart_first = std::atoi(a.c_str());})
       .default_value(conf.restart_first)
       .help("The size of the base restart");
    program.add_argument("--gluehist")
        .action([&](const auto& a) {conf.shortTermHistorySize = std::atoi(a.c_str());})
        .default_value(conf.shortTermHistorySize)
        .help("The size of the moving window for short-term glue history of redundant clauses. If higher, the minimal number of conflicts between restarts is longer");
    program.add_argument("--lwrbndblkrest")
       .action([&](const auto& a) {conf.lower_bound_for_blocking_restart = std::atoi(a.c_str());})
       .default_value(conf.lower_bound_for_blocking_restart)
       .help("Lower bound on blocking restart -- don't block before this many conflicts");
    program.add_argument("--locgmult" )
       .action([&](const auto& a) {conf.local_glue_multiplier = std::atof(a.c_str());})
       .default_value(conf.local_glue_multiplier)
       /* .s_local_glue_multiplier.str()*/
       .help("The multiplier used to determine if we should restart during glue-based restart");
    program.add_argument("--ratiogluegeom")
        .action([&](const auto& a) {conf.ratio_glue_geom = std::atof(a.c_str());})
        .default_value(conf.ratio_glue_geom)
        .help("Ratio of glue vs geometric restarts -- more is more glue");
    program.add_argument("--blockingglue")
        .action([&](const auto& a) {conf.do_blocking_restart = std::atoi(a.c_str());})
        .default_value(conf.do_blocking_restart)
        .help("Do blocking restart for glues");

    std::ostringstream s_incclean;
    std::ostringstream s_adjust_low;
    s_adjust_low << std::setprecision(2) << conf.adjust_glue_if_too_many_tier0;

    /* po::options_description reduceDBOptions("Redundant clause options"); */
    /* reduceDBOptions.add_options() */
    program.add_argument("--gluecut0")
        .action([&](const auto& a) {conf.glue_put_lev0_if_below_or_eq = std::atoi(a.c_str());})
        .default_value(conf.glue_put_lev0_if_below_or_eq)
        .help("Glue value for lev 0 ('keep') cut");
    program.add_argument("--gluecut1")
        .action([&](const auto& a) {conf.glue_put_lev1_if_below_or_eq = std::atoi(a.c_str());})
        .default_value(conf.glue_put_lev1_if_below_or_eq)
        .help("Glue value for lev 1 cut ('give another shot'");
    program.add_argument("--adjustglue")
        .action([&](const auto& a) {conf.adjust_glue_if_too_many_tier0 = std::atof(a.c_str());})
        .default_value(conf.adjust_glue_if_too_many_tier0)
        //, s_adjust_low.str()
        .help("If more than this % of clauses is LOW glue (level 0) then lower the glue cutoff by 1 -- once and never again");
    program.add_argument("--everylev1")
        .action([&](const auto& a) {conf.every_lev1_reduce = std::atoi(a.c_str());})
        .default_value(conf.every_lev1_reduce)
        .help("Reduce lev1 clauses every N");
    program.add_argument("--everylev2")
        .action([&](const auto& a) {conf.every_lev2_reduce = std::atoi(a.c_str());})
        .default_value(conf.every_lev2_reduce)
        .help("Reduce lev2 clauses every N");
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    program.add_argument("--everypred")
        .action([&](const auto& a) {conf.every_pred_reduce = std::atoi(a.c_str());})
        .default_value(conf.every_pred_reduce)
        .help("Reduce final predictor (lev3) clauses every N, and produce data at every N in case of STATS_NEEDED");
    #endif
    program.add_argument("--lev1usewithin")
        .action([&](const auto& a) {conf.must_touch_lev1_within = std::atoi(a.c_str());})
        .default_value(conf.must_touch_lev1_within)
        .help("Learnt clause must be used in lev1 within this timeframe or be dropped to lev2");

    /* po::options_description varPickOptions("Variable branching options"); */
    /* varPickOptions.add_options() */
    program.add_argument("--branchstr")
        .action([&](const auto& a) {conf.branch_strategy_setup = a;})
        .default_value(conf.branch_strategy_setup)
        .help("Branch strategy string that switches between different branch strategies while solving e.g. 'vsids1+vsids2'");

    program.add_argument("--nobansol")
        .flag()
        .action([&](const auto&) {dont_ban_solutions = true;})
        .help("Don't ban the solution once it's found");
    program.add_argument("--debuglib")
        .action([&](const auto& a) {debugLib = a;})
        .help("Parse special comments to run solve/simplify during parsing of CNF");

    /* po::options_description breakid_options("Breakid options"); */
    /* breakid_options.add_options() */
    program.add_argument("--breakid")
        .action([&](const auto& a) {conf.doBreakid = std::atoi(a.c_str());})
        .default_value(conf.doBreakid)
        .help("Run BreakID to break symmetries.");
    program.add_argument("--breakideveryn")
        .action([&](const auto& a) {conf.breakid_every_n = std::atoi(a.c_str());})
        .default_value(conf.breakid_every_n)
        .help("Run BreakID every N simplification iterations");
    program.add_argument("--breakidmaxlits")
        .action([&](const auto& a) {conf.breakid_lits_limit_K = std::atoll(a.c_str());})
        .default_value(conf.breakid_lits_limit_K)
        .help("Maximum number of literals in thousands. If exceeded, BreakID will not run");
    program.add_argument("--breakidmaxcls")
        .action([&](const auto& a) {conf.breakid_cls_limit_K = std::atoll(a.c_str());})
        .default_value(conf.breakid_cls_limit_K)
        .help("Maximum number of clauses in thousands. If exceeded, BreakID will not run");
    program.add_argument("--breakidmaxvars")
        .action([&](const auto& a) {conf.breakid_vars_limit_K = std::atoll(a.c_str());})
        .default_value(conf.breakid_vars_limit_K)
        .help("Maximum number of variables in thousands. If exceeded, BreakID will not run");;
    program.add_argument("--breakidtime")
        .action([&](const auto& a) {conf.breakid_time_limit_K = std::atoll(a.c_str());})
        .default_value(conf.breakid_time_limit_K)
        .help("Maximum number of steps taken during automorphism finding.");
    program.add_argument("--breakidcls")
        .action([&](const auto& a) {conf.breakid_max_constr_per_permut = std::atoi(a.c_str());})
        .default_value(conf.breakid_max_constr_per_permut)
        .help("Maximum number of breaking clauses per permutation.");;
    program.add_argument("--breakidmatrix")
        .action([&](const auto& a) {conf.breakid_matrix_detect = std::atoi(a.c_str());})
        .default_value(conf.breakid_matrix_detect)
        .help("Detect matrix row interchangability");
    ;

    /* po::options_description sls_options("Stochastic Local Search options"); */
    /* sls_options.add_options() */
    program.add_argument("--sls")
        .action([&](const auto& a) {conf.doSLS = std::atoi(a.c_str());})
        .default_value(conf.doSLS)
        .help("Run SLS during simplification");
    program.add_argument("--slstype")
        .action([&](const auto& a) {conf.which_sls = a;})
        .default_value(conf.which_sls)
        .help("Which SLS to run. Allowed values: walksat, yalsat, ccnr, ccnr_yalsat");
    program.add_argument("--slsmaxmem")
        .action([&](const auto& a) {conf.sls_memoutMB = std::atoi(a.c_str());})
        .default_value(conf.sls_memoutMB)
        .help("Maximum number of MB to give to SLS solver. Doesn't run SLS solver if the memory usage would be more than this.");
    program.add_argument("--slseveryn")
        .action([&](const auto& a) {conf.sls_every_n = std::atoi(a.c_str());})
        .default_value(conf.sls_every_n)
        .help("Run SLS solver every N simplifications only");
    program.add_argument("--yalsatmems")
        .action([&](const auto& a) {conf.yalsat_max_mems = std::atoi(a.c_str());})
        .default_value(conf.yalsat_max_mems)
        .help("Run Yalsat with this many mems*million timeout. Limits time of yalsat run");
    program.add_argument("--walksatruns")
        .action([&](const auto& a) {conf.walksat_max_runs = std::atoi(a.c_str());})
        .default_value(conf.walksat_max_runs)
        .help("Max 'runs' for WalkSAT. Limits time of WalkSAT run");
    program.add_argument("--slsgetphase")
        .action([&](const auto& a) {conf.sls_get_phase = std::atoi(a.c_str());})
        .default_value(conf.sls_get_phase)
        .help("Get phase from SLS solver, set as new phase for CDCL");
    program.add_argument("--slsccnraspire")
        .action([&](const auto& a) {conf.sls_ccnr_asipire = std::atoi(a.c_str());})
        .default_value(conf.sls_ccnr_asipire)
        .help("Turn aspiration on/off for CCANR");
    program.add_argument("--slstobump")
        .action([&](const auto& a) {conf.sls_how_many_to_bump = std::atoi(a.c_str());})
        .default_value(conf.sls_how_many_to_bump)
        .help("How many variables to bump in CCNR");
    program.add_argument("--slstobumpmaxpervar")
        .action([&](const auto& a) {conf.sls_bump_var_max_n_times = std::atoi(a.c_str());})
        .default_value(conf.sls_bump_var_max_n_times)
        .help("How many times to bump an individual variable's activity in CCNR");
    program.add_argument("--slsbumptype")
        .action([&](const auto& a) {conf.sls_bump_type = std::atoi(a.c_str());})
        .default_value(conf.sls_bump_type)
        .help("How to calculate what variable to bump. 1 = clause-based, 2 = var-flip-based, 3 = var-score-based");

    /* po::options_description probeOptions("Probing options"); */
    /* probeOptions.add_options() */
    program.add_argument("--transred")
        .action([&](const auto& a) {conf.doTransRed = std::atoi(a.c_str());})
        .default_value(conf.doTransRed)
        .help("Remove useless binary clauses (transitive reduction)");
    program.add_argument("--intree")
        .action([&](const auto& a) {conf.doIntreeProbe = std::atoi(a.c_str());})
        .default_value(conf.doIntreeProbe)
        .help("Carry out intree-based probing");
    program.add_argument("--intreemaxm")
        .action([&](const auto& a) {conf.intree_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.intree_time_limitM)
        .help("Time in mega-bogoprops to perform intree probing");
    program.add_argument("--otfhyper")
        .action([&](const auto& a) {conf.do_hyperbin_and_transred = std::atoi(a.c_str());})
        .default_value(conf.do_hyperbin_and_transred)
        .help("Perform hyper-binary resolution during probing");

    /* po::options_description simp_schedules("Simplification schedules"); */
    /* simp_schedules.add_options() */
    std::ostringstream ssERatio;
    ssERatio << std::setprecision(4) << conf.varElimRatioPerIter;
    std::ostringstream s_num_conflicts_of_search_inc;
    s_num_conflicts_of_search_inc << std::setprecision(4) << conf.num_conflicts_of_search_inc;
    program.add_argument("--schedsimp")
        .action([&](const auto& a) {conf.do_simplify_problem = std::atoi(a.c_str());})
        .default_value(conf.do_simplify_problem)
        .help("Perform simplification rounds. If 0, we never perform any.");
    program.add_argument("--presimp")
        .action([&](const auto& a) {conf.simplify_at_startup = std::atoi(a.c_str());})
        .default_value(conf.simplify_at_startup)
        .help("Perform simplification at the very start");
    program.add_argument("--allpresimp")
        .action([&](const auto& a) {conf.simplify_at_every_startup = std::atoi(a.c_str());})
        .default_value(conf.simplify_at_every_startup)
        .help("Perform simplification at EVERY start -- only matters in library mode");
    program.add_argument("-n", "--nonstop")
        .action([&](const auto& a) {conf.never_stop_search = std::atoi(a.c_str());})
        .default_value(conf.never_stop_search)
        .help("Never stop the search() process in class SATSolver");
    program.add_argument("--maxnumsimppersolve")
        .action([&](const auto& a) {conf.max_num_simplify_per_solve_call = std::atoi(a.c_str());})
        .default_value(conf.max_num_simplify_per_solve_call)
        .help("Maximum number of simplifiactions to perform for every solve() call. After this, no more inprocessing will take place.");

    program.add_argument("--schedule")
        .action([&](const auto& a) {conf.simplify_schedule_nonstartup = a;})
        .help("Schedule for simplification during run");
    program.add_argument("--preschedule")
        .action([&](const auto& a) {conf.simplify_schedule_startup = a;})
        .help("Schedule for simplification at startup");
    program.add_argument("--occsimp")
        .action([&](const auto& a) {conf.perform_occur_based_simp = std::atoi(a.c_str());})
        .default_value(conf.perform_occur_based_simp)
        .help("Perform occurrence-list-based optimisations (variable elimination, subsumption, bounded variable addition...)");
    program.add_argument("--confbtwsimp")
        .action([&](const auto& a) {conf.num_conflicts_of_search = std::atoll(a.c_str());})
        .default_value(conf.num_conflicts_of_search)
        .help("Start first simplification after this many conflicts");
    program.add_argument("--confbtwsimpinc")
        .action([&](const auto& a) {conf.num_conflicts_of_search_inc = std::atof(a.c_str());})
        .default_value(conf.num_conflicts_of_search_inc)
        //s_num_conflicts_of_search_inc.str()
        .help("Simp rounds increment by this power of N");

    /* po::options_description tern_res_options("Ternary resolution"); */
    /* tern_res_options.add_options() */
    std::ostringstream tern_keep;
    tern_keep << std::setprecision(2) << conf.ternary_keep_mult;
    std::ostringstream tern_max_create;
    tern_max_create << std::setprecision(2) << conf.ternary_max_create;
    program.add_argument("--tern")
        .action([&](const auto& a) {conf.doTernary = std::atoi(a.c_str());})
        .default_value(conf.doTernary)
        .help("Perform Ternary resolution");
    program.add_argument("--terntimelim")
        .action([&](const auto& a) {conf.ternary_res_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.ternary_res_time_limitM)
        .help("Time-out in bogoprops M of ternary resolution as per paper 'Look-Ahead Versus Look-Back for Satisfiability Problems'");
    program.add_argument("--ternkeep")
        .action([&](const auto& a) {conf.ternary_keep_mult = std::atof(a.c_str());})
        .default_value(conf.ternary_keep_mult)
        //, tern_keep.str()
        .help("Keep ternary resolution clauses only if they are touched within this multiple of 'lev1usewithin'");
    program.add_argument("--terncreate")
        .action([&](const auto& a) {conf.ternary_max_create = std::atof(a.c_str());})
        .default_value(conf.ternary_max_create)
        //, tern_max_create.str()
        .help("Create only this multiple (of linked in cls) ternary resolution clauses per simp run");
    program.add_argument("--ternbincreate")
        .action([&](const auto& a) {conf.allow_ternary_bin_create = std::atoi(a.c_str());})
        .default_value(conf.allow_ternary_bin_create)
        //, tern_max_create.str()
        .help("Allow ternary resolving to generate binary clauses");

    /* po::options_description occ_mem_limits("Occ-based simplification memory limits"); */
    /* occ_mem_limits.add_options() */
    program.add_argument("--occredmax")
        .action([&](const auto& a) {conf.maxRedLinkInSize = std::atoi(a.c_str());})
        .default_value(conf.maxRedLinkInSize)
        .help("Don't add to occur list any redundant clause larger than this");
    program.add_argument("--occredmaxmb")
        .action([&](const auto& a) {conf.maxOccurRedMB = std::atof(a.c_str());})
        .default_value(conf.maxOccurRedMB)
        .help("Don't allow redundant occur size to be beyond this many MB");
    program.add_argument("--occirredmaxmb")
        .action([&](const auto& a) {conf.maxOccurIrredMB = std::atof(a.c_str());})
        .default_value(conf.maxOccurIrredMB)
        .help("Don't allow irredundant occur size to be beyond this many MB");
    ;

    /* po::options_description sub_str_time_limits("Occ-based subsumption and strengthening time limits"); */
    /* sub_str_time_limits.add_options() */
    program.add_argument("--strengthen")
        .action([&](const auto& a) {conf.do_strengthen_with_occur = std::atoi(a.c_str());})
        .default_value(conf.do_strengthen_with_occur)
        .help("Perform clause contraction through self-subsuming resolution as part of the occurrence-subsumption system");
    program.add_argument("--weakentimelim")
        .action([&](const auto& a) {conf.weaken_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.weaken_time_limitM)
        .help("Time-out in bogoprops M of weakeaning used");
    program.add_argument("--substimelim")
        .action([&](const auto& a) {conf.subsumption_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.subsumption_time_limitM)
        .help("Time-out in bogoprops M of subsumption of long clauses with long clauses, after computing occur");
    program.add_argument("--substimelimbinratio")
        .action([&](const auto& a) {conf.subsumption_time_limit_ratio_sub_str_w_bin = std::atof(a.c_str());})
        .default_value(conf.subsumption_time_limit_ratio_sub_str_w_bin)
        .help("Ratio of subsumption time limit to spend on sub&str long clauses with bin");
    program.add_argument("--substimelimlongratio")
        .action([&](const auto& a) {conf.subsumption_time_limit_ratio_sub_w_long = std::atof(a.c_str());})
        .default_value(conf.subsumption_time_limit_ratio_sub_w_long)
        .help("Ratio of subsumption time limit to spend on sub long clauses with long");
    program.add_argument("--strstimelim")
        .action([&](const auto& a) {conf.strengthening_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.strengthening_time_limitM)
        .help("Time-out in bogoprops M of strengthening of long clauses with long clauses, after computing occur");
    program.add_argument("--sublonggothrough")
        .action([&](const auto& a) {conf.subsume_gothrough_multip = std::atof(a.c_str());})
        .default_value(conf.subsume_gothrough_multip)
        .help("How many times go through subsume");
    ;

    /* po::options_description bva_options("BVA options"); */
    /* bva_options.add_options() */
    program.add_argument("--bva")
        .action([&](const auto& a) {conf.do_bva = std::atoi(a.c_str());})
        .default_value(conf.do_bva)
        .help("Perform bounded variable addition");
    program.add_argument("--bvaeveryn")
        .action([&](const auto& a) {conf.bva_every_n = std::atoi(a.c_str());})
        .default_value(conf.bva_every_n)
        .help("Perform BVA only every N occ-simplify calls");
    program.add_argument("--bvalim")
        .action([&](const auto& a) {conf.bva_limit_per_call = std::atoi(a.c_str());})
        .default_value(conf.bva_limit_per_call)
        .help("Maximum number of variables to add by BVA per call");
    program.add_argument("--bva2lit")
        .action([&](const auto& a) {conf.bva_also_twolit_diff = std::atoi(a.c_str());})
        .default_value(conf.bva_also_twolit_diff)
        .help("BVA with 2-lit difference hack, too. Beware, this reduces the effectiveness of 1-lit diff");
    program.add_argument("--bvato")
        .action([&](const auto& a) {conf.bva_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.bva_time_limitM)
        .help("BVA time limit in bogoprops M");
    ;

    /* po::options_description bve_options("BVE options"); */
    /* bve_options.add_options() */
    program.add_argument("--varelim")
        .action([&](const auto& a) {conf.doVarElim = std::atoi(a.c_str());})
        .default_value(conf.doVarElim)
        .help("Perform variable elimination as per Een and Biere");
    program.add_argument("--varelimto")
        .action([&](const auto& a) {conf.varelim_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.varelim_time_limitM)
        .help("Var elimination bogoprops M time limit");
    program.add_argument("--varelimover")
        .action([&](const auto& a) {conf.min_bva_gain = std::atoi(a.c_str());})
        .default_value(conf.min_bva_gain)
        .help("Do BVE until the resulting no. of clause increase is less than X. Only power of 2 makes sense, i.e. 2,4,8...");
    program.add_argument("--emptyelim")
        .action([&](const auto& a) {conf.do_empty_varelim = std::atoi(a.c_str());})
        .default_value(conf.do_empty_varelim)
        .help("Perform empty resolvent elimination using bit-map trick");
    program.add_argument("--varelimmaxmb")
        .action([&](const auto& a) {conf.var_linkin_limit_MB = std::atoi(a.c_str());})
        .default_value(conf.var_linkin_limit_MB)
        .help("Maximum extra MB of memory to use for new clauses during varelim");
    program.add_argument("--eratio")
        .action([&](const auto& a) {conf.varElimRatioPerIter = std::atof(a.c_str());})
        .default_value(conf.varElimRatioPerIter)
        //, ssERatio.str()
        .help("Eliminate this ratio of free variables at most per variable elimination iteration");
    program.add_argument("--varelimcheckres")
        .action([&](const auto& a) {conf.varelim_check_resolvent_subs = std::atoi(a.c_str());})
        .default_value(conf.varelim_check_resolvent_subs)
        .help("BVE should check whether resolvents subsume others and check for exact size increase");

    /* po::options_description xorOptions("XOR-related options"); */
    /* xorOptions.add_options() */
    program.add_argument("--xor")
        .action([&](const auto& a) {conf.doFindXors = std::atoi(a.c_str());})
        .default_value(conf.doFindXors)
        .help("Discover long XORs");
    program.add_argument("--maxxorsize")
        .action([&](const auto& a) {conf.maxXorToFind = std::atoi(a.c_str());})
        .default_value(conf.maxXorToFind)
        .help("Maximum XOR size to find");
    program.add_argument("--xorfindtout")
        .action([&](const auto& a) {conf.xor_finder_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.xor_finder_time_limitM)
        .help("Time limit for finding XORs");
    program.add_argument("--varsperxorcut")
        .action([&](const auto& a) {conf.xor_var_per_cut = std::atoi(a.c_str());})
        .default_value(conf.xor_var_per_cut)
        .help("Number of _real_ variables per XOR when cutting them. So 2 will have XORs of size 4 because 1 = connecting to previous, 1 = connecting to next, 2 in the midde. If the XOR is 4 long, it will be just one 4-long XOR, no connectors");
    program.add_argument("--maxxormat")
        .action([&](const auto& a) {conf.maxXORMatrix = std::atoll(a.c_str());})
        .default_value(conf.maxXORMatrix)
        .help("Maximum matrix size (=num elements) that we should try to echelonize");
    program.add_argument("--forcepreservexors")
        .action([&](const auto& a) {conf.force_preserve_xors = std::atoi(a.c_str());})
        .default_value(conf.force_preserve_xors)
        .help("Force preserving XORs when they have been found. Easier to make sure XORs are not lost through simplifiactions such as strenghtening");

    /* po::options_description gateOptions("Gate-related options"); */
    /* gateOptions.add_options() */
    program.add_argument("--gates")
        .action([&](const auto& a) {conf.doGateFind = std::atoi(a.c_str());})
        .default_value(conf.doGateFind)
        .help("Find gates.");
    program.add_argument("--printgatedot")
        .action([&](const auto& a) {conf.doPrintGateDot = std::atoi(a.c_str());})
        .default_value(conf.doPrintGateDot)
        .help("Print gate structure regularly to file 'gatesX.dot'");
    program.add_argument("--gatefindto")
        .action([&](const auto& a) {conf.gatefinder_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.gatefinder_time_limitM)
        .help("Max time in bogoprops M to find gates");

    /* po::options_description conflOptions("Conflict options"); */
    /* conflOptions.add_options() */
    program.add_argument("--recur")
        .action([&](const auto& a) {conf.doRecursiveMinim = std::atoi(a.c_str());})
        .default_value(conf.doRecursiveMinim)
        .help("Perform recursive minimisation");
    program.add_argument("--moreminim")
        .action([&](const auto& a) {conf.doMinimRedMore = std::atoi(a.c_str());})
        .default_value(conf.doMinimRedMore)
        .help("Perform strong minimisation at conflict gen.");
    program.add_argument("--moremoreminim")
        .action([&](const auto& a) {conf.doMinimRedMoreMore = std::atoi(a.c_str());})
        .default_value(conf.doMinimRedMoreMore)
        .help("Perform even stronger minimisation at conflict gen.");
    program.add_argument("--moremorealways")
        .action([&](const auto& a) {conf.doAlwaysFMinim = std::atoi(a.c_str());})
        .default_value(conf.doAlwaysFMinim)
        .help("Always strong-minimise clause");
    program.add_argument("--decbased")
        .action([&](const auto& a) {conf.do_decision_based_cl = std::atoi(a.c_str());})
        .default_value(conf.do_decision_based_cl)
        .help("Create decision-based conflict clauses when the UIP clause is too large");

    /* po::options_description propOptions("Glue options"); */
    /* propOptions.add_options() */
    program.add_argument("--updateglueonanalysis")
        .action([&](const auto& a) {conf.update_glues_on_analyze = std::atoi(a.c_str());})
        .default_value(conf.update_glues_on_analyze)
        .help("Update glues while analyzing");
    program.add_argument("--maxgluehistltlimited")
        .action([&](const auto& a) {conf.max_glue_cutoff_gluehistltlimited = std::atoi(a.c_str());})
        .default_value(conf.max_glue_cutoff_gluehistltlimited)
        .help("Maximum glue used by glue-based restart strategy when populating glue history.");

    /* po::options_description chrono_bt_opts("Propagation options"); */
    /* chrono_bt_opts.add_options() */
    program.add_argument("--diffdeclevelchrono")
        .action([&](const auto& a) {conf.diff_declev_for_chrono = std::atoi(a.c_str());})
        .default_value(conf.diff_declev_for_chrono)
        .help("Difference in decision level is more than this, perform chonological backtracking instead of non-chronological backtracking. Giving -1 means it is never turned on (overrides '--confltochrono -1' in this case).");

#ifdef USE_SQLITE3
    /* po::options_description sqlOptions("SQL options"); */
    /* sqlOptions.add_options() */
    program.add_argument("--sql")
        .action([&](const auto& a) {sql = std::atoi(a.c_str());})
        .default_value(0)
        .help("Write to SQL. 0 = no SQL, 1 or 2 = sqlite");
    program.add_argument("--sqlitedb")
        .action([&](const auto& a) {sqlite_filename = a;})
        .default_value(conf.verbosity)
        .help("[0-10] Verbosity of solver. 0 = only solution");;
    program.add_argument("--sqlitedboverwrite")
        .action([&](const auto& a) {conf.sql_overwrite_file = std::atoi(a.c_str());})
        .default_value(conf.sql_overwrite_file)
        .help("Overwrite the SQLite database file if it exists");
    program.add_argument("--cldatadumpratio")
        .action([&](const auto& a) {conf.dump_individual_cldata_ratio = std::atof(a.c_str());})
        .default_value(conf.dump_individual_cldata_ratio)
        .help("Only dump this ratio of clauses' data, randomly selected. Since machine learning doesn't need that much data, this can reduce the data you have to deal with.");
    program.add_argument("--cllockdatagen")
        .action([&](const auto& a) {conf.lock_for_data_gen_ratio = std::atof(a.c_str());})
        .default_value(conf.lock_for_data_gen_ratio)
        .help("Lock for data generation into lev0, setting locked_for_data_gen. Only works when clause is marked for dumping ('--cldatadumpratio' )");
#endif

    /* po::options_description printOptions("Printing options"); */
    /* printOptions.add_options() */
    program.add_argument("--verbstat")
        .action([&](const auto& a) {conf.verbStats = std::atoi(a.c_str());})
        .default_value(conf.verbStats)
        .help("Change verbosity of statistics at the end of the solving [0..3]");
    program.add_argument("--verbrestart")
        .action([&](const auto& a) {conf.print_full_restart_stat = std::atoi(a.c_str());})
        .default_value(conf.print_full_restart_stat)
        .help("Print more thorough, but different stats");
    program.add_argument("--verballrestarts")
        .action([&](const auto& a) {conf.print_all_restarts = std::atoi(a.c_str());})
        .default_value(conf.print_all_restarts)
        .help("Print a line for every restart");
    program.add_argument("--printsol,s")
        .action([&](const auto& a) {printResult = std::atoi(a.c_str());})
        .default_value(printResult)
        .help("Print assignment if solution is SAT");
    program.add_argument("--restartprint")
        .action([&](const auto& a) {conf.print_restart_line_every_n_confl = std::atoi(a.c_str());})
        .default_value(conf.print_restart_line_every_n_confl)
        .help("Print restart status lines at least every N conflicts");


    /* po::options_description distillOptions("Distill options"); */
    /* distillOptions.add_options() */
    program.add_argument("--distill")
        .action([&](const auto& a) {conf.do_distill_clauses = std::atoi(a.c_str());})
        .default_value(conf.do_distill_clauses)
        .help("Regularly execute clause distillation");
    program.add_argument("--distillbin")
        .action([&](const auto& a) {conf.do_distill_bin_clauses = std::atoi(a.c_str());})
        .default_value(conf.do_distill_bin_clauses)
        .help("Regularly execute clause distillation");
    program.add_argument("--distillmaxm")
        .action([&](const auto& a) {conf.distill_long_cls_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.distill_long_cls_time_limitM)
        .help("Maximum number of Mega-bogoprops(~time) to spend on vivifying/distilling long cls by enqueueing and propagating");
    program.add_argument("--distillincconf")
        .action([&](const auto& a) {conf.distill_increase_conf_ratio = std::atof(a.c_str());})
        .default_value(conf.distill_increase_conf_ratio)
        .help("Multiplier for current number of conflicts OTF distill");
    program.add_argument("--distillminconf")
        .action([&](const auto& a) {conf.distill_min_confl = std::atoll(a.c_str());})
        .default_value(conf.distill_min_confl)
        .help("Minimum number of conflicts between OTF distill");
    program.add_argument("--distilltier0ratio")
        .action([&](const auto& a) {conf.distill_red_tier0_ratio = std::atof(a.c_str());})
        .default_value(conf.distill_red_tier0_ratio)
        .help("How much of tier 0 to distill");
    program.add_argument("--distilltier1ratio")
        .action([&](const auto& a) {conf.distill_red_tier1_ratio = std::atof(a.c_str());})
        .default_value(conf.distill_red_tier1_ratio)
        .help("How much of tier 1 to distill");
    program.add_argument("--distillirredalsoremratio")
        .action([&](const auto& a) {conf.distill_irred_alsoremove_ratio = std::atof(a.c_str());})
        .default_value(conf.distill_irred_alsoremove_ratio)
        .help("How much of irred to distill when doing also removal");
    program.add_argument("--distillirrednoremratio")
        .action([&](const auto& a) {conf.distill_irred_noremove_ratio = std::atof(a.c_str());})
        .default_value(conf.distill_irred_noremove_ratio)
        .help("How much of irred to distill when doing no removal");
    program.add_argument("--distillshuffleeveryn")
        .action([&](const auto& a) {conf.distill_rand_shuffle_order_every_n = std::atoi(a.c_str());})
        .default_value(conf.distill_rand_shuffle_order_every_n)
        .help("Shuffle to-be-distilled clauses every N cases randomly");
    program.add_argument("--distillsort")
        .action([&](const auto& a) {conf.distill_sort = std::atoi(a.c_str());})
        .default_value(conf.distill_sort)
        .help("Distill sorting type");
    ;

    /* po::options_description mem_save_opts("Memory saving options"); */
    /* mem_save_opts.add_options() */
    program.add_argument("--renumber")
        .action([&](const auto& a) {conf.doRenumberVars = std::atoi(a.c_str());})
        .default_value(conf.doRenumberVars)
        .help("Renumber variables to increase CPU cache efficiency");
    program.add_argument("--mustconsolidate")
        .action([&](const auto& a) {conf.must_always_conslidate = std::atoi(a.c_str());})
        .default_value(conf.must_always_conslidate)
        .help("Always consolidate, even if not useful. This is used for debugging ONLY");
    program.add_argument("--savemem")
        .action([&](const auto& a) {conf.doSaveMem = std::atoi(a.c_str());})
        .default_value(conf.doSaveMem)
        .help("Save memory by deallocating variable space after renumbering. Only works if renumbering is active.");
    program.add_argument("--mustrenumber")
        .action([&](const auto& a) {conf.must_renumber = std::atoi(a.c_str());})
        .default_value(conf.must_renumber)
        .help("Treat all 'renumber' strategies as 'must-renumber'");
    program.add_argument("--fullwatchconseveryn")
        .action([&](const auto& a) {conf.full_watch_consolidate_every_n_confl = std::atoll(a.c_str());})
        .default_value(conf.full_watch_consolidate_every_n_confl)
        .help("Consolidate watchlists fully once every N conflicts. Scheduled during simplification rounds.");

    /* po::options_description miscOptions("Misc options"); */
    /* miscOptions.add_options() */
    program.add_argument("--strmaxt")
        .action([&](const auto& a) {conf.watch_based_str_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.watch_based_str_time_limitM)
        .help("Maximum MBP to spend on distilling long irred cls through watches");
    program.add_argument("--implicitmanip")
        .action([&](const auto& a) {conf.doStrSubImplicit = std::atoi(a.c_str());})
        .default_value(conf.doStrSubImplicit)
        .help("Subsume and strengthen implicit clauses with each other");
    program.add_argument("--implsubsto")
        .action([&](const auto& a) {conf.subsume_implicit_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.subsume_implicit_time_limitM)
        .help("Timeout (in bogoprop Millions) of implicit subsumption");
    program.add_argument("--implstrto")
        .action([&](const auto& a) {conf.distill_implicit_with_implicit_time_limitM = std::atoll(a.c_str());})
        .default_value(conf.distill_implicit_with_implicit_time_limitM)
        .help("Timeout (in bogoprop Millions) of implicit strengthening");
    program.add_argument("--cardfind")
        .action([&](const auto& a) {conf.doFindCard = std::atoi(a.c_str());})
        .default_value(conf.doFindCard)
        .help("Find cardinality constraints");

    /* hiddenOptions.add_options() */
    program.add_argument("--sync")
        .action([&](const auto& a) {conf.sync_every_confl = std::atoll(a.c_str());})
        .default_value(conf.sync_every_confl)
        .help("Sync threads every N conflicts");
    program.add_argument("--clearinter")
        .action([&](const auto& a) {need_clean_exit = std::atoi(a.c_str());})
        .default_value(0)
        .help("Interrupt threads cleanly, all the time");
    program.add_argument("--zero-exit-status")
        .flag()
        .action([&](const auto&) {zero_exit_status = true;})
        .help("Exit with status zero in case the solving has finished without an issue");
    program.add_argument("--printtimes")
        .action([&](const auto& a) {conf.do_print_times = std::atoi(a.c_str());})
        .default_value(conf.do_print_times)
        .help("Print time it took for each simplification run. If set to 0, logs are easier to compare");
    program.add_argument("--maxsccdepth")
        .action([&](const auto& a) {conf.max_scc_depth = std::atoi(a.c_str());})
        .default_value(conf.max_scc_depth)
        .help("The maximum for scc search depth");
    program.add_argument("--simfrat")
        .action([&](const auto& a) {conf.simulate_frat = std::atoi(a.c_str());})
        .default_value(conf.simulate_frat)
        .help("Simulate FRAT");
    program.add_argument("--sampling")
        .action([&](const auto& a) {sampling_vars_str = a;})
        .default_value(sampling_vars_str)
        .help("Sampling vars, separated by comma");
    program.add_argument("--onlysampling")
        .flag()
        .action([&](const auto&) {only_sampling_solution = true;})
        .help("Print and ban(!) solutions' vars only in 'c ind' or as --sampling '...'");
    program.add_argument("--assump")
        .action([&](const auto& a) {assump_filename = a;})
        .default_value(assump_filename)
        .help("Assumptions file");

#ifdef USE_BOSPHORUS
    po::options_description bosph_options("Gauss options");
    bosph_options.add_options()
     program.add_argument("--bosph")
        .action([&](const auto& a) {conf.do_bosphorus = std::atoi(a.c_str());})
        .default_value(conf.do_bosphorus)
        .help("Execute bosphorus");
     program.add_argument("--bospheveryn")
        .action([&](const auto& a) {conf.bosphorus_every_n = std::atoi(a.c_str());})
        .default_value(conf.bosphorus_every_n)
        .help("Execute bosphorus only every Nth iteration -- starting at Nth iter.");
     ;
#endif

/*     po::options_description gaussOptions("Gauss options"); */
/*     gaussOptions.add_options() */
     program.add_argument("--maxmatrixrows")
        .action([&](const auto& a) {conf.gaussconf.max_matrix_rows = std::atoi(a.c_str());})
        .default_value(conf.gaussconf.max_matrix_rows)
        .help("Set maximum no. of rows for gaussian matrix. Too large matrices"
            "should bee discarded for reasons of efficiency");
     program.add_argument("--maxmatrixcols")
        .action([&](const auto& a) {conf.gaussconf.max_matrix_columns = std::atoi(a.c_str());})
        .default_value(conf.gaussconf.max_matrix_columns)
        .help("Set maximum no. of columns for gaussian matrix. Too large matrices"
            "should bee discarded for reasons of efficiency");
    program.add_argument("--autodisablegauss")
        .action([&](const auto& a) {conf.gaussconf.autodisable = std::atoi(a.c_str());})
        .default_value(conf.gaussconf.autodisable)
        .help("Automatically disable gauss when performing badly");
    program.add_argument("--minmatrixrows")
        .action([&](const auto& a) {conf.gaussconf.min_matrix_rows = std::atoi(a.c_str());})
        .default_value(conf.gaussconf.min_matrix_rows)
        .help("Set minimum no. of rows for gaussian matrix. Normally, too small"
            " matrices are discarded for reasons of efficiency");
    program.add_argument("--maxnummatrices")
        .action([&](const auto& a) {conf.gaussconf.max_num_matrices = std::atoi(a.c_str());})
        .default_value(conf.gaussconf.max_num_matrices)
        .help("Maximum number of matrices to treat.");
    program.add_argument("--detachxor")
        .action([&](const auto& a) {conf.xor_detach_reattach = std::atoi(a.c_str());})
        .default_value(conf.xor_detach_reattach)
        .help("Detach and reattach XORs");
    program.add_argument("--useallmatrixes")
        .action([&](const auto& a) {conf.force_use_all_matrixes = std::atoi(a.c_str());})
        .default_value(conf.force_use_all_matrixes)
        .help("Force using all matrices");
    program.add_argument("--detachverb")
        .action([&](const auto& a) {conf.xor_detach_verb = std::atoi(a.c_str());})
        .default_value(conf.xor_detach_verb)
        .help("If set, verbosity for XOR detach code is upped, ignoring normal verbosity");
    program.add_argument("--gaussusefulcutoff")
        .action([&](const auto& a) {conf.gaussconf.min_usefulness_cutoff = std::atof(a.c_str());})
        .default_value(conf.gaussconf.min_usefulness_cutoff)
        .help("Turn off Gauss if less than this many usefulenss ratio is recorded");
    program.add_argument("--dumpresult")
        .action([&](const auto& a) {result_fname = a;})
        .help("Write solution(s) to this file");

    //these a kind of special and determine positional options' meanings
    program.add_argument("files").remaining().help("input file and FRAT output");

}
/* clang-format on */

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
        program.parse_args(argc, argv);
        if (program.is_used("--help")) {
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
            << "cryptominisat5 [options] inputfile [frat-file]" << endl << endl;

            cout << program << endl;
            cout << "Normal run schedules:" << endl;
            cout << "  Default schedule: "
            << remove_last_comma_if_exists(conf.simplify_schedule_nonstartup) << endl<< endl;
            cout << "  Schedule at startup: "
            << remove_last_comma_if_exists(conf.simplify_schedule_startup) << endl << endl;
            std::exit(0);
        }

        /* if (vm.count("help")) */
        /* { */
        /*     cout */
        /*     << "USAGE: " << argv[0] << " [options] inputfile [frat-trim-file]" << endl */

        /*     << " where input is " */
        /*     #ifndef USE_ZLIB */
        /*     << "plain" */
        /*     #else */
        /*     << "plain or gzipped" */
        /*     #endif */
        /*     << " DIMACS." << endl; */

        /*     cout << help_options_simple << endl; */
        /*     std::exit(0); */
        /* } */
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        exit(-1);
    }

    /* } catch (boost::exception_detail::clone_impl< */
    /*     boost::exception_detail::error_info_injector<po::unknown_option> >& c */
    /* ) { */
    /*     cerr */
    /*     << "ERROR: Some option you gave was wrong. Please give '--help' to get help" << endl */
    /*     << "       Unknown option: " << c.what() << endl; */
    /*     std::exit(-1); */
    /* } catch (boost::bad_any_cast &e) { */
    /*     std::cerr */
    /*     << "ERROR! You probably gave a wrong argument type" << endl */
    /*     << "       Bad cast: " << e.what() */
    /*     << endl; */

    /*     std::exit(-1); */
    /* } catch (boost::exception_detail::clone_impl< */
    /*     boost::exception_detail::error_info_injector<po::invalid_option_value> >& what */
    /* ) { */
    /*     cerr */
    /*     << "ERROR: Invalid value '" << what.what() << "'" << endl */
    /*     << "       given to option '" << what.get_option_name() << "'" */
    /*     << endl; */

    /*     std::exit(-1); */
    /* } catch (boost::exception_detail::clone_impl< */
    /*     boost::exception_detail::error_info_injector<po::multiple_occurrences> >& what */
    /* ) { */
    /*     cerr */
    /*     << "ERROR: " << what.what() << " of option '" */
    /*     << what.get_option_name() << "'" */
    /*     << endl; */

    /*     std::exit(-1); */
    /* } catch (boost::exception_detail::clone_impl< */
    /*     boost::exception_detail::error_info_injector<po::required_option> >& what */
    /* ) { */
    /*     cerr */
    /*     << "ERROR: You forgot to give a required option '" */
    /*     << what.get_option_name() << "'" */
    /*     << endl; */

    /*     std::exit(-1); */
    /* } catch (boost::exception_detail::clone_impl< */
    /*     boost::exception_detail::error_info_injector<po::too_many_positional_options_error> >& what */
    /* ) { */
    /*     cerr */
    /*     << "ERROR: You gave too many positional arguments. Only at most two can be given:" << endl */
    /*     << "       the 1st the CNF file input, and optionally, the 2nd the FRAT file output" << endl */
    /*     << "    OR (pre-processing)  1st for the input CNF, 2nd for the simplified CNF" << endl */
    /*     << "    OR (post-processing) 1st for the solution file" << endl */
    /*     ; */

    /*     std::exit(-1); */
    /* } catch (boost::exception_detail::clone_impl< */
    /*     boost::exception_detail::error_info_injector<po::ambiguous_option> >& what */
    /* ) { */
    /*     cerr */
    /*     << "ERROR: The option you gave was not fully written and matches" << endl */
    /*     << "       more than one option. Please give the full option name." << endl */
    /*     << "       The option you gave: '" << what.get_option_name() << "'" <<endl */
    /*     << "       The alternatives are: "; */
    /*     for(size_t i = 0; i < what.alternatives().size(); i++) { */
    /*         cout << what.alternatives()[i]; */
    /*         if (i+1 < what.alternatives().size()) { */
    /*             cout << ", "; */
    /*         } */
    /*     } */
    /*     cout << endl; */

    /*     std::exit(-1); */
    /* } catch (boost::exception_detail::clone_impl< */
    /*     boost::exception_detail::error_info_injector<po::invalid_command_line_syntax> >& what */
    /* ) { */
    /*     cerr */
    /*     << "ERROR: The option you gave is missing the argument or the" << endl */
    /*     << "       argument is given with space between the equal sign." << endl */
    /*     << "       detailed error message: " << what.what() << endl */
    /*     ; */
    /*     std::exit(-1); */
    /* } */
}

void Main::parse_restart_type()
{
    string type = program.get<string>("restart");
    if (type == "geom")
        conf.restartType = Restart::geom;
    else if (type == "luby")
        conf.restartType = Restart::luby;
    else if (type == "glue")
        conf.restartType = Restart::glue;
    else if (type == "auto")
        conf.restartType = Restart::automatic;
    else throw WrongParam("restart", "unknown restart type");
}

void Main::parse_polarity_type()
{
    string mode = program.get<string>("polar");
    if (mode == "true") conf.polarity_mode = PolarityMode::polarmode_pos;
    else if (mode == "false") conf.polarity_mode = PolarityMode::polarmode_neg;
    else if (mode == "rnd") conf.polarity_mode = PolarityMode::polarmode_rnd;
    else if (mode == "auto") conf.polarity_mode = PolarityMode::polarmode_automatic;
    else if (mode == "stable") conf.polarity_mode = PolarityMode::polarmode_best;
    else if (mode == "weight") conf.polarity_mode = PolarityMode::polarmode_weighted;
    else throw WrongParam(mode, "unknown polarity-mode");
}

void Main::manually_parse_some_options()
{
    #ifndef USE_BREAKID
    if (conf.doBreakid) {
        if (conf.verbosity) {
            cout << "c BreakID not compiled in, disabling" << endl;
        }
        conf.doBreakid = false;
    }
    #endif

    if (conf.max_glue_cutoff_gluehistltlimited > 1000) {
        cout << "ERROR: 'Maximum supported glue size is currently 100000" << endl;
        exit(-1);
    }

    if (conf.which_sls != "yalsat" &&
        conf.which_sls != "walksat" &&
        conf.which_sls != "ccnr_yalsat" &&
        conf.which_sls != "ccnr")
    {
        cout << "ERROR: you gave '" << conf.which_sls << " for SLS with the option '--slstype'."
        << " This is incorrect, we only accept 'yalsat' and 'walksat'"
        << endl;
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

    if (!result_fname.empty()) {
        resultfile = new std::ofstream;
        resultfile->open(result_fname.c_str());
        if (!(*resultfile)) {
            cout << "ERROR: Couldn't open file '" << result_fname << "' for writing result!" << endl;
            std::exit(-1);
        }
    }

    parse_polarity_type();
    parse_restart_type();

    try {
        auto files = program.get<std::vector<std::string>>("files");
        if (files.size() > 2) {
            cerr << "ERROR: you can only have at most two files as positional options:"
                "the input file and the output FRAT file" << endl;
            exit(-1);
        }
        if (!files.empty()) {
            input_file = files[0];
#ifdef USE_SQLITE3
            if (!program.is_used("sqlitedb")) sqlite_filename = input_file + ".sqlite";
            else sqlite_filename = program.get<string>("sqlitedb");
#endif
            fileNamePresent = true;
        } else assert(false && "The try() should not have succeeded");
        if (files.size() > 1 || conf.simulate_frat) {
            if (files.size() > 1) {
                assert(!conf.simulate_frat && "You can't have both simulation of FRAT and frat");
                frat_fname = files[1];
            }
            handle_frat_option();
        }
    } catch (std::logic_error& e) {
        fileNamePresent = false;
    }
    if (conf.verbosity >= 3) cout << "c Outputting solution to console" << endl;
}

void Main::parseCommandLine() {
    need_clean_exit = 0;

    //Reconstruct the command line so we can emit it later if needed
    for(int i = 0; i < argc; i++) {
        commandLine += string(argv[i]);
        if (i+1 < argc) {
            commandLine += " ";
        }
    }

    add_supported_options();
    /* all_options.add(help_options_complicated); */
    /* all_options.add(hiddenOptions); */

    /* help_options_simple */
    /* .add(generalOptions) */
    /* ; */

    check_options_correctness();
    if (program["version"] == true) {
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

int Main::solve()
{
    wallclock_time_started = real_time_sec();
    solver = new SATSolver((void*)&conf);
    solverToInterrupt = solver;
    if (fratf) solver->set_frat(fratf);
    if (program.is_used("maxtime")) solver->set_max_time(program.get<double>("maxtime"));
    if (program.is_used("maxconfl")) solver->set_max_confl(program.get<uint64_t>("maxconfl"));

    check_num_threads_sanity(num_threads);
    solver->set_num_threads(num_threads);
    if (sql != 0) solver->set_sqlite(sqlite_filename);

    //Print command line used to execute the solver: for options and inputs
    if (conf.verbosity) {
        printVersionInfo();
        cout
        << "c Executed with command line: "
        << commandLine
        << endl;
    }

    solver->add_sql_tag("commandline", commandLine);
    solver->add_sql_tag("verbosity", std::to_string(conf.verbosity));
    solver->add_sql_tag("threads", std::to_string(num_threads));
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
    parseInAllFiles(solver);
    if (!assump_filename.empty()) {
        std::ifstream* tmp = new std::ifstream;
        tmp->open(assump_filename.c_str());
        std::string temp;
        while(std::getline(*tmp, temp)) {
            //Do with temp
            int x = std::stoi(temp);
            cout << "Assume: " << x << endl;
            Lit l = Lit(std::abs(x)-1, x < 0);
            assumps.push_back(l);
        }

        delete tmp;
    }

    lbool ret = multi_solutions();
    if (ret == l_Undef && conf.verbosity) {
        cout
        << "c Not finished running -- signal caught or some maximum reached"
        << endl;
    }
    if (conf.verbosity) {
        solver->print_stats(wallclock_time_started);
    }

    printResultFunc(&cout, false, ret);
    if (resultfile) {
        printResultFunc(resultfile, true, ret);
    }
    if (ret == l_True && max_nr_of_solutions > 1) {
       // If ret is l_True then we must have hit the solution limit.
       // Print final number of solutions when we hit the limit here
       // as multi_solutions() doesn't. Don't print for a single solution.
       if (conf.verbosity) {
           cout
           << "c Number of solutions found until now: "
           << std::setw(6) << max_nr_of_solutions
           << endl
           << "c maxsol reached"
           << endl;
       }
    }

    return correctReturnValue(ret);
}

lbool Main::multi_solutions()
{
    if (max_nr_of_solutions == 1
        && fratf == NULL
        && !conf.simulate_frat
        && debugLib.empty()
    ) {
        solver->set_single_run();
    }

    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    while(current_nr_of_solutions < max_nr_of_solutions && ret == l_True) {
        ret = solver->solve(&assumps, only_sampling_solution);
        current_nr_of_solutions++;

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

            if (!dont_ban_solutions) {
                ban_found_solution();
            }
        }
    }
    return ret;
}

void Main::ban_found_solution()
{
    vector<Lit> lits;
    if (sampling_vars.empty()) {
        //all of the solution
        for (uint32_t var = 0; var < solver->nVars(); var++) {
            if (solver->get_model()[var] != l_Undef) {
                lits.push_back( Lit(var, (solver->get_model()[var] == l_True)? true : false) );
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
