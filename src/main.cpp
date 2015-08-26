/*
Copyright (c) 2014 Mate Soos

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
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>

#include "main.h"
#include "main_common.h"
#include "time_mem.h"
#include "dimacsparser.h"
#include "cryptominisat4/cryptominisat.h"


#include <boost/lexical_cast.hpp>
using namespace CMSat;
using boost::lexical_cast;

using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;

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

bool fileExists(const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}


Main::Main(int _argc, char** _argv) :
    fileNamePresent (false)
    , argc(_argc)
    , argv(_argv)
{
}

SATSolver* solverToInterrupt;
int clear_interrupt;
string redDumpFname;
string irredDumpFname;

void SIGINT_handler(int)
{
    SATSolver* solver = solverToInterrupt;
    cout << "c " << endl;
    std::cerr << "*** INTERRUPTED ***" << endl;
    if (!redDumpFname.empty() || !irredDumpFname.empty() || clear_interrupt) {
        solver->interrupt_asap();
        std::cerr
        << "*** Please wait. We need to interrupt cleanly" << endl
        << "*** This means we might need to finish some calculations"
        << endl;
    } else {
        if (solver->nVars() > 0) {
            //if (conf.verbosity >= 1) {
                solver->add_in_partial_solving_stats();
                solver->print_stats();
            //}
        } else {
            cout
            << "No clauses or variables were put into the solver, exiting without stats"
            << endl;
        }
        _exit(1);
    }
}

void Main::readInAFile(const string& filename)
{
    solver->add_sql_tag("filename", filename);
    if (conf.verbosity >= 1) {
        cout << "c Reading file '" << filename << "'" << endl;
    }
    #ifndef USE_ZLIB
        FILE * in = fopen(filename.c_str(), "rb");
    #else
        gzFile in = gzopen(filename.c_str(), "rb");
    #endif

    if (in == NULL) {
        std::cerr
        << "ERROR! Could not open file '"
        << filename
        << "' for reading" << endl;

        std::exit(1);
    }

    DimacsParser parser(solver, debugLib, conf.verbosity);
    parser.parse_DIMACS(in);

    #ifndef USE_ZLIB
        fclose(in);
    #else
        gzclose(in);
    #endif
}

void Main::readInStandardInput()
{
    if (conf.verbosity) {
        cout
        << "c Reading from standard input... Use '-h' or '--help' for help."
        << endl;
    }

    #ifndef USE_ZLIB
        FILE * in = stdin;
    #else
        gzFile in = gzdopen(fileno(stdin), "rb");
    #endif

    if (in == NULL) {
        std::cerr << "ERROR! Could not open standard input for reading" << endl;
        std::exit(1);
    }

    DimacsParser parser(solver, debugLib, conf.verbosity);
    parser.parse_DIMACS(in);

    #ifdef USE_ZLIB
        gzclose(in);
    #endif
}

void Main::parseInAllFiles()
{
    const double myTime = cpuTime();

    //First read normal extra files
    if (debugLib && filesToRead.size() > 1) {
        cout
        << "debugNewVar and debugLib must both be OFF"
        << "to parse in more than one file"
        << endl;

        std::exit(-1);
    }

    for (vector<string>::const_iterator
        it = filesToRead.begin(), end = filesToRead.end(); it != end; ++it
    ) {
        readInAFile(it->c_str());
    }

    if (!fileNamePresent)
        readInStandardInput();

    if (conf.verbosity >= 1) {
        cout
        << "c Parsing time: "
        << std::fixed << std::setprecision(2)
        << (cpuTime() - myTime)
        << " s" << endl;
    }
}

void Main::printResultFunc(
    std::ostream* os
    , const bool toFile
    , const lbool ret
    , const bool firstSolution
) {
    if (ret == l_True) {
        if(toFile) {
            if(firstSolution)  *os << "SAT" << endl;
        }
        else if (!printResult) *os << "s SATISFIABLE" << endl;
        else                   *os << "s SATISFIABLE" << endl;
     } else if (ret == l_False) {
        if(toFile) {
            if(firstSolution)  *os << "UNSAT" << endl;
        }
        else if (!printResult) *os << "s UNSATISFIABLE" << endl;
        else                   *os << "s UNSATISFIABLE" << endl;
    } else {
        *os << "s INDETERMINATE" << endl;
    }

    if (ret == l_True && (printResult || toFile)) {
        if (toFile) {
            for (uint32_t var = 0; var < solver->nVars(); var++) {
                if (solver->get_model()[var] != l_Undef) {
                    *os << ((solver->get_model()[var] == l_True)? "" : "-") << var+1 << " ";
                }
            }
            *os << "0" << endl;
        } else {
            print_model(os, solver);
        }
    }
}

void Main::add_supported_options()
{
    // Declare the supported options.
    po::options_description generalOptions("Most important options");
    generalOptions.add_options()
    ("help,h", "Prints this help")
    ("version,v", "Print version number")
    ("input", po::value< vector<string> >(), "file(s) to read")
    ("random,r", po::value(&conf.origSeed)->default_value(conf.origSeed)
        , "[0..] Sets random seed")
    ("threads,t", po::value(&num_threads)->default_value(1)
        ,"Number of threads")
    ("sync", po::value(&conf.sync_every_confl)->default_value(conf.sync_every_confl)
        , "Sync threads every N conflicts")
    ("maxtime", po::value(&conf.maxTime)->default_value(conf.maxTime, "MAX")
        , "Stop solving after this much time, print stats and exit")
    ("maxconfl", po::value(&conf.maxConfl)->default_value(conf.maxConfl, "MAX")
        , "Stop solving after this many conflicts, print stats and exit")
    ("occsimp", po::value(&conf.perform_occur_based_simp)->default_value(conf.perform_occur_based_simp)
        , "Perform occurrence-list-based optimisations (variable elimination, subsumption, bounded variable addition...)")
    ("drup,d", po::value(&drupfilname)
        , "Put DRUP verification information into this file")
    ("drupexistscheck", po::value(&drupExistsCheck)->default_value(drupExistsCheck)
        , "Check if the drup file provided already exists")
    ("satcomp", po::bool_switch(&satcomp)
        , "Run with sat competition-tuned defaults")
    ("drupdebug", po::bool_switch(&drupDebug)
        , "Output DRUP verification into the console. Helpful to see where DRUP fails -- use in conjunction with --verb 20")
    ("reconf", po::value(&conf.reconfigure_val)->default_value(conf.reconfigure_val)
        , "Reconfigure after some time to this solver conf")
    ("reconfat", po::value(&conf.reconfigure_at)->default_value(conf.reconfigure_at)
        , "Reconfigure after this many simplifications"
    )
    //("greedyunbound", po::bool_switch(&conf.greedyUnbound)
    //    , "Greedily unbound variables that are not needed for SAT")
    ;

    std::ostringstream s_blocking_multip;
    s_blocking_multip << std::setprecision(4) << conf.blocking_restart_multip;

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
        , "Lower bound on blocking restart -- don't block before this many concflicts")
    ;

    std::ostringstream s_incclean;

    std::ostringstream s_clean_confl_multiplier;
    s_clean_confl_multiplier << std::setprecision(2) << conf.clean_confl_multiplier;

    po::options_description reduceDBOptions("Red clause removal options");
    reduceDBOptions.add_options()
    ("cleanconflmult", po::value(&conf.clean_confl_multiplier)->default_value(conf.clean_confl_multiplier, s_clean_confl_multiplier.str())
        , "If prop&confl are used to clean, by what value should we multiply the conflicts relative to propagations (conflicts are much more rare, but maybe more useful)")
    ("clearstat", po::value(&conf.doClearStatEveryClauseCleaning)->default_value(conf.doClearStatEveryClauseCleaning)
        , "Clear clause statistics data of each clause after clause cleaning")
    ("incclean", po::value(&conf.inc_max_temp_red_cls)->default_value(conf.inc_max_temp_red_cls, s_incclean.str())
        , "Clean increment cleaning by this factor for next cleaning")
    ("maxredratio", po::value(&conf.maxNumRedsRatio)->default_value(conf.maxNumRedsRatio)
        , "Don't ever have more than maxNumRedsRatio*(irred_clauses) redundant clauses")
    ("maxtemp", po::value(&conf.max_temporary_learnt_clauses)->default_value(conf.max_temporary_learnt_clauses)
        , "Maximum number of temporary clauses of high glue")
    ;

    std::ostringstream s_random_var_freq;
    s_random_var_freq << std::setprecision(5) << conf.random_var_freq;

    std::ostringstream s_var_decay_start;
    s_var_decay_start << std::setprecision(5) << conf.var_decay_start;

    std::ostringstream s_var_decay_max;
    s_var_decay_max << std::setprecision(5) << conf.var_decay_max;

    po::options_description varPickOptions("Variable branching options");
    varPickOptions.add_options()
    ("vardecaystart", po::value(&conf.var_decay_start)->default_value(conf.var_decay_start, s_var_decay_start.str())
        , "variable activity increase divider (MUST be smaller than multiplier)")
    ("vardecaymax", po::value(&conf.var_decay_max)->default_value(conf.var_decay_max, s_var_decay_max.str())
        , "variable activity increase divider (MUST be smaller than multiplier)")
    ("vincstart", po::value(&conf.var_inc_start)->default_value(conf.var_inc_start)
        , "variable activity increase stars with this value. Make sure that this multiplied by multiplier and dividied by divider is larger than itself")
    ("freq", po::value(&conf.random_var_freq)->default_value(conf.random_var_freq, s_random_var_freq.str())
        , "[0 - 1] freq. of picking var at random")
    ("dompickf", po::value(&conf.dominPickFreq)->default_value(conf.dominPickFreq)
        , "Use dominating literal every once in N when picking decision literal")
    ("morebump", po::value(&conf.extra_bump_var_activities_based_on_glue)->default_value(conf.extra_bump_var_activities_based_on_glue)
        , "Bump variables' activities based on the glue of red clauses there are in during UIP generation (as per Glucose)")
    ;

    po::options_description polar_options("Variable polarity options");
    polar_options.add_options()
    ("polar", po::value<string>()->default_value("auto")
        , "{true,false,rnd,auto} Selects polarity mode. 'true' -> selects only positive polarity when branching. 'false' -> selects only negative polarity when brancing. 'auto' -> selects last polarity used (also called 'caching')")
    ("calcpolar1st", po::value(&conf.do_calc_polarity_first_time)->default_value(conf.do_calc_polarity_first_time)
        , "Calculate the polarity of variables based on their occurrences at startup of solve()")
    ("calcpolarall", po::value(&conf.do_calc_polarity_every_time)->default_value(conf.do_calc_polarity_every_time)
        , "Calculate the polarity of variables based on their occurrences at startup & after every simplification")
    ;


    po::options_description iterativeOptions("Iterative solve options");
    iterativeOptions.add_options()
    ("maxsol", po::value(&max_nr_of_solutions)->default_value(max_nr_of_solutions)
        , "Search for given amount of solutions")
    ("dumpred", po::value(&redDumpFname)
        , "If stopped dump redundant clauses here")
    ("maxdump", po::value(&conf.maxDumpRedsSize)
        , "Maximum length of redundant clause dumped")
    ("dumpirred", po::value(&irredDumpFname)
        , "If stopped, dump irred original problem here")
    ("debuglib", po::bool_switch(&debugLib)
        , "MainSolver at specific 'solve()' points in CNF file")
    ("dumpresult", po::value(&resultFilename)
        , "Write result(s) to this file")
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

    po::options_description simplificationOptions("Simplification options");
    simplificationOptions.add_options()
    ("schedsimp", po::value(&conf.do_simplify_problem)->default_value(conf.do_simplify_problem)
        , "Perform simplification rounds. If 0, we never perform any.")
    ("presimp", po::value(&conf.simplify_at_startup)->default_value(conf.simplify_at_startup)
        , "Perform simplification at the very start")
    ("nonstop,n", po::value(&conf.never_stop_search)->default_value(conf.never_stop_search)
        , "Never stop the search() process in class Solver")

    ("schedule", po::value(&conf.simplify_nonstartup_sequence)->default_value(conf.simplify_nonstartup_sequence, string())
        , "Schedule for simplification during run")
    ("preschedule", po::value(&conf.simplify_at_startup_sequence)->default_value(conf.simplify_at_startup_sequence, string())
        , "Schedule for simplification at startup")

    ("occschedule", po::value(&conf.occsimp_schedule_nonstartup)->default_value(conf.occsimp_schedule_nonstartup, string())
        , "Schedule for simplification during run")
    ("occpreschedule", po::value(&conf.occsimp_schedule_startup)->default_value(conf.occsimp_schedule_startup, string())
        , "Schedule for simplification at startup")


    ("confbtwsimp", po::value(&conf.num_conflicts_of_search)->default_value(conf.num_conflicts_of_search)
        , "Start first simplification after this many conflicts")
    ("confbtwsimpinc", po::value(&conf.num_conflicts_of_search_inc)->default_value(conf.num_conflicts_of_search_inc)
        , "Simp rounds increment by this power of N")
    ("varelim", po::value(&conf.doVarElim)->default_value(conf.doVarElim)
        , "Perform variable elimination as per Een and Biere")
    ("varelimto", po::value(&conf.varelim_time_limitM)->default_value(conf.varelim_time_limitM)
        , "Var elimination bogoprops M time limit")
    ("emptyelim", po::value(&conf.do_empty_varelim)->default_value(conf.do_empty_varelim)
        , "Perform empty resolvent elimination using bit-map trick")
    ("elimstrgy", po::value(&var_elim_strategy)->default_value(getNameOfElimStrategy(conf.var_elim_strategy))
        , "Sort variable elimination order by intelligent guessing ('heuristic') or by exact calculation ('calculate')")
    ("elimcplxupd", po::value(&conf.updateVarElimComplexityOTF)->default_value(conf.updateVarElimComplexityOTF)
        , "Update estimated elimination complexity on-the-fly while eliminating")
    ("elimcoststrategy", po::value(&conf.varElimCostEstimateStrategy)->default_value(conf.varElimCostEstimateStrategy)
        , "How simple strategy (guessing, above) is calculated. Valid values: 0, 1")
    ("strengthen", po::value(&conf.do_strengthen_with_occur)->default_value(conf.do_strengthen_with_occur)
        , "Perform clause contraction through self-subsuming resolution as part of the occurrence-subsumption system")
    ("bva", po::value(&conf.do_bva)->default_value(conf.do_bva)
        , "Perform bounded variable addition")
    ("bvalim", po::value(&conf.bva_limit_per_call)->default_value(conf.bva_limit_per_call)
        , "Maximum number of variables to add by BVA per call")
    ("bva2lit", po::value(&conf.bva_also_twolit_diff)->default_value(conf.bva_also_twolit_diff)
        , "BVA with 2-lit difference hack, too. Beware, this reduces the effectiveness of 1-lit diff")
    ("bvato", po::value(&conf.bva_time_limitM)->default_value(conf.bva_time_limitM)
        , "BVA time limit in bogoprops M")
    ("noextbinsubs", po::value(&conf.doExtBinSubs)->default_value(conf.doExtBinSubs)
        , "No extended subsumption with binary clauses")
    ("eratio", po::value(&conf.varElimRatioPerIter)->default_value(conf.varElimRatioPerIter, ssERatio.str())
        , "Eliminate this ratio of free variables at most per variable elimination iteration")
    ("skipresol", po::value(&conf.skip_some_bve_resolvents)->default_value(conf.skip_some_bve_resolvents)
        , "Skip BVE resolvents in case they belong to a gate")
    ("occredmax", po::value(&conf.maxRedLinkInSize)->default_value(conf.maxRedLinkInSize)
        , "Don't add to occur list any redundant clause larger than this")
    ("occirredmaxmb", po::value(&conf.maxOccurIrredMB)->default_value(conf.maxOccurIrredMB)
        , "Don't allow irredundant occur size to be beyond this many MB")
    ("occredmaxmb", po::value(&conf.maxOccurRedMB)->default_value(conf.maxOccurRedMB)
        , "Don't allow redundant occur size to be beyond this many MB")
    ("substimelim", po::value(&conf.subsumption_time_limitM)->default_value(conf.subsumption_time_limitM)
        , "Time-out in bogoprops M of subsumption of long clauses with long clauses, after computing occur")
    ("substimelim", po::value(&conf.strengthening_time_limitM)->default_value(conf.strengthening_time_limitM)
        , "Time-out in bogoprops M of strengthening of long clauses with long clauses, after computing occur")
    ("substimelim", po::value(&conf.aggressive_elim_time_limitM)->default_value(conf.aggressive_elim_time_limitM)
        , "Time-out in bogoprops M of agressive(=uses reverse distillation) var-elimination")
    ;

    std::ostringstream sccFindPercent;
    sccFindPercent << std::fixed << std::setprecision(3) << conf.sccFindPercent;

    po::options_description xorOptions("XOR-related options");
    xorOptions.add_options()
    ("xor", po::value(&conf.doFindXors)->default_value(conf.doFindXors)
        , "Discover long XORs")
    ("xorcache", po::value(&conf.useCacheWhenFindingXors)->default_value(conf.useCacheWhenFindingXors)
        , "Use cache when finding XORs. Finds a LOT more XORs, but takes a lot more time")
    ("echelonxor", po::value(&conf.doEchelonizeXOR)->default_value(conf.doEchelonizeXOR)
        , "Extract data from XORs through echelonization (TOP LEVEL ONLY)")
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
    ("sccperc", po::value(&conf.sccFindPercent)->default_value(conf.sccFindPercent, sccFindPercent.str())
        , "Perform SCC only if the number of new binary clauses is at least this many % of the number of free variables")
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
    ("moreminimcache", po::value(&conf.more_red_minim_limit_cache)->default_value(conf.more_red_minim_limit_cache)
        , "Time-out in microsteps for each more minimisation with cache. Only active if 'moreminim' is on")
    ("moreminimbin", po::value(&conf.more_red_minim_limit_binary)->default_value(conf.more_red_minim_limit_binary)
        , "Time-out in microsteps for each more minimisation with binary clauses. Only active if 'moreminim' is on")
    ("moreminimlit", po::value(&conf.max_num_lits_more_red_min)->default_value(conf.max_num_lits_more_red_min)
        , "Number of first literals to look through for more minimisation when doing learnt cl minim right after learning it")
    ("cacheformoreminim", po::value(&conf.more_otf_shrink_with_stamp)->default_value(conf.more_otf_shrink_with_stamp)
        , "Use cache for otf more minim of learnt clauses")
    ("stampformoreminim", po::value(&conf.more_otf_shrink_with_cache)->default_value(conf.more_otf_shrink_with_cache)
        , "Use stamp for otf more minim of learnt clauses")
    ("alwaysmoremin", po::value(&conf.doAlwaysFMinim)->default_value(conf.doAlwaysFMinim)
        , "Always strong-minimise clause")
    ("otfsubsume", po::value(&conf.doOTFSubsume)->default_value(conf.doOTFSubsume)
        , "Perform on-the-fly subsumption")
    ("rewardotfsubsume", po::value(&conf.rewardShortenedClauseWithConfl)
        ->default_value(conf.rewardShortenedClauseWithConfl)
        , "Reward with this many prop&confl a clause that has been shortened with on-the-fly subsumption")
    ("printimpldot", po::value(&conf.doPrintConflDot)->default_value(conf.doPrintConflDot)
        , "Print implication graph DOT files (for input into graphviz package)")
    ;

    po::options_description propOptions("Propagation options");
    propOptions.add_options()
    ("updateglueonprop", po::value(&conf.update_glues_on_prop)->default_value(conf.update_glues_on_prop)
        , "Update glues while propagating")
    ("updateglueonanalysis", po::value(&conf.update_glues_on_analyze)->default_value(conf.update_glues_on_analyze)
        , "Update glues while analyzing")
    ("binpri", po::value(&conf.propBinFirst)->default_value(conf.propBinFirst)
        , "Propagated binary clauses strictly first")
    ("otfhyper", po::value(&conf.otfHyperbin)->default_value(conf.otfHyperbin)
        , "Perform hyper-binary resolution at dec. level 1 after every restart and during probing")
    ;


    po::options_description stampOptions("Stamping options");
    stampOptions.add_options()
    ("stamp", po::value(&conf.doStamp)->default_value(conf.doStamp)
        , "Use time stamping as per Heule&Jarvisalo&Biere paper")
    ("cache", po::value(&conf.doCache)->default_value(conf.doCache)
        , "Use implication cache -- may use a lot of memory")
    ("cachesize", po::value(&conf.maxCacheSizeMB)->default_value(conf.maxCacheSizeMB)
        , "Maximum size of the implication cache in MB. It may temporarily reach higher usage, but will be deleted&disabled if this limit is reached.")
    ("calcreach", po::value(&conf.doCalcReach)->default_value(conf.doCalcReach)
        , "Calculate literal reachability")
    ("cachecutoff", po::value(&conf.cacheUpdateCutoff)->default_value(conf.cacheUpdateCutoff)
        , "If the number of literals propagated by a literal is more than this, it's not included into the implication cache")
    ;

    po::options_description sqlOptions("SQL options");
    sqlOptions.add_options()
    ("sql", po::value(&conf.doSQL)->default_value(conf.doSQL)
        , "Write to SQL. 0 = don't attempt to writ to DB, 1 = try but continue if fails, 2 = abort if cannot write to DB")
    ("wsql", po::value(&conf.whichSQL)->default_value(0)
        , "0 = prefer MySQL \
1 = prefer SQLite, \
2 = only use MySQL, \
3 = only use SQLite" )
    ("sqlitedb", po::value(&conf.sqlite_filename)
        , "Where to put the SQLite database")
    ("sqluser", po::value(&conf.sqlUser)->default_value(conf.sqlUser)
        , "SQL user to connect with")
    ("sqlpass", po::value(&conf.sqlPass)->default_value(conf.sqlPass)
        , "SQL user's pass to connect with")
    ("sqldb", po::value(&conf.sqlDatabase)->default_value(conf.sqlDatabase)
        , "SQL database name. Default is used by PHP system, so it's highly recommended")
    ("sqlserver", po::value(&conf.sqlServer)->default_value(conf.sqlServer)
        , "SQL server hostname/IP")
    ;

    po::options_description printOptions("Printing options");
    printOptions.add_options()
    ("verb", po::value(&conf.verbosity)->default_value(conf.verbosity)
        , "[0-10] Verbosity of solver. 0 = only solution")
    ("verbstat", po::value(&conf.verbStats)->default_value(conf.verbStats)
        , "Turns off verbose stats if needed")
    ("printfull", po::value(&conf.print_all_stats)->default_value(conf.print_all_stats)
        , "Print more thorough, but different stats")
    ("printsol,s", po::value(&printResult)->default_value(printResult)
        , "Print assignment if solution is SAT")
    ("printbest", po::value(&conf.doPrintBestRedClauses)->default_value(conf.doPrintBestRedClauses)
        , "Print the best N irredundant longer-than-3 learnt clauses. Value '0' means not to print anything.")
    ("printtimes", po::value(&conf.do_print_times)->default_value(conf.do_print_times)
        , "Print time it took for each simplification run. If set to 0, logs are easier to compare")
    ("restartprint", po::value(&conf.print_restart_line_every_n_confl)->default_value(conf.print_restart_line_every_n_confl)
        , "Print restart status lines at least every N conflicts")
    ;

    po::options_description miscOptions("Misc options");
    miscOptions.add_options()
    //("noparts", "Don't find&solve subproblems with subsolvers")
    ("distill", po::value(&conf.do_distill_clauses)->default_value(conf.do_distill_clauses)
        , "Regularly execute clause distillation")
    ("distillmaxm", po::value(&conf.distill_long_irred_cls_time_limitM)->default_value(conf.distill_long_irred_cls_time_limitM)
        , "Maximum number of Mega-bogoprops(~time) to spend on viviying long irred cls by enqueueing and propagating")
    ("distillto", po::value(&conf.distill_time_limitM)->default_value(conf.distill_time_limitM)
        , "Maximum time in bogoprops M for distillation")
    ("strcachemaxm", po::value(&conf.watch_cache_stamp_based_str_time_limitM)->default_value(conf.watch_cache_stamp_based_str_time_limitM)
        , "Maximum number of Mega-bogoprops(~time) to spend on viviying long irred cls through watches, cache and stamps")
    ("sortwatched", po::value(&conf.doSortWatched)->default_value(conf.doSortWatched)
        , "Sort watches according to size")
    ("renumber", po::value(&conf.doRenumberVars)->default_value(conf.doRenumberVars)
        , "Renumber variables to increase CPU cache efficiency")
    ("savemem", po::value(&conf.doSaveMem)->default_value(conf.doSaveMem)
        , "Save memory by deallocating variable space after renumbering. Only works if renumbering is active.")
    ("implicitmanip", po::value(&conf.doStrSubImplicit)->default_value(conf.doStrSubImplicit)
        , "Subsume and strengthen implicit clauses with each other")
    ("implsubsto", po::value(&conf.subsume_implicit_time_limitM)->default_value(conf.subsume_implicit_time_limitM)
        , "Timeout (in bogoprop Millions) of implicit subsumption")
    ("implstrto", po::value(&conf.strengthen_implicit_time_limitM)->default_value(conf.strengthen_implicit_time_limitM)
        , "Timeout (in bogoprop Millions) of implicit strengthening")
    ("burst", po::value(&conf.burst_search_len)->default_value(conf.burst_search_len)
        , "Number of conflicts to do in burst search")
    ("clearinter", po::value(&clear_interrupt)->default_value(0)
        , "Interrupt threads cleanly, all the time")
    ("zero-exit-status", po::bool_switch(&zero_exit_status)
        , "Exit with status zero in case the solving has finished without an issue")
    ;

    po::options_description componentOptions("Component options");
    componentOptions.add_options()
    ("findcomp", po::value(&conf.doFindComps)->default_value(conf.doFindComps)
        , "Find components, but do not treat them")
    ("comps", po::value(&conf.doCompHandler)->default_value(conf.doCompHandler)
        , "Perform component-finding and separate handling")
    ("compsfrom", po::value(&conf.handlerFromSimpNum)->default_value(conf.handlerFromSimpNum)
        , "Component finding only after this many simplification rounds")
    ("compsvar", po::value(&conf.compVarLimit)->default_value(conf.compVarLimit)
        , "Only use components in case the number of variables is below this limit")
    ("compslimit", po::value(&conf.comp_find_time_limitM)->default_value(conf.comp_find_time_limitM)
        , "Limit how much time is spent in component-finding");

    p.add("input", 1);
    p.add("drup", 1);

    cmdline_options
    .add(generalOptions)
    #if defined(USE_MYSQL) or defined(USE_SQLITE3)
    .add(sqlOptions)
    #endif
    .add(restartOptions)
    .add(printOptions)
    .add(propOptions)
    .add(reduceDBOptions)
    .add(varPickOptions)
    .add(polar_options)
    .add(conflOptions)
    .add(iterativeOptions)
    .add(probeOptions)
    .add(stampOptions)
    .add(simplificationOptions)
    .add(eqLitOpts)
    .add(componentOptions)
    #ifdef USE_M4RI
    .add(xorOptions)
    #endif
    .add(gateOptions)
    .add(miscOptions)
    ;
}

void Main::check_options_correctness()
{
    try {
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
        //po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            cout
            << "USAGE: " << argv[0] << " [options] <input-files>" << endl
            << " where input is "
            #ifndef USE_ZLIB
            << "plain"
            #else
            << "plain or gzipped"
            #endif
            << " DIMACS." << endl;

            cout << cmdline_options << endl;
            cout << "Default schedule for simplifier: " << conf.simplify_nonstartup_sequence << endl;
            cout << "Default schedule for simplifier at startup: " << conf.simplify_at_startup_sequence << endl;

            cout << "Default schedule for occur simplifier: " << conf.occsimp_schedule_nonstartup<< endl;
            cout << "Default schedule for occur simplifier at startup: " << conf.occsimp_schedule_startup << endl;
            std::exit(0);
        }

        po::notify(vm);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::unknown_option> >& c
    ) {
        cerr
        << "ERROR: Some option you gave was wrong. Please give '--help' to get help" << endl
        << "       Unkown option: " << c.what() << endl;
        std::exit(-1);
    } catch (boost::bad_any_cast &e) {
        std::cerr
        << "ERROR! You probably gave a wrong argument type" << endl
        << "       Bad cast: " << e.what()
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::invalid_option_value> > what
    ) {
        cerr
        << "ERROR: Invalid value '" << what.what() << "'" << endl
        << "       given to option '" << what.get_option_name() << "'"
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::multiple_occurrences> > what
    ) {
        cerr
        << "ERROR: " << what.what() << " of option '"
        << what.get_option_name() << "'"
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::required_option> > what
    ) {
        cerr
        << "ERROR: You forgot to give a required option '"
        << what.get_option_name() << "'"
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::too_many_positional_options_error> > what
    ) {
        cerr
        << "ERROR: You gave too many files. Only 2 files can be given:" << endl
        << "       the 1st the CNF file input, the 2nd the DRUP file output"
        << endl;

        std::exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::ambiguous_option> > what
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
        boost::exception_detail::error_info_injector<po::invalid_command_line_syntax> > what
    ) {
        cerr
        << "ERROR: The option you gave is missing the argument or the" << endl
        << "       argument is given with space between the equal sign." << endl
        << "       detailed error message: " << what.what() << endl
        ;
        std::exit(-1);
    }
}

void Main::handle_drup_option()
{
    if (drupDebug) {
        drupf = &std::cout;
    } else {
        if (drupExistsCheck && fileExists(drupfilname)) {
            std::cerr
            << "ERROR! File selected for DRUP output, '"
            << drupfilname
            << "' already exists. Please delete the file or pick another"
            << endl
            << "DRUP filename"
            << endl;
            std::exit(-1);
        }
        std::ofstream* drupfTmp = new std::ofstream;
        drupfTmp->open(drupfilname.c_str(), std::ofstream::out);
        if (!*drupfTmp) {
            std::cerr
            << "ERROR: Could not open DRUP file "
            << drupfilname
            << " for writing"
            << endl;

            std::exit(-1);
        }
        drupf = drupfTmp;
    }

    if (!conf.otfHyperbin) {
        if (conf.verbosity >= 2) {
            cout
            << "c OTF hyper-bin is needed for BProp in DRUP, turning it back"
            << endl;
        }
        conf.otfHyperbin = true;
    }

    if (conf.doFindXors) {
        if (conf.verbosity >= 2) {
            cout
            << "c XOR manipulation is not supported in DRUP, turning it off"
            << endl;
        }
        conf.doFindXors = false;
    }

    if (conf.doRenumberVars) {
        if (conf.verbosity >= 2) {
            cout
            << "c Variable renumbering is not supported during DRUP, turning it off"
            << endl;
        }
        conf.doRenumberVars = false;
    }

    if (conf.doCompHandler) {
        if (conf.verbosity >= 2) {
            cout
            << "c Component finding & solving is not supported during DRUP, turning it off"
            << endl;
        }
        conf.doCompHandler = false;
    }
}

void Main::parse_var_elim_strategy()
{
    if (var_elim_strategy == getNameOfElimStrategy(ElimStrategy::heuristic)) {
        conf.var_elim_strategy = ElimStrategy::heuristic;
    } else if (var_elim_strategy == getNameOfElimStrategy(ElimStrategy::calculate_exactly)) {
        conf.var_elim_strategy = ElimStrategy::calculate_exactly;
    } else {
        std::cerr
        << "ERROR: Cannot parse option given to '--elimstrgy'. It's '"
        << var_elim_strategy << "'" << " but that none of the possiblities listed."
        << endl;

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
    if (conf.shortTermHistorySize <= 0) {
        cout
        << "You MUST give a short term history size (\"--gluehist\")" << endl
        << "  greater than 0!"
        << endl;

        std::exit(-1);
    }

    if (vm.count("dumpresult")) {
        needResultFile = true;
    }

    parse_polarity_type();

    if (conf.random_var_freq < 0 || conf.random_var_freq > 1) {
        throw WrongParam(lexical_cast<string>(conf.random_var_freq), "Illegal random var frequency ");
    }

    //Conflict
    if (vm.count("maxdump") && redDumpFname.empty()) {
        throw WrongParam("maxdump", "--dumpred <filename> must be activated if issuing --maxdump <size>");
    }

    parse_restart_type();
    parse_var_elim_strategy();

    if (vm.count("input")) {
        filesToRead = vm["input"].as<vector<string> >();
        if (!vm.count("sqlitedb")) {
            conf.sqlite_filename = filesToRead[0] + ".sqlite";
        } else {
            conf.sqlite_filename = vm["sqlitedb"].as<string>();
        }
        fileNamePresent = true;
    } else {
        fileNamePresent = false;
    }

    if (vm.count("drup")) {
        handle_drup_option();
    }

    if (conf.verbosity >= 1) {
        cout << "c Outputting solution to console" << endl;
    }

    if (satcomp) {
        conf.varElimRatioPerIter = 1.0;
        conf.simplify_at_startup = true;
        conf.probe_bogoprops_time_limitM *= 2;
    }
}

void Main::parseCommandLine()
{
    clear_interrupt = 0;
    conf.verbosity = 2;
    conf.verbStats = 1;

    //Reconstruct the command line so we can emit it later if needed
    for(int i = 0; i < argc; i++) {
        commandLine += string(argv[i]);
        if (i+1 < argc) {
            commandLine += " ";
        }
    }

    add_supported_options();
    check_options_correctness();
    if (vm.count("version")) {
        printVersionInfo();
        std::exit(0);
    }

    try {
        manually_parse_some_options();
    } catch(WrongParam& p) {
        cerr << "ERROR: " << p.getMsg() << endl;
        exit(-1);
    }
}

void Main::printVersionInfo()
{
    cout << "c CryptoMiniSat version " << solver->get_version() << endl;
    cout << "c CryptoMiniSat SHA revision " << solver->get_version_sha1() << endl;
    cout << "c CryptoMiniSat compilation env " << solver->get_compilation_env() << endl;
    #ifdef __GNUC__
    cout << "c compiled with gcc version " << __VERSION__ << endl;
    #else
    cout << "c compiled with non-gcc compiler" << endl;
    #endif
}

void Main::dumpIfNeeded() const
{
    if (redDumpFname.empty()
        && irredDumpFname.empty()
    ) {
        return;
    }

    if (!redDumpFname.empty()) {
        solver->open_file_and_dump_red_clauses(redDumpFname);
        if (conf.verbosity >= 1) {
            cout << "c Dumped redundant clauses" << endl;
        }
    }

    if (!irredDumpFname.empty()) {
        solver->open_file_and_dump_irred_clauses(irredDumpFname);
        if (conf.verbosity >= 1) {
            cout
            << "c [solver] Dumped irredundant clauses to file "
            << "'" << irredDumpFname << "'." << endl
            << "c [solver] Note that these may NOT be in the original CNF, but"
            << " *describe the same problem* with the *same variables*"
            << endl;
        }
    }
}

void Main::check_num_threads_sanity(const unsigned thread_num) const
{
    const unsigned num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) {
        //Library doesn't know much, we can't do any checks.
        return;
    }

    if (thread_num > num_cores) {
        std::cerr
        << "c WARNING: Number of threads requested is more than the number of"
        << " cores reported by the system.\n"
        << "c WARNING: This is not a good idea in general. It's best to set the"
        << " number of threads to the number of real cores" << endl;
    }
}

int Main::solve()
{
    solver = new SATSolver((void*)&conf);
    solverToInterrupt = solver;
    if (drupf) {
        solver->set_drup(drupf);
    }
    check_num_threads_sanity(num_threads);
    solver->set_num_threads(num_threads);

    std::ofstream resultfile;
    if (needResultFile) {
        resultfile.open(resultFilename.c_str());
        if (!resultfile) {
            cout
            << "ERROR: Couldn't open file '"
            << resultFilename
            << "' for writing!"
            << endl;
            std::exit(-1);
        }
    }

    //Print command line used to execute the solver: for options and inputs
    if (conf.verbosity >= 1) {
        printVersionInfo();
        cout
        << "c Executed with command line: "
        << commandLine
        << endl;
    }
    solver->add_sql_tag("commandline", commandLine);

    //Parse in DIMACS (maybe gzipped) files
    //solver->log_to_file("mydump.cnf");
    parseInAllFiles();

    //Multi-solutions
    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    while(current_nr_of_solutions < max_nr_of_solutions && ret == l_True) {
        ret = solver->solve();
        current_nr_of_solutions++;

        if (ret == l_True && current_nr_of_solutions < max_nr_of_solutions) {
            //Print result
            printResultFunc(&cout, false, ret, current_nr_of_solutions == 1);
            if (needResultFile) {
                printResultFunc(&resultfile, true, ret, current_nr_of_solutions == 1);
            }

            if (conf.verbosity >= 1) {
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
            for (uint32_t var = 0; var < solver->nVars(); var++) {
                if (solver->get_model()[var] != l_Undef) {
                    lits.push_back( Lit(var, (solver->get_model()[var] == l_True)? true : false) );
                }
            }
            solver->add_clause(lits);
        }
    }

    dumpIfNeeded();

    if (ret == l_Undef && conf.verbosity >= 1) {
        cout
        << "c Not finished running -- signal caught or some maximum reached"
        << endl;
    }
    if (conf.verbosity >= 1) {
        solver->print_stats();
    }

    //Final print of solution
    printResultFunc(&cout, false, ret, current_nr_of_solutions == 1);
    if (needResultFile) {
        printResultFunc(&resultfile, true, ret, current_nr_of_solutions == 1);
    }

    //Delete solver
    delete solver;
    solver = NULL;

    if (drupf) {
        //flush DRUP
        *drupf << std::flush;

        //If it's not stdout, we have to delete the ofstream
        if (drupf != &std::cout) {
            delete drupf;
        }
    }

    return correctReturnValue(ret);
}

int Main::correctReturnValue(const lbool ret) const
{
    if (zero_exit_status) {
        return 0;
    }

    int retval = -1;
    if      (ret == l_True)  retval = 10;
    else if (ret == l_False) retval = 20;
    else if (ret == l_Undef) retval = 15;
    else {
        cerr
        << "Something is very wrong, output is neither l_Undef, nor l_False, nor l_True"
        << endl;

        std::exit(-1);
    }

    return retval;
}

int main(int argc, char** argv)
{
    Main main(argc, argv);
    main.parseCommandLine();

    signal(SIGINT, SIGINT_handler);
    //signal(SIGHUP,SIGINT_handler);

    return main.solve();
}
