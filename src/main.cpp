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

#include "solvertypesmini.h"
#define DEBUG_DIMACSPARSER_CMS

#include <cerrno>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
#include <cstring>
#include <thread>
#include <charconv>
#include <type_traits>
#include <stdexcept>

#include "main.h"
#include "time_mem.h"
#include "dimacsparser.h"
#include "cryptominisat.h"
#include "signalcode.h"
#include "argparse.hpp"

using namespace CMSat;

using std::cout;
using std::cerr;
using std::endl;

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

void Main::readInAFile(SATSolver* solver2, const string& filename) {
    std::unique_ptr<FieldGen> fg = std::make_unique<FGenDouble>();
    solver2->add_sql_tag("filename", filename);
    if (conf.verbosity) cout << "c Reading file '" << filename << "'" << endl;
    #ifndef USE_ZLIB
    FILE * in = fopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<FILE*, FN>, SATSolver> parser(solver2, &debugLib, conf.verbosity, fg);
    #else
    gzFile in = gzopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<gzFile, GZ>, SATSolver> parser(solver2, &debugLib, conf.verbosity, fg);
    #endif

    if (in == nullptr) {
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

    #ifndef USE_ZLIB
        fclose(in);
    #else
        gzclose(in);
    #endif
}

void Main::readInStandardInput(SATSolver* solver2)
{
    if (conf.verbosity) cout << "c Reading from standard input... Use '-h' or '--help' for help." << endl;
    std::unique_ptr<FieldGen> fg = std::make_unique<FGenDouble>();

    #ifndef USE_ZLIB
    FILE * in = stdin;
    #else
    gzFile in = gzdopen(0, "rb"); //opens stdin, which is 0
    #endif

    if (in == nullptr) {
        std::cerr << "ERROR! Could not open standard input for reading" << endl;
        std::exit(1);
    }

    #ifndef USE_ZLIB
    DimacsParser<StreamBuffer<FILE*, FN>, SATSolver> parser(solver2, &debugLib, conf.verbosity, fg);
    #else
    DimacsParser<StreamBuffer<gzFile, GZ>, SATSolver> parser(solver2, &debugLib, conf.verbosity, fg);
    #endif

    if (!parser.parse_DIMACS(in, false)) exit(-1);
    #ifdef USE_ZLIB
        gzclose(in);
    #endif
}

void Main::parseInAllFiles(SATSolver* solver2)
{
    const double my_timeTotal = cpuTimeTotal();
    const double my_time = cpu_time();

    //First read normal extra files
    solver->add_sql_tag("stdin", fileNamePresent ? "False" : "True");
    if (!fileNamePresent) readInStandardInput(solver2);
    else readInAFile(solver2, input_file);

    if (conf.verbosity) {
        if (num_threads > 1) {
            cout
            << "c Sum parsing time among all threads (wall time will differ): "
            << std::fixed << std::setprecision(2)
            << (cpuTimeTotal() - my_timeTotal)
            << " s" << endl;
        } else {
            cout
            << "c Parsing time: "
            << std::fixed << std::setprecision(2)
            << (cpu_time() - my_time)
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

            if (!solver->get_sampl_vars_set()) {
                for (uint32_t var = 0; var < solver->nVars(); var++) {
                    fun(var);
                }

            } else {
                for (uint32_t var: solver->get_sampl_vars()) {
                    fun(var);
                }
            }
            *os << "0" << endl;
        } else {
            uint32_t num_undef;
            if (!solver->get_sampl_vars_set()) {
                num_undef = print_model(solver, os);
            } else {
                num_undef = print_model(solver, os, &solver->get_sampl_vars());
            }
            if (num_undef && !toFile && conf.verbosity) {
                cout << "c NOTE: " << num_undef << " variables are UNDEF. Sampling vars set:"
                    << solver->get_sampl_vars_set() << endl;
            }
        }
    }
}

template<class T> static T parse_opt(const std::string& s) {
    if constexpr (std::is_same_v<T, std::string>) {
        return s;
    } else if constexpr (std::is_same_v<T, bool>) {
        return parse_opt<int>(s) != 0;
    } else if constexpr (std::is_floating_point_v<T>) {
        size_t pos = 0;
        double val;
        try { val = std::stod(s, &pos); }
        catch (const std::exception&) { throw std::invalid_argument("not a number: " + s); }
        if (pos != s.size()) throw std::invalid_argument("trailing characters in number: " + s);
        return val;
    } else if constexpr (std::is_integral_v<T>) {
        T val{};
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc::result_out_of_range) throw std::invalid_argument("integer out of range: " + s);
        if (ec != std::errc{}) throw std::invalid_argument("not an integer: " + s);
        if (ptr != s.data() + s.size()) throw std::invalid_argument("trailing characters in integer: " + s);
        return val;
    } else {
        static_assert(sizeof(T) == 0, "parse_opt: unsupported option type");
    }
}

template<class T>
argparse::Argument& Main::opt(const char* name, T& var, const char* help) {
    return program.add_argument(name)
        .action([&var](const std::string& a) { var = parse_opt<T>(a); })
        .default_value(var)
        .help(help);
}

