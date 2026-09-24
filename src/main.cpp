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
#include "conf_options.h"

using namespace CMSat;

using std::cout;
using std::cerr;
using std::endl;

Main::Main(int _argc, char** _argv) :
    argc(_argc)
    , argv(_argv)
    , fileNamePresent (false)
{
}

void Main::readInAFile(SATSolver* solver2, const string& filename) {
    std::unique_ptr<FieldGen> fg = std::make_unique<FGenDouble>();
    solver2->add_sql_tag("filename", filename);
    if (conf.verbosity) cout << conf.prefix << "Reading file '" << filename << "'" << endl;
    #ifndef USE_ZLIB
    FILE * in = fopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<FILE*, FN>, SATSolver> parser(solver2, &debugLib, conf.verbosity, fg);
    #else
    gzFile in = gzopen(filename.c_str(), "rb");
    DimacsParser<StreamBuffer<gzFile, GZ>, SATSolver> parser(solver2, &debugLib, conf.verbosity, fg);
    #endif
    parser.prefix = conf.prefix;

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
    if (conf.verbosity) cout << conf.prefix << "Reading from standard input... Use '-h' or '--help' for help." << endl;
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
    parser.prefix = conf.prefix;

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
            << conf.prefix << "Sum parsing time among all threads (wall time will differ): "
            << std::fixed << std::setprecision(2)
            << (cpuTimeTotal() - my_timeTotal)
            << " s" << endl;
        } else {
            cout
            << conf.prefix << "Parsing time: "
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
        cout << conf.prefix << "Not printing satisfying assignment. "
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
                cout << conf.prefix << "NOTE: " << num_undef << " variables are UNDEF. Sampling vars set:"
                    << solver->get_sampl_vars_set() << endl;
            }
        }
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

        cout << conf.prefix << "Assume: " << x << endl;
        assumps.push_back(Lit(std::abs(x)-1, x < 0));
    }
}

/* clang-format off */
void Main::add_supported_options() {
    program.add_argument("--version", "-v")
        .action([&](const auto ) {printVersionInfo(); exit(0);})
        .flag()
        .help("Print version information");
    opt("--xlrup", xlrup_mode,
        "Emit the proof in XLRUP format, checkable directly by cake_xlrup. Set to 0 to emit raw FRAT instead, for debugging proof generation with frat-rs [0..1]")
        .metavar("{0,1}");
    program.add_argument("--maxtime")
        .help("Stop solving after this much time (s)")
        .scan<'g', double>();
    program.add_argument("--maxconfl")
        .help("Stop solving after this many conflicts")
        .scan<'d', uint64_t>();
    opt("-t", "--threads", num_threads,
        "Number of threads");
    opt("--maxsol", max_nr_of_solutions,
        "Search for given amount of solutions. Thanks to Jannis Harder for the decision-based banning idea");
    program.add_argument("--polar")
        .default_value("auto")
        .help("{true,false,rnd,weight,auto} Selects polarity mode. 'true'/'false' -> always branch positive/negative. 'weight' -> random, biased by the per-variable weight. 'auto' -> CaDiCaL's saved/target/best phases with rephasing");
    #ifdef STATS_NEEDED
    program.add_argument("--clid")
        .flag()
        .action([&](const auto&) {clause_ID_needed = true;})
        .help("Add clause IDs to FRAT output");
    #endif
    program.add_argument("--nobansol")
        .flag()
        .action([&](const auto&) {dont_ban_solutions = true;})
        .help("Don't ban the solution once it's found");
    program.add_argument("--debuglib")
        .action([&](const auto& a) {debugLib = a;})
        .help("Parse special comments to run solve/simplify during parsing of CNF");
    #ifdef USE_SQLITE3
    opt("--sql", sql,
        "Write to SQL. 0 = no SQL, 1 or 2 = sqlite");
    opt("--sqlitedb", sqlite_filename,
        "SQLite database filename to write clause data to");
    #endif
    opt("--printsol","-s", printResult,
        "Print assignment if solution is SAT");
    opt("--clearinter", need_clean_exit,
        "Interrupt threads cleanly, all the time");
    program.add_argument("--zero-exit-status")
        .flag()
        .action([&](const auto&) {zero_exit_status = true;})
        .help("Exit with status zero in case the solving has finished without an issue");
    program.add_argument("--sampling")
        .help("Set sampling vars such as '1,84,44'. Can also be set via CNF using 'c p show 1 84 44 0'");
    opt("--assump", assump_filename,
        "Assumptions file");
    program.add_argument("--dumpresult")
        .action([&](const auto& a) {result_fname = a;})
        .help("Write solution(s) to this file");
    for_each_conf_opt(conf, [&](const ConfOpt& o, auto& var) {
        if (o.short_name) opt(o.short_name, o.name, var, o.help);
        else opt(o.name, var, o.help);
    });
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
    conf.polarity_mode = parse_polarity(program.get<string>("polar"));
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
    check_conf(conf);

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
    } catch(std::invalid_argument& e) {
        cerr << "ERROR: " << e.what() << endl;
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
        << conf.prefix << "WARNING: Number of threads requested is more than the number of"
        << " cores reported by the system.\n"
        << conf.prefix << "WARNING: This is not a good idea in general. It's best to set the"
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
        << conf.prefix << "Executed with command line: "
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
        << conf.prefix << "Not finished running -- signal caught or some maximum reached"
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
           << conf.prefix << "Number of solutions found until now: "
           << std::setw(6) << max_nr_of_solutions
           << endl
           << conf.prefix << "maxsol reached"
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
                << conf.prefix << "Number of solutions found until now: "
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
    cout << conf.prefix << "CMS SHA1: " << solver->get_version_sha1() << endl;
    cout << conf.prefix << "CaDiCaL SHA1: " << solver->get_cadical_version_sha1() << endl;
    cout << conf.prefix << "CadiBack SHA1: " << solver->get_cadiback_version_sha1() << endl;
    cout << conf.prefix << "CryptoMiniSat version " << solver->get_version() << endl;
    cout << conf.prefix << "CMS compilation env " << solver->get_compilation_env() << endl;
    cout << solver->get_thanks_info(conf.prefix.c_str()) << endl;
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