template<class T>
argparse::Argument& Main::opt(const char* short_name, const char* name, T& var, const char* help) {
    return program.add_argument(short_name, name)
        .action([&var](const std::string& a) { var = parse_opt<T>(a); })
        .default_value(var)
        .help(help);
}

void Main::readInAssumptions()
{
    if (assump_filename.empty()) return;

    std::ifstream tmp;
    tmp.open(assump_filename.c_str());
    if (!tmp.is_open()) {
        std::cerr
        << "ERROR! Could not open assumptions file '"
        << assump_filename
        << "' for reading: " << strerror(errno) << endl;
        std::exit(1);
    }

    std::string line;
    size_t line_no = 0;
    while(std::getline(tmp, line)) {
        line_no++;

        //Trim leading/trailing whitespace, skip empty/blank lines
        const auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        const auto last = line.find_last_not_of(" \t\r\n");
        const std::string token = line.substr(first, last - first + 1);

        int x = 0;
        try {
            x = parse_opt<int>(token);
        } catch (const std::invalid_argument&) {
            std::cerr
            << "ERROR! Could not parse assumptions file '"
            << assump_filename
            << "' at line " << line_no
            << ": expected a single integer literal, got '"
            << token << "'" << endl;
            std::exit(1);
        }
        if (x == 0) {
            std::cerr
            << "ERROR! Invalid assumption in file '"
            << assump_filename
            << "' at line " << line_no
            << ": literal must not be 0" << endl;
            std::exit(1);
        }

        cout << "Assume: " << x << endl;
        assumps.push_back(Lit(std::abs(x)-1, x < 0));
    }
}

/* clang-format off */
void Main::add_supported_options() {
    program.add_argument("--version", "-v")
        .action([&](const auto ) {printVersionInfo(); exit(0);})
        .flag()
        .help("Print version information");
    opt("--verb", conf.verbosity,
        "[0-10] Verbosity of solver. 0 = only solution");
    opt("--xlrup", xlrup_mode,
        "Emit the proof in XLRUP format, checkable directly by cake_xlrup. Set to 0 to emit raw FRAT instead, for debugging proof generation with frat-rs [0..1]")
        .metavar("{0,1}");
    program.add_argument("--maxtime")
        .help("Stop solving after this much time (s)")
        .scan<'g', double>();
    program.add_argument("--maxconfl")
        .help("Stop solving after this many conflicts")
        .scan<'d', uint64_t>();
    opt("-r", "--random", conf.orig_seed,
        "[0..] Random seed");
    opt("-t", "--threads", num_threads,
        "Number of threads");
    opt("-m", "--mult", conf.orig_global_timeout_multiplier,
        "Time multiplier for all simplification cutoffs");
    opt("--nextm", conf.global_next_multiplier,
        "Global multiplier when the next inprocessing should take place");
    opt("--memoutmult", conf.var_and_mem_out_mult,
        "Multiplier for memory-out checks on inprocessing functions. It limits things such as clause-link-in. Useful when you have limited memory but still want to do some inprocessing");
    opt("--maxsol", max_nr_of_solutions,
        "Search for given amount of solutions. Thanks to Jannis Harder for the decision-based banning idea");
    program.add_argument("--polar")
        .default_value("auto")
        .help("{true,false,rnd,weight,auto} Selects polarity mode. 'true'/'false' -> always branch positive/negative. 'weight' -> random, biased by the per-variable weight. 'auto' -> CaDiCaL's saved/target/best phases with rephasing");
    opt("--scc", conf.do_find_and_replace_eq_lits,
        "Find equivalent literals through SCC and replace them");

    #ifdef STATS_NEEDED
    program.add_argument("--clid")
        .flag()
        .action([&](const auto&) {clause_ID_needed = true;})
        .help("Add clause IDs to FRAT output");
    #endif

    #ifdef FINAL_PREDICTOR
    opt("--predloc", conf.pred_conf_location,
        "Directory with predictor-<table>-<tier>-<type>.json (see --predtables), empty = use built-in models");
    opt("--predtype", conf.predictor_type,
        "Type of predictor. Supported: py, xgb");
    opt("--predtables", conf.pred_tables,
        "Per short/long/forever: 0 = used_later, 1 = used_later_anc. 000 = normal for all, 111 = ancestor for all");
    opt("--predbestfeats", conf.predict_best_feat_fname,
        "Best features file, only for --predtype py");

    opt("--predsortby", conf.pred_sort_by,
        "Reduce removes the candidates with the lowest predicted use over the next: 0 = short, 1 = long, 2 = forever horizon, 3 = sum of the three (near-term counts 3x, long-term still counts)");
    opt("--dumppreddistrib", conf.dump_pred_distrib,
        "Dump predictions of all clauses at every reduce to pred_distrib.csv");
    #endif

    /* po::options_description restartOptions("Restart options"); */
    opt("--restart", conf.do_restart,
        "Enable restarts");
    opt("--restartint", conf.restartint,
        "Minimum number of conflicts between restarts");
    opt("--restartmargin", conf.restartmargin,
        "Percent the fast glue EMA must be above the slow one to restart");
    opt("--emagluefast", conf.emagluefast,
        "Window size of the fast glue EMA");
    opt("--emaglueslow", conf.emaglueslow,
        "Window size of the slow glue EMA");
    opt("--stabilize", conf.do_stabilize,
        "Alternate stable (reluctant doubling) and focused (glue EMA) phases");
    opt("--stabilizeint", conf.stabilizeint,
        "Length of first stabilizing phase, in conflicts");
    opt("--stabilizefactor", conf.stabilizefactor,
        "Multiplier of stabilizing phase length at each phase change");
    opt("--stabilizemaxint", conf.stabilizemaxint,
        "Maximum stabilizing phase length");
    opt("--reluctant", conf.reluctantint,
        "Reluctant doubling base period for stable-phase restarts, 0 = never restart there");
    opt("--reluctantmax", conf.reluctantmax,
        "Maximum reluctant doubling period multiplier");

    /* po::options_description reduceDBOptions("Redundant clause options"); */
    opt("--reduce", conf.reduce,
        "Enable learnt clause DB reduction");
    opt("--reduceint", conf.reduceint,
        "Base reduce interval, in conflicts");
    opt("--reducetarget", conf.reducetarget,
        "Percent of unused reduce candidates removed per reduce");
    opt("--reducelow", conf.reducelow,
        "Fraction per mille of reduce candidates removed at the first reduce, rising towards --reducehigh as kissat. Set >= reducehigh to use --reducetarget");
    opt("--reducehigh", conf.reducehigh,
        "Asymptotic fraction per mille of reduce candidates removed");
    opt("--reducekeepused", conf.reduce_keep_used,
        "Keep every learnt clause used since the last reduce, as CaDiCaL. 0: kissat, tier3 is always a candidate");
    opt("--eagersubsume", conf.eager_subsume,
        "Demote the last learnt clauses subsumed by a new one, as kissat");
    opt("--reducetier1glue", conf.reducetier1glue,
        "Glue at/below which learnt clauses are kept forever");
    opt("--reducetier2glue", conf.reducetier2glue,
        "Glue at/below which learnt clauses get a double life");
    opt("--dyntiers", conf.dynamic_tiers,
        "Recompute tier1/tier2 glue limits from glue usage at each reduce, as kissat");
    opt("--flush", conf.flush,
        "Once in a while flush ALL unused redundant clauses");
    opt("--flushfactor", conf.flushfactor,
        "Flush interval multiplier");
    opt("--flushint", conf.flushint,
        "Initial flush interval, in conflicts");
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    opt("--everypred", conf.every_pred_reduce,
        "Calculate satzilla features every N conflicts (STATS builds)");
    #endif

    /* po::options_description varPickOptions("Variable branching options"); */
    opt("--branchstr", conf.branch_strategy_setup,
        "Branch strategy string that switches between different branch strategies while solving e.g. 'vsids1+vsids2'");

    program.add_argument("--nobansol")
        .flag()
        .action([&](const auto&) {dont_ban_solutions = true;})
        .help("Don't ban the solution once it's found");
    program.add_argument("--debuglib")
        .action([&](const auto& a) {debugLib = a;})
        .help("Parse special comments to run solve/simplify during parsing of CNF");

    /* po::options_description breakid_options("Breakid options"); */
    opt("--breakid", conf.do_breakid,
        "Run BreakID to break symmetries.");
    opt("--breakideveryn", conf.breakid_every_n,
        "Run BreakID every N simplification iterations");
    opt("--breakidmaxlits", conf.breakid_lits_limit_K,
        "Maximum number of literals in thousands. If exceeded, BreakID will not run");
    opt("--breakidmaxcls", conf.breakid_cls_limit_K,
        "Maximum number of clauses in thousands. If exceeded, BreakID will not run");
    opt("--breakidmaxvars", conf.breakid_vars_limit_K,
        "Maximum number of variables in thousands. If exceeded, BreakID will not run");
    opt("--breakidtime", conf.breakid_time_limit_K,
        "Maximum number of steps taken during automorphism finding.");
    opt("--breakidcls", conf.breakid_max_constr_per_permut,
        "Maximum number of breaking clauses per permutation.");
    opt("--breakidmatrix", conf.breakid_matrix_detect,
        "Detect matrix row interchangability");

    /* po::options_description sls_options("Stochastic Local Search options"); */
    opt("--sls", conf.do_sls,
        "Run local search ('walk') during rephasing");
    opt("--walknonstable", conf.walknonstable,
        "Run local search during focused phases too");
    opt("--walkseedphase", conf.walkseedphase,
        "Start local search off the CDCL phases, as CaDiCaL does");
    opt("--walkinitially", conf.walkinitially,
        "Local search rounds to run before simplifying and searching, 0=none");
    opt("--walkxorweight", conf.walkxorweight,
        "Weight of XOR constraints in yalsat's break values, times 100 (range 0-1000)");
    opt("--walkmineff", conf.walkmineff,
        "Minimum local search effort, in yalsat mems");
    opt("--walkmaxeff", conf.walkmaxeff,
        "Maximum local search effort, in yalsat mems");
    opt("--walkreleff", conf.walkreleff,
        "Local search effort per mille of the search propagations done so far");
    opt("--slsmaxmem", conf.sls_memoutMB,
        "Maximum number of MB to give to the local search solver. Skips local search if handing over the formula would need more.");
    opt("--ccnrneighmaxsz", conf.ccnr_neighbor_max_cl_size,
        "CCNR builds no neighbor edges for clauses longer than this. The neighborhood is quadratic in clause size, so one huge clause costs GBs and makes every flip of its vars charge thousands of mems");
    opt("--backboneccnrlim", conf.backbone_ccnr_mems_limitM,
        "Mems budget, in millions, for each of the CCNR local search tries that pre-filter backbone candidates. Too low and no model is found, so cadiback must test every variable");

    /* po::options_description rephase_options("Rephasing options"); */
    opt("--rephase", conf.do_rephase,
        "Enable resetting the saved phases");
    opt("--rephaseint", conf.rephaseint,
        "Rephase interval, in conflicts. The interval grows arithmetically.");
    opt("--phase", conf.phase,
        "Default decision polarity");
    opt("--lucky", conf.lucky,
        "Search for lucky phases before the CDCL loop");
    opt("--target", conf.target_phases,
        "Decide on target phases. 0 = never, 1 = stable phases only, 2 = always");

    /* po::options_description probeOptions("Probing options"); */
    opt("--transred", conf.do_trans_red,
        "Remove useless binary clauses (transitive reduction)");
    opt("--intree", conf.do_intree_probe,
        "Carry out intree-based probing");
    opt("--fullprobe", conf.do_full_probe,
        "Regularly probe both polarities of variables during search");
    opt("--fullprobemaxm", conf.full_probe_time_limitM,
        "Time in mega-bogoprops to perform full probing");
    opt("--intreemaxm", conf.intree_time_limitM,
        "Time in mega-bogoprops to perform intree probing");
    opt("--intreeeff", conf.intree_effort,
        "Intree probing budget as a fraction of all propagations since its last call");
    opt("--otfhyper", conf.do_hyperbin_and_transred,
        "Perform hyper-binary resolution during probing");
    opt("--hyperkeepconfl", conf.hyperbin_keep_confl,
        "Conflicts an unused intree hyper-bin is kept for before being dropped");

    /* po::options_description simp_schedules("Simplification schedules"); */
    opt("--schedsimp", conf.do_simplify_problem,
        "Perform simplification rounds. If 0, we never perform any.");
    opt("--presimp", conf.simplify_at_startup,
        "Perform simplification at the very start");
    opt("--allpresimp", conf.simplify_at_every_startup,
        "Perform simplification at EVERY start -- only matters in library mode");
    opt("-n", "--nonstop", conf.never_stop_search,
        "Never stop the search() process in class SATSolver");
    opt("--maxnumsimppersolve", conf.max_num_simplify_per_solve_call,
        "Maximum number of simplifications to perform for every solve() call. After this, no more inprocessing will take place.");

    program.add_argument("--schedule")
        .action([&](const auto& a) {conf.simplify_schedule_nonstartup = a;})
        .help("Schedule for simplification during run");
    program.add_argument("--preschedule")
        .action([&](const auto& a) {conf.simplify_schedule_startup = a;})
        .help("Schedule for simplification at startup");
    opt("--occsimp", conf.perform_occur_based_simp,
        "Perform occurrence-list-based optimisations (variable elimination, subsumption, bounded variable addition...)");
    opt("--confbtwsimp", conf.num_conflicts_of_search,
        "Start first simplification after this many conflicts");
    opt("--confbtwsimpinc", conf.num_conflicts_of_search_inc,
        "Simp rounds increment by this power of N");

    /* po::options_description tern_res_options("Ternary resolution"); */
    std::ostringstream tern_max_create;
    tern_max_create << std::setprecision(2) << conf.ternary_max_create;
    opt("--tern", conf.do_ternary,
        "Perform Ternary resolution");
    opt("--terntimelim", conf.ternary_res_time_limitM,
        "Time-out in bogoprops M of ternary resolution as per paper 'Look-Ahead Versus Look-Back for Satisfiability Problems'");
    opt("--terncreate", conf.ternary_max_create,
        "Create only this multiple (of linked in cls) ternary resolution clauses per simp run");
    opt("--ternbincreate", conf.allow_ternary_bin_create,
        "Allow ternary resolving to generate binary clauses");

    /* po::options_description occ_mem_limits("Occ-based simplification memory limits"); */
    opt("--occredmax", conf.max_red_link_in_size,
        "Don't add to occur list any redundant clause larger than this");
    opt("--occredmaxmb", conf.max_occur_red_mb,
        "Don't allow redundant occur size to be beyond this many MB");
    opt("--occirredmaxmb", conf.max_occur_irred_mb,
        "Don't allow irredundant occur size to be beyond this many MB");
    ;

    /* po::options_description sub_str_time_limits("Occ-based subsumption and strengthening time limits"); */
    opt("--strengthen", conf.do_strengthen_with_occur,
        "Perform clause contraction through self-subsuming resolution as part of the occurrence-subsumption system");
    opt("--weakentimelim", conf.weaken_time_limitM,
        "Time-out in bogoprops M of weakening used");
    opt("--substimelim", conf.subsumption_time_limitM,
        "Time-out in bogoprops M of subsumption of long clauses with long clauses, after computing occur");
    opt("--substimelimbinratio", conf.subsumption_time_limit_ratio_sub_str_w_bin,
        "Ratio of subsumption time limit to spend on sub&str long clauses with bin");
    opt("--substimelimlongratio", conf.subsumption_time_limit_ratio_sub_w_long,
        "Ratio of subsumption time limit to spend on sub long clauses with long");
    opt("--strstimelim", conf.strengthening_time_limitM,
        "Time-out in bogoprops M of strengthening of long clauses with long clauses, after computing occur");
    opt("--sublonggothrough", conf.subsume_gothrough_multip,
        "How many times go through subsume");

    /* po::options_description bva_options("BVA options"); */
    opt("--bva", conf.do_bva,
        "Perform bounded variable addition");

    /* po::options_description bve_options("BVE options"); */
    opt("--varelim", conf.do_var_elim,
        "Perform variable elimination as per Een and Biere");
    opt("--varelimto", conf.varelim_time_limitM,
        "Var elimination bogoprops M time limit");
    opt("--varelimover", conf.min_bva_gain,
        "Do BVE until the resulting no. of clause increase is less than X. Only power of 2 makes sense, i.e. 2,4,8...");
    opt("--emptyelim", conf.do_empty_varelim,
        "Perform empty resolvent elimination using bit-map trick");
    opt("--varelimmaxmb", conf.var_linkin_limit_MB,
        "Maximum extra MB of memory to use for new clauses during varelim");
    opt("--eratio", conf.var_elim_ratio_per_iter,
        "Eliminate this ratio of free variables at most per variable elimination iteration");
    opt("--varelimocclim", conf.varelim_occ_cutoff,
        "Don't try to eliminate a variable whose more frequent polarity occurs more than this many times. 0 = no limit");
    opt("--varelimprodlim", conf.varelim_occ_prod_cutoff,
        "Don't try to eliminate a variable whose pos*neg occurrence product is over this");
    opt("--varelimschedtouched", conf.varelim_sched_only_touched,
        "Only schedule for elimination the vars whose clauses changed since BVE last looked (CaDiCaL's Flags::elim). 0 = schedule every eligible var");
    opt("--weakenclsmaxsz", conf.weaken_max_cls_size,
        "Don't weaken a clause longer than this during BVE. 0 = no limit");
    opt("--varelimclsmaxsz", conf.varelim_max_cls_size,
        "Don't try to eliminate a variable that occurs in a clause longer than this. 0 = no limit");
    opt("--varelimclslim", conf.velim_resolvent_too_large,
        "Maximum resolvent size during BVE, -1 = no limit");
    opt("--varelimirregocclim", conf.varelim_irreg_gate_occ_cutoff,
        "Don't run kitten-based irregular gate finding if the variable has more occurrences than this");
    opt("--varelimirregconfl", conf.varelim_irreg_gate_confl_limit,
        "Picosat conflict budget for one irregular gate query during BVE");
    opt("--varelimirregunit", conf.varelim_irreg_gate_unit,
        "Turn a one-sided irregular-gate core into a unit instead of a gate");
    opt("--varelimprod", conf.varelim_score_prod,
        "Weight of pos*neg in the BVE ordering score");
    opt("--varelimsum", conf.varelim_score_sum,
        "Weight of pos+neg in the BVE ordering score");
    opt("--varelimcheckres", conf.varelim_check_resolvent_subs,
        "BVE should check whether resolvents subsume others and check for exact size increase");
    opt("--occrelocatelim", conf.occ_relocate_lim,
        "When strengthening removes a literal whose occurrence list is longer than this, move the clause to a new place instead of searching the list");

    /* po::options_description xorOptions("XOR-related options"); */
    opt("--xor", conf.do_find_xors,
        "Discover long XORs");
    opt("--maxxorsize", conf.max_xor_to_find,
        "Maximum XOR size to find");
    opt("--xorfindtout", conf.xor_finder_time_limitM,
        "Time limit for finding XORs");
    opt("--maxxormat", conf.max_xor_matrix,
        "Maximum matrix size (=num elements) that we should try to echelonize");

    /* po::options_description gateOptions("Gate-related options"); */
    opt("--gates", conf.do_gate_find,
        "Find gates.");
    opt("--printgatedot", conf.do_print_gate_dot,
        "Print gate structure regularly to file 'gatesX.dot'");
    opt("--gatefindto", conf.gatefinder_time_limitM,
        "Max time in bogoprops M to find gates");

    /* po::options_description conflOptions("Conflict options"); */
    opt("--recur", conf.do_recursive_minim,
        "Perform recursive minimisation");
    opt("--moreminim", conf.do_minim_red_more,
        "Perform strong minimisation at conflict gen.");
    opt("--moremoreminim", conf.do_minim_red_more_more,
        "Perform even stronger minimisation at conflict gen.");
    opt("--moremorealways", conf.do_always_fminim,
        "Always strong-minimise clause");
    opt("--decbased", conf.do_decision_based_cl,
        "Create decision-based conflict clauses when the UIP clause is too large");

    /* po::options_description propOptions("Glue options"); */
    opt("--bumpreasondepth", conf.bump_reason_depth,
        "Bump vars in reasons of learnt clause lits up to this depth. 0 = off");
    opt("--shrink", conf.do_shrink_uip,
        "All-UIP shrinking of learnt clauses");
    opt("--otfs", conf.do_otfs,
        "On-the-fly strengthening of clauses during conflict analysis");

    /* po::options_description chrono_bt_opts("Propagation options"); */
    opt("--diffdeclevelchrono", conf.diff_declev_for_chrono,
        "Difference in decision level is more than this, perform chronological backtracking instead of non-chronological backtracking. Giving -1 means it is never turned on (overrides '--confltochrono -1' in this case).");
    opt("--chronoreusetrail", conf.do_chrono_reuse_trail,
        "On backjump, only backtrack to the level of the best-ranked var above the jump level");
    opt("--restartreusetrail", conf.do_restart_reuse_trail,
        "On restart, keep decisions that would be re-made anyway");

#ifdef USE_SQLITE3
    /* po::options_description sqlOptions("SQL options"); */
    opt("--sql", sql,
        "Write to SQL. 0 = no SQL, 1 or 2 = sqlite");
    opt("--sqlitedb", sqlite_filename,
        "SQLite database filename to write clause data to");
    opt("--sqlitedboverwrite", conf.sql_overwrite_file,
        "Overwrite the SQLite database file if it exists");
    opt("--cldatadumpratio", conf.dump_individual_cldata_ratio,
        "Only dump this ratio of clauses' data, randomly selected. Since machine learning doesn't need that much data, this can reduce the data you have to deal with.");
    opt("--cllockdatagen", conf.lock_for_data_gen_ratio,
        "Lock for data generation into lev0, setting locked_for_data_gen. Only works when clause is marked for dumping ('--cldatadumpratio' )");
#endif

    /* po::options_description printOptions("Printing options"); */
    opt("--verbstat", conf.verb_stats,
        "Change verbosity of statistics at the end of the solving [0..3]");
    opt("--verbrestart", conf.print_full_restart_stat,
        "Print more thorough, but different stats");
    opt("--verballrestarts", conf.print_all_restarts,
        "Print a line for every restart");
    opt("--printsol","-s", printResult,
        "Print assignment if solution is SAT");
    opt("--restartprint", conf.print_restart_line_every_n_confl,
        "Print restart status lines at least every N conflicts");


    /* po::options_description distillOptions("Distill options"); */
    opt("--distill", conf.do_distill_clauses,
        "Regularly execute clause distillation");
    opt("--distillbin", conf.do_distill_bin_clauses,
        "Regularly execute binary clause distillation");
    opt("--distillbineff", conf.distill_bin_effort,
        "Binary clause distillation budget as a fraction of all propagations since its last call");
    opt("--distillmaxm", conf.distill_long_cls_time_limitM,
        "Maximum number of Mega-bogoprops(~time) to spend on vivifying/distilling long cls by enqueueing and propagating");
    opt("--distillincconf", conf.distill_increase_conf_ratio,
        "Multiplier for current number of conflicts OTF distill");
    opt("--distillminconf", conf.distill_min_confl,
        "Minimum number of conflicts between OTF distill");
    opt("--distillredreleff", conf.distill_red_releff,
        "Per-mille of all bogoprops since last call to spend distilling red cls");
    opt("--distillirredreleff", conf.distill_irred_releff,
        "Per-mille of all bogoprops since last call to spend distilling irred cls");
    opt("--distillmineffm", conf.distill_min_effortM,
        "Floor of the distill effort reference, in mega-bogoprops");
    opt("--distillschedmax", conf.distill_sched_max,
        "Max clauses scheduled per distill pass");
    opt("--distillinst", conf.distill_instantiate,
        "Try to remove the last literal during distillation");
    opt("--xorgatemaxsize", conf.xor_gate_find_maxsize,
        "Largest clause XOR-gate finding considers, before the log2 occurrence cap");
    opt("--distillremlevel", conf.distill_rem_level,
        "Clause removal during distillation. 0 = never, 1 = only on a real conflict, 2 = also when a literal is positively implied");
    opt("--distillirredalsoremratio", conf.distill_irred_alsoremove_ratio,
        "How much of irred to distill when doing also removal");
    opt("--distillirrednoremratio", conf.distill_irred_noremove_ratio,
        "How much of irred to distill when doing no removal");
    ;

    /* po::options_description oracleOptions("Oracle options"); */
    opt("--oraclemult", conf.oracle_mult,
        "Time multiplier for all oracle-based (oracle-vivif*, oracle-sparsify*) cutoffs");
    opt("--oraclegetlearnts", conf.oracle_get_learnts,
        "Keep the clauses the oracle learnt during vivification as redundant clauses");
    opt("--oracleremovedislearnt", conf.oracle_removed_is_learnt,
        "Clauses removed by the oracle are re-added as redundant instead of being deleted");
    opt("--oraclefindbins", conf.oracle_find_bins,
        "[0..] Effort spent looking for binary clauses during oracle vivification. 0 = off");
    ;

    /* po::options_description sweep_opts("SAT sweeping options"); */
    opt("--sweep", conf.do_sweep,
        "Perform SAT sweeping with kitten (occ-sweep)");
    opt("--sweeptimelimM", conf.sweep_time_limitM,
        "Tick limit cap for one occ-sweep run, in millions");
    opt("--sweepeff", conf.sweep_effort,
        "occ-sweep budget as a fraction of all bogoprops since its last call");
    opt("--sweepmineffm", conf.sweep_min_effortM,
        "Floor of the occ-sweep budget, in mega-ticks");
    opt("--sweepvars", conf.sweep_vars,
        "Starting number of variables in a sweeping environment");
    opt("--sweepclauses", conf.sweep_clauses,
        "Starting number of clauses in a sweeping environment");
    opt("--sweepdepth", conf.sweep_depth,
        "Starting depth of a sweeping environment");
    opt("--sweepfliprounds", conf.sweep_flip_rounds,
        "Rounds of model flipping during sweeping");
    ;

    /* po::options_description mem_save_opts("Memory saving options"); */
    opt("--renumber", conf.do_renumber_vars,
        "Renumber variables to increase CPU cache efficiency");
    opt("--mustconsolidate", conf.must_always_conslidate,
        "Always consolidate, even if not useful. This is used for debugging ONLY");
    opt("--savemem", conf.do_save_mem,
        "Save memory by deallocating variable space after renumbering. Only works if renumbering is active.");
    opt("--mustrenumber", conf.must_renumber,
        "Treat all 'renumber' strategies as 'must-renumber'");
    opt("--fullwatchconseveryn", conf.full_watch_consolidate_every_n_confl,
        "Consolidate watchlists fully once every N conflicts. Scheduled during simplification rounds.");

    /* po::options_description miscOptions("Misc options"); */
    opt("--strmaxt", conf.watch_based_str_time_limitM,
        "Maximum MBP to spend on distilling long irred cls through watches");
    opt("--implicitmanip", conf.do_str_sub_implicit,
        "Subsume and strengthen implicit clauses with each other");
    opt("--implsubsto", conf.subsume_implicit_time_limitM,
        "Timeout (in bogoprop Millions) of implicit subsumption");
    opt("--implstrto", conf.distill_implicit_with_implicit_time_limitM,
        "Timeout (in bogoprop Millions) of implicit strengthening");
    opt("--cardfind", conf.do_find_card,
        "Find cardinality constraints");

    /* hiddenOptions.add_options() */
    opt("--sync", conf.sync_every_confl,
        "Sync threads every N conflicts");
    opt("--clearinter", need_clean_exit,
        "Interrupt threads cleanly, all the time");
    program.add_argument("--zero-exit-status")
        .flag()
        .action([&](const auto&) {zero_exit_status = true;})
        .help("Exit with status zero in case the solving has finished without an issue");
    opt("--printtimes", conf.do_print_times,
        "Print time it took for each simplification run. If set to 0, logs are easier to compare");
    opt("--maxsccdepth", conf.max_scc_depth,
        "The maximum for scc search depth");
    program.add_argument("--sampling")
        .help("Set sampling vars such as '1,84,44'. Can also be set via CNF using 'c p show 1 84 44 0'");
    opt("--assump", assump_filename,
        "Assumptions file");

/*     po::options_description gaussOptions("Gauss options"); */
     opt("--maxmatrixrows", conf.gaussconf.max_matrix_rows,
        "Set maximum no. of rows for gaussian matrix. Too large matrices"
            " should be discarded for reasons of efficiency");
     opt("--maxmatrixcols", conf.gaussconf.max_matrix_columns,
        "Set maximum no. of columns for gaussian matrix. Too large matrices"
            " should be discarded for reasons of efficiency");
    opt("--autodisablegauss", conf.gaussconf.autodisable,
        "Automatically disable gauss when performing badly");
    opt("--minmatrixrows", conf.gaussconf.min_matrix_rows,
        "Set minimum no. of rows for gaussian matrix. Normally, too small"
            " matrices are discarded for reasons of efficiency");
    opt("--maxnummatrices", conf.gaussconf.max_num_matrices,
        "Maximum number of matrices to treat.");
    opt("--gaussusefulcutoff", conf.gaussconf.min_usefulness_cutoff,
        "Turn off Gauss if less than this many usefulness ratio is recorded");
    opt("--gaussmincalls", conf.gaussconf.autodisable_min_calls,
        "Only consider disabling a matrix after this many Gauss calls");
    opt("--gausscheckevery", conf.gaussconf.autodisable_check_every,
        "Check whether to disable a matrix every this many conflicts");
    program.add_argument("--dumpresult")
        .action([&](const auto& a) {result_fname = a;})
        .help("Write solution(s) to this file");

    //these a kind of special and determine positional options' meanings
    program.add_argument("files").remaining().help("input file and proof output (XLRUP by default, see --xlrup)");
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
            << "cryptominisat5 [options] inputfile [proof-file]" << endl << endl;

            cout << program << endl;
            cout << "Normal run schedules:" << endl;
            cout << "  Default schedule: "
            << remove_last_comma_if_exists(conf.simplify_schedule_nonstartup) << endl<< endl;
            cout << "  Schedule at startup: "
            << remove_last_comma_if_exists(conf.simplify_schedule_startup) << endl << endl;
            std::exit(0);
        }
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        exit(-1);
    }
}

void Main::parse_polarity_type()
{
    string mode = program.get<string>("polar");
    if (mode == "true") conf.polarity_mode = PolarityMode::polarmode_pos;
    else if (mode == "false") conf.polarity_mode = PolarityMode::polarmode_neg;
    else if (mode == "rnd") conf.polarity_mode = PolarityMode::polarmode_rnd;
    else if (mode == "auto") conf.polarity_mode = PolarityMode::polarmode_automatic;
    else if (mode == "weight") conf.polarity_mode = PolarityMode::polarmode_weighted;
    else throw WrongParam(mode, "unknown polarity-mode");
}

void Main::parse_sampling_vars()
{
    if (!program.is_used("sampling")) return;

    string str_vars = program.get<string>("sampling");
    std::vector<uint32_t> sampl_vars;
    std::stringstream ss(str_vars);
    for (int32_t i; ss >> i;) {
        if (i <= 0) {
           cerr << "Sampling variables must be positive (i.e. larger than 0)" << endl;
           exit(-1);
        }
        sampl_vars.push_back(i-1);
        if (ss.peek() == ',') ss.ignore();
    }
    solver->set_sampl_vars(sampl_vars);
}

void Main::manually_parse_some_options()
{
    #ifndef USE_BREAKID
    if (conf.do_breakid) {
        if (conf.verbosity) cout << "c BreakID not compiled in, disabling" << endl;
        conf.do_breakid = false;
    }
    #endif

    if (conf.walkmineff > conf.walkmaxeff) {
        cout << "ERROR: '--walkmineff' must not be above '--walkmaxeff'" << endl;
        exit(-1);
    }

    if (conf.max_xor_to_find > MAX_XOR_RECOVER_SIZE) {
        cout << "ERROR: The '--maxxorsize' parameter cannot be larger than " << MAX_XOR_RECOVER_SIZE << endl;
        exit(-1);
    }

    if (conf.short_term_history_size <= 0) {
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

    try {
        auto files = program.get<std::vector<std::string>>("files");
        for(const auto& f: files) {
            if (!f.empty() && f[0] == '-') {
                cerr << "ERROR: '" << f << "' after the input file would be the proof file name."
                    " Options must come before the input file" << endl;
                exit(-1);
            }
        }
        if (files.size() > 2) {
            cerr << "ERROR: you can only have at most two files as positional options:"
                "the input file and the output FRAT file" << endl;
            exit(-1);
        }

        if (!files.empty()) {
            input_file = files[0];
#ifdef USE_SQLITE3
            if (!program.is_used("sqlitedb")) sqlite_filename = input_file + ".sqlite";
#endif
            fileNamePresent = true;
        } else assert(false && "The try() should not have succeeded");

        if (files.size() > 1) {
            frat_fname = files[1];
            handle_frat_option();
        }
    } catch (std::logic_error& e) {
        fileNamePresent = false;
    }
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
    check_options_correctness();

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
    if (fratf) {
        if (xlrup_mode) solver->set_xlrup(fratf);
        else solver->set_frat(fratf);
    }
    if (program.is_used("maxtime")) solver->set_max_time(program.get<double>("maxtime"));
    if (program.is_used("maxconfl")) solver->set_max_confl(program.get<uint64_t>("maxconfl"));

    parse_sampling_vars();
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
    readInAssumptions();

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
        && fratf == nullptr
        && debugLib.empty()
    ) {
        solver->set_single_run();
    }

    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    while(current_nr_of_solutions < max_nr_of_solutions && ret == l_True) {
        ret = solver->solve(&assumps, solver->get_sampl_vars_set());
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

            if (!dont_ban_solutions) ban_found_solution();
        }
    }
    return ret;
}

void Main::ban_found_solution() {
    vector<Lit> lits;
    if (!solver->get_sampl_vars_set()) {
        //all of the solution
        for (uint32_t var = 0; var < solver->nVars(); var++) {
            if (solver->get_model()[var] != l_Undef) {
                lits.push_back( Lit(var, (solver->get_model()[var] == l_True)? true : false) );
            }
        }
    } else {
      for (const uint32_t var: solver->get_sampl_vars()) {
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
    cout << "c " << "CMS SHA1: " << solver->get_version_sha1() << endl;
    cout << "c " << "CaDiCaL SHA1: " << solver->get_cadical_version_sha1() << endl;
    cout << "c " << "CadiBack SHA1: " << solver->get_cadiback_version_sha1() << endl;
    cout << "c " << "CryptoMiniSat version " << solver->get_version() << endl;
    cout << "c " << "CMS compilation env " << solver->get_compilation_env() << endl;
    cout << solver->get_thanks_info("c ") << endl;
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
