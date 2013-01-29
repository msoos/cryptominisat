/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

/**
@mainpage CryptoMiniSat
@author Mate Soos, and collaborators

CryptoMiniSat is an award-winning SAT solver based on MiniSat. It brings a
number of benefits relative to MiniSat.
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
#include <signal.h>
#include "constants.h"

#include "main.h"
#include "constants.h"
#include "time_mem.h"
#include "constants.h"
#include "dimacsparser.h"
#include "solver.h"


#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
using boost::lexical_cast;
namespace po = boost::program_options;
using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;

Main::Main(int _argc, char** _argv) :
        debugLib (false)
        , debugNewVar (false)
        , printResult (true)
        , max_nr_of_solutions (1)
        , doBanFoundSolution(true)
        , fileNamePresent (false)
        , argc(_argc)
        , argv(_argv)
{
}

Solver* solverToInterrupt;

/**
@brief For correctly and gracefully exiting

It can happen that the user requests a dump of the learnt clauses. In this case,
the program must wait until it gets to a state where the learnt clauses are in
a correct state, then dump these and quit normally. This interrupt hander
is used to achieve this
*/
void SIGINT_handler(int)
{
    Solver* solver = solverToInterrupt;
    cout << "c " << endl;
    std::cerr << "*** INTERRUPTED ***" << endl;
    if (solver->getNeedToDumpLearnts() || solver->getNeedToDumpSimplified()) {
        solver->setNeedToInterrupt();
        std::cerr
        << "*** Please wait. We need to interrupt cleanly" << endl
        << "*** This means we might need to finish some calculations"
        << endl;
    } else {
        if (solver->getVerbosity() >= 1) {
            solver->addInPartialSolvingStat();
            solver->printFullStats();
        }
        _exit(1);
    }
}

void Main::readInAFile(const std::string& filename)
{
    solver->fileAdded(filename);
    if (conf.verbosity >= 1) {
        cout << "c Reading file '" << filename << "'" << endl;
    }
    #ifndef USE_ZLIB
        FILE * in = fopen(filename.c_str(), "rb");
    #else
        gzFile in = gzopen(filename.c_str(), "rb");
    #endif

    if (in == NULL) {
        cout
        << "ERROR! Could not open file '"
        << filename
        << "' for reading" << endl;

        exit(1);
    }

    DimacsParser parser(solver, debugLib, debugNewVar);
    parser.parse_DIMACS(in);

    #ifndef USE_ZLIB
        fclose(in);
    #else
        gzclose(in);
    #endif
}

void Main::readInStandardInput()
{
    if (solver->getVerbosity()) {
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
        cout << "ERROR! Could not open standard input for reading" << endl;
        exit(1);
    }

    DimacsParser parser(solver, debugLib, debugNewVar);
    parser.parse_DIMACS(in);

    #ifdef USE_ZLIB
        gzclose(in);
    #endif
}

void Main::parseInAllFiles()
{
    const double myTime = cpuTime();

    //First read normal extra files
    if ((debugLib || debugNewVar) && filesToRead.size() > 1) {
        cout
        << "debugNewVar and debugLib must both be OFF"
        << "to parse in more than one file"
        << endl;

        exit(-1);
    }

    for (vector<string>::const_iterator
        it = filesToRead.begin(), end = filesToRead.end(); it != end; it++
    ) {
        readInAFile(it->c_str());
    }

    if (!fileNamePresent)
        readInStandardInput();

    if (conf.verbosity >= 1) {
        cout
        << "c Parsing time: "
        << std::fixed << std::setw(5) << std::setprecision(2)
        << (cpuTime() - myTime)
        << " s" << endl;
    }
}

void Main::printResultFunc(const lbool ret)
{
    if (ret == l_True) {
        if (!printResult) cout << "c SATISFIABLE" << endl;
        else              cout << "s SATISFIABLE" << endl;
    } else if (ret == l_False) {
        if (!printResult) cout << "c UNSATISFIABLE" << endl;
        else              cout << "s UNSATISFIABLE" << endl;
    }

    if(ret == l_True && printResult) {
        std::stringstream toPrint;
        toPrint << "v ";
        for (Var var = 0; var != solver->nVars(); var++)
            if (solver->model[var] != l_Undef)
                toPrint << ((solver->model[var] == l_True)? "" : "-") << var+1 << " ";
            toPrint << "0" << endl;
        cout << toPrint.str();
    }
}

struct WrongParam
{
    WrongParam(std::string _param, std::string _msg) :
        param(_param)
        , msg(_msg)
    {}

    const std::string& getMsg() const
    {
        return msg;
    }

    const std::string& getParam() const
    {
        return param;
    }

    std::string param;
    std::string msg;
};

void Main::parseCommandLine()
{
    conf.verbosity = 2;

    std::string typeclean;

    // Declare the supported options.
    po::options_description generalOptions("Most important options");
    generalOptions.add_options()
    ("help,h", "Prints this help")
    ("version,v", "Print version number")
    ("input", po::value< std::vector<std::string> >(), "file(s) to read")
    ("random,r", po::value<uint32_t>(&conf.origSeed)->default_value(conf.origSeed)
        , "[0..] Sets random seed")
    ("restart", po::value<std::string>()->default_value("glueagility")
        , "{geom, agility, glue, glueagility}  Restart strategy to follow.")
    ("threads,t", po::value<int>(&numThreads)->default_value(1)
        , "Number of threads to use")
    ("simplify", po::value<int>(&conf.doSchedSimp)->default_value(conf.doSchedSimp)
        , "Perform regular simplification rounds")
    ("clbtwsimp", po::value<size_t>(&conf.numCleanBetweenSimplify)->default_value(conf.numCleanBetweenSimplify)
        , "Perform this many cleaning iterations between simplification rounds")
    //("greedyunbound", po::bool_switch(&conf.greedyUnbound)
    //    , "Greedily unbound variables that are not needed for SAT")
    ;

    std::stringstream ssAgilG;
    ssAgilG << std::setprecision(6) << conf.agilityG;

    std::stringstream ssAgilL;
    ssAgilL << std::setprecision(6) << conf.agilityLimit;

    po::options_description restartOptions("Restart options");
    restartOptions.add_options()
    ("agilg", po::value<double>(&conf.agilityG)->default_value(conf.agilityG, ssAgilG.str())
        , "See paper by Armin Biere on agilities")
    ("agillim", po::value<double>(&conf.agilityLimit)->default_value(conf.agilityLimit, ssAgilL.str())
        , "The agility below which the agility is considered too low")
    ("agilviollim", po::value<uint64_t>(&conf.agilityViolationLimit)->default_value(conf.agilityViolationLimit)
        , "Number of agility limit violations over which to demand a restart")
    ("gluehist", po::value<uint32_t>(&conf.shortTermHistorySize)->default_value(conf.shortTermHistorySize)
        , "The size of the moving window for short-term glue history of learnt clauses. If higher, the minimal number of conflicts between restarts is longer")
    ;

    po::options_description reduceDBOptions("Learnt clause removal options");
    reduceDBOptions.add_options()
    ("ltclean", po::value<double>(&conf.ratioRemoveClauses)->default_value(conf.ratioRemoveClauses)
        , "Remove at least this ratio of learnt clauses when doing learnt clause-cleaning")
    ("typeclean", po::value<string>(&typeclean)->default_value(getNameOfCleanType(conf.clauseCleaningType))
        , "Metric to use to clean clauses: 'size', 'glue', 'activity' or 'propconfl' for sum of propagations and conflicts caused in last iteration")
    ("preclean", po::value<int>(&conf.doPreClauseCleanPropAndConfl)->default_value(conf.doPreClauseCleanPropAndConfl)
        , "Before cleaning clauses with whatever sorting strategy, remove learnt clauses whose sum of props&conflicts during last iteration is less than 'precleanlimit'")
    ("precleanlim", po::value<uint32_t>(&conf.preClauseCleanLimit)->default_value(conf.preClauseCleanLimit)
        , "Limit of sum of propagation&conflicts for pre-cleaning of clauses. See previous option")
    ("precleantime", po::value<uint32_t>(&conf.preCleanMinConflTime)->default_value(conf.preCleanMinConflTime)
        , "At least this many conflict must have passed since creation of the clause before preclean can remove it")
    ("clearstat", po::value<int>(&conf.doClearStatEveryClauseCleaning)->default_value(conf.doClearStatEveryClauseCleaning)
        , "Clear clause statistics data of each clause after clause cleaning")
    ("startclean", po::value<size_t>(&conf.startClean)->default_value(conf.startClean)
        , "Clean first time after this many conflicts")
    ("incclean", po::value<double>(&conf.increaseClean)->default_value(conf.increaseClean)
        , "Clean increment cleaning by this factor for next cleaning")
    ("maxredratio", po::value<double>(&conf.maxNumLearntsRatio)->default_value(conf.maxNumLearntsRatio)
        , "Don't ever have more than maxNumLearntsRatio*(irred_clauses) redundant clauses")
    ;

    po::options_description varPickOptions("Variable branching options");
    varPickOptions.add_options()
    ("vincmult", po::value<uint32_t>(&conf.var_inc_multiplier)->default_value(conf.var_inc_multiplier)
        , "variable activity increase multiplier")
    ("vincdiv", po::value<uint32_t>(&conf.var_inc_divider)->default_value(conf.var_inc_divider)
        , "variable activity increase divider (MUST be smaller than multiplier)")
    ("vincvary", po::value<uint32_t>(&conf.var_inc_variability)->default_value(conf.var_inc_variability)
        , "variable activity divider and multiplier are both changed +/- with this amount, randomly, in sync")
    ("vincstart", po::value<uint32_t>(&conf.var_inc_start)->default_value(conf.var_inc_start)
        , "variable activity increase stars with this value. Make sure that this multiplied by multiplier and dividied by divider is larger than itself")
    ("varbumprarely", po::bool_switch(&conf.rarely_bump_var_act)
        , "Only bump variables' activities when they appear in a non-learnt long clause or any 2- or 3-long clause")
    ("freq", po::value<double>(&conf.random_var_freq)->default_value(conf.random_var_freq)
        , "[0 - 1] freq. of picking var at random")
    ("polar", po::value<std::string>()->default_value("auto")
        , "{true,false,rnd,auto} Selects polarity mode. 'true' -> selects only positive polarity when branching. 'false' -> selects only negative polarity when brancing. 'auto' -> selects last polarity used (also called 'caching')")
    ("flippolarfreq", po::value<uint32_t>(&conf.flipPolarFreq)->default_value(conf.flipPolarFreq)
        , "How rarely, relative to the branch depth delta history should we flip polarities randomly? A higher value will flip it less often.")
    ("dompickf", po::value<uint32_t>(&conf.dominPickFreq)->default_value(conf.dominPickFreq)
        , "Use dominating literal every once in N when picking decision literal")
    ("flippolf", po::value<uint32_t>(&conf.flipPolarFreq)->default_value(conf.flipPolarFreq)
        , "Flip polarity frequency once every N, multiplied by avg. branch depth delta")
    ;


    po::options_description iterativeOptions("Iterative solve options");
    iterativeOptions.add_options()
    ("maxsolutions", po::value<uint32_t>(&max_nr_of_solutions)->default_value(max_nr_of_solutions)
        , "Search for given amount of solutions")
    ("dumplearnts", po::value<std::string>(&conf.learntsDumpFilename)
        , "If stopped dump learnt clauses here")
    ("maxdump", po::value<uint32_t>(&conf.maxDumpLearntsSize)
        , "Maximum length of learnt clause dumped")
    ("dumpsimplified", po::value<std::string>()
        , "If stopped, dump simplified original problem here")
    ("banfoundsol", po::value<int>(&doBanFoundSolution)->default_value(doBanFoundSolution)
        , "Ban solutions found")
    ("debuglib", po::bool_switch(&debugLib)
        , "Solve at specific 'solve()' points in CNF file")
    ("debugnewvar", po::bool_switch(&debugNewVar)
        , "Add new vars at specific 'newVar()' points in 6CNF file")
    ;

    po::options_description probeOptions("Probing lit options");
    probeOptions.add_options()
    ("bothprop", po::value<int>(&conf.doBothProp)->default_value(conf.doBothProp)
        , "Do propagations solely to propagate the same value twice")
    ("probe", po::value<int>(&conf.doProbe)->default_value(conf.doProbe)
        , "Carry out probing")
    ("probemultip", po::value<double>(&conf.probeMultiplier)->default_value(conf.probeMultiplier)
      , "Do this times more/less failed lit than default")
    ("hyperbinres", po::value<int>(&conf.doHyperBinRes)->default_value(conf.doHyperBinRes)
        , "Add binary clauses when doing failed lit probing.")
    ("transred", po::value<int>(&conf.doTransRed)->default_value(conf.doTransRed)
        , "Remove useless binary clauses (transitive reduction)")
    ;

    std::stringstream ssERatio;
    ssERatio << std::setprecision(4) << conf.varElimRatioPerIter;

    po::options_description simplificationOptions("Simplification options");
    simplificationOptions.add_options()
    ("varelim", po::value<int>(&conf.doVarElim)->default_value(conf.doVarElim)
        , "Perform variable elimination as per Een and Biere")
    ("elimstrategy", po::value<int>(&conf.varelimStrategy)->default_value(conf.varelimStrategy)
        , "Sort variable elimination order by guessing(0) or by calculation(1)")
    ("elimcomplexupdate", po::value<int>(&conf.updateVarElimComplexityOTF)->default_value(conf.updateVarElimComplexityOTF)
        , "Update estimated elimination complexity on-the-fly while eliminating")
    ("elimcoststrategy", po::value<int>(&conf.varElimCostEstimateStrategy)->default_value(conf.varElimCostEstimateStrategy)
        , "How simple strategy (guessing, above) is calculated. Valid values: [0..1]")
    ("subsume1", po::value<int>(&conf.doSubsume1)->default_value(conf.doSubsume1)
        , "Perform clause contraction through resolution")
    ("block", po::value<int>(&conf.doBlockedClause)->default_value(conf.doBlockedClause)
        , "Do blocked-clause removal")
    ("asymmte", po::value<int>(&conf.doAsymmTE)->default_value(conf.doAsymmTE)
        , "Do asymmetric tautology elimination. See Armin Biere & collaborators' papers")
    ("noextbinsubs", po::value<int>(&conf.doExtBinSubs)->default_value(conf.doExtBinSubs)
        , "No extended subsumption with binary clauses")
    ("eratio", po::value<double>(&conf.varElimRatioPerIter)->default_value(conf.varElimRatioPerIter, ssERatio.str())
        , "Eliminate this ratio of free variables at most per variable elimination iteration")
    ;

    po::options_description xorOptions("XOR-related options");
    xorOptions.add_options()
    ("xor", po::value<int>(&conf.doFindXors)->default_value(conf.doFindXors)
        , "Discover long XORs")
    ("xorcache", po::value<int>(&conf.useCacheWhenFindingXors)->default_value(conf.useCacheWhenFindingXors)
        , "Use cache when finding XORs. Finds a LOT more XORs, but takes a lot more time")
    ("ehelonxor", po::value<int>(&conf.doEchelonizeXOR)->default_value(conf.doEchelonizeXOR)
        , "Extract data from XORs through echelonization (TOP LEVEL ONLY)")
    ("maxxormat", po::value<uint64_t>(&conf.maxXORMatrix)->default_value(conf.maxXORMatrix)
        , "Maximum matrix size (=num elements) that we should try to echelonize")

    //Not implemented yet
    //("mix", po::value<int>(&conf.doMixXorAndGates)->default_value(conf.doMixXorAndGates)
    //    , "Mix XORs and OrGates for new truths")
    ("scc", po::value<int>(&conf.doFindAndReplaceEqLits)->default_value(conf.doFindAndReplaceEqLits)
        , "Find equivalent literals through SCC and replace them")
    ("extendedscc", po::value<int>(&conf.doExtendedSCC)->default_value(conf.doExtendedSCC)
        , "Perform SCC using cache")
    ;

    po::options_description gateOptions("Gate-related options");
    gateOptions.add_options()
    ("gates", po::value<int>(&conf.doGateFind)->default_value(conf.doGateFind)
        , "Find gates. Disables all sub-options below")
    ("gorshort", po::value<int>(&conf.doShortenWithOrGates)->default_value(conf.doShortenWithOrGates)
        , "Shorten clauses with OR gates")
    ("gandrem", po::value<int>(&conf.doRemClWithAndGates)->default_value(conf.doRemClWithAndGates)
        , "Remove clauses with AND gates")
    ("gateeqlit", po::value<int>(&conf.doFindEqLitsWithGates)->default_value(conf.doFindEqLitsWithGates)
        , "Find equivalent literals using gates")
    ("maxgatesz", po::value<size_t>(&conf.maxGateSize)->default_value(conf.maxGateSize)
        , "Maximum gate size to discover")
    ("er", po::value<int>(&conf.doER)->default_value(conf.doER)
        , "Find gates to add, and perform Extended Resolution")
    ("printgatedot", po::value<int>(&conf.doPrintGateDot)->default_value(conf.doPrintGateDot)
        , "Print gate structure regularly to file 'gatesX.dot'")
    ;

    po::options_description conflOptions("Conflict options");
    conflOptions.add_options()
    ("recminim", po::value<int>(&conf.doRecursiveCCMin)->default_value(conf.doRecursiveCCMin)
        , "Perform MiniSat-type conflict-clause minim.")
    ("moreminim", po::value<int>(&conf.doMinimLearntMore)->default_value(conf.doMinimLearntMore)
        , "Perform strong minimisation at conflict gen.")
    ("alwaysmoremin", po::value<int>(&conf.doAlwaysFMinim)->default_value(conf.doAlwaysFMinim)
        , "Always strong-minimise clause")
    ("otfsubsume", po::value<int>(&conf.doOTFSubsume)->default_value(conf.doOTFSubsume)
        , "Perform on-the-fly subsumption")
    ("rewardotfsubsume", po::value<int>(&conf.rewardShortenedClauseWithConfl)
        ->default_value(conf.rewardShortenedClauseWithConfl)
        , "Reward with this many prop&confl a clause that has been shortened with on-the-fly subsumption")
    ("printimpldot", po::value<int>(&conf.doPrintConflDot)->default_value(conf.doPrintConflDot)
        , "Print implication graph DOT files (for input into graphviz package)")
    ;

    po::options_description propOptions("Propagation options");
    propOptions.add_options()
    ("updateglue", po::value<int>(&conf.updateGlues)->default_value(conf.updateGlues)
        , "Update glues while propagating")
    ("lhbr", po::value<int>(&conf.doLHBR)->default_value(conf.doLHBR)
        , "Perform lazy hyper-binary resolution while propagating")
    ("otfhyper", po::value<int>(&conf.otfHyperbin)->default_value(conf.otfHyperbin)
        , "Perform hyper-binary resolution at dec. level 1 after every restart")
    ;

    po::options_description sqlOptions("SQL options");
    sqlOptions.add_options()
    ("sql", po::value<int>(&conf.doSQL)->default_value(conf.doSQL)
        , "Write to SQL")
    ("cldistribper", po::value<size_t>(&conf.dumpClauseDistribPer)->default_value(conf.dumpClauseDistribPer)
        , "Dump learnt clause size distribution every N conflicts")
    ("cldistmaxsize", po::value<size_t>(&conf.dumpClauseDistribMaxSize)->default_value(conf.dumpClauseDistribMaxSize)
        , "Dumped learnt clause size maximum -- longer will be 'truncated' in the statistics output")
    ("cldistmaxglue", po::value<size_t>(&conf.dumpClauseDistribMaxGlue)->default_value(conf.dumpClauseDistribMaxGlue)
        , "Dumped learnt clause glue maximum -- longer will be 'truncated' in the statistics output")
    ("prepstmtscatter", po::value<size_t>(&conf.preparedDumpSizeScatter)->default_value(conf.preparedDumpSizeScatter)
        , "When dumping scatter data, dump by chunks of this size (depends on SQL server, default should be safe)")
    ("topnvars", po::value<size_t>(&conf.dumpTopNVars)->default_value(conf.dumpTopNVars)
        , "At every restart, dump the data about the top N variables")
    ("sqluser", po::value<string>(&conf.sqlUser)->default_value(conf.sqlUser)
        , "SQL user to connect with")
    ("sqlpass", po::value<string>(&conf.sqlPass)->default_value(conf.sqlPass)
        , "SQL user's pass to connect with")
    ("sqldb", po::value<string>(&conf.sqlDatabase)->default_value(conf.sqlDatabase)
        , "SQL database name. Default is used by PHP system, so it's highly recommended")
    ("sqlserver", po::value<string>(&conf.sqlServer)->default_value(conf.sqlServer)
        , "SQL server hostname/IP")
    ;

    po::options_description printOptions("Printing options");
    printOptions.add_options()
    ("verb,w", po::value<int>(&conf.verbosity)->default_value(conf.verbosity)
        , "[0-10] Verbosity of solver. 0 = only solution")
    ("printfull", po::value<int>(&conf.printFullStats)->default_value(conf.printFullStats)
        , "Print more thorough, but different stats")
    ("printoften", po::bool_switch(&conf.printAllRestarts)
        , "Print a stat line for every restart")
    ("printsol,s", po::value<int>(&printResult)->default_value(printResult)
        , "Print assignment if solution is SAT")
    ;

    po::options_description miscOptions("Misc options");
    miscOptions.add_options()
    ("presimp", po::value<int>(&conf.doPerformPreSimp)->default_value(conf.doPerformPreSimp)
        , "Perform simplification at startup (turning this OFF can save you time for small instances)")
    //("noparts", "Don't find&solve subproblems with subsolvers")
    ("vivif", po::value<int>(&conf.doClausVivif)->default_value(conf.doClausVivif)
        , "Regularly execute clause vivification")
    ("sortwatched", po::value<int>(&conf.doSortWatched)->default_value(conf.doSortWatched)
        , "Sort watches according to size")
    ("calcreach", po::value<int>(&conf.doCalcReach)->default_value(conf.doCalcReach)
        , "Calculate literal reachability")
    ("stamp", po::value<int>(&conf.doStamp)->default_value(conf.doStamp)
        , "Use time stamping as per Heule&Jarvisalo&Biere paper")
    ("renumber", po::value<int>(&conf.doRenumberVars)->default_value(conf.doRenumberVars)
        , "Renumber variables to increase cache efficiency")
    ;

    po::positional_options_description p;
    p.add("input", -1);

    po::variables_map vm;
    po::options_description cmdline_options;
    cmdline_options
    .add(generalOptions)
    .add(restartOptions)
    .add(printOptions)
    .add(propOptions)
    .add(reduceDBOptions)
    .add(varPickOptions)
    .add(conflOptions)
    .add(iterativeOptions)
    .add(probeOptions)
    .add(simplificationOptions)
    #ifdef USE_M4RI
    .add(xorOptions)
    #endif

    //Currently unimplemented
    //.add(gateOptions)

    #ifdef USE_GAUSS
    .add(gaussOptions)
    #endif
    #ifdef USE_MYSQL
    .add(sqlOptions)
    #endif
    .add(miscOptions)
    ;

    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
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
        exit(0);
    }

    if (vm.count("version")) {
        printVersionInfo();
        exit(0);
    }


    if (vm.count("polar")) {
        std::string mode = vm["polar"].as<std::string>();

        if (mode == "true") conf.polarity_mode = polarity_true;
        else if (mode == "false") conf.polarity_mode = polarity_false;
        else if (mode == "rnd") conf.polarity_mode = polarity_rnd;
        else if (mode == "auto") conf.polarity_mode = polarity_auto;
        else throw WrongParam(mode, "unknown polarity-mode");
    }

    if (conf.random_var_freq < 0 || conf.random_var_freq > 1) {
        WrongParam(lexical_cast<std::string>(conf.random_var_freq), "Illegal random var frequency ");
    }

    //Conflict
    if (vm.count("dumplearnts")) {
        conf.needToDumpLearnts = true;
    }

    if (vm.count("dumpsimplified")) {
        conf.simplifiedDumpFilename = vm["dumpsimplified"].as<std::string>();
        conf.needToDumpSimplified = true;
    }

    if (vm.count("maxdump")) {
        if (!conf.needToDumpLearnts)
            throw WrongParam("maxdumplearnts", "--dumplearnts=<filename> must be first activated before issuing --maxdumplearnts=<size>");
    }

    if (typeclean == "glue") {
        conf.clauseCleaningType = CLEAN_CLAUSES_GLUE_BASED;
    } else if (typeclean == "size") {
        conf.clauseCleaningType = CLEAN_CLAUSES_SIZE_BASED;
    } else if (typeclean == "propconfl") {
        conf.clauseCleaningType = CLEAN_CLAUSES_PROPCONFL_BASED;
    } else if (typeclean == "activiy") {
        conf.clauseCleaningType = CLEAN_CLAUSES_ACTIVITY_BASED;
    } else {
        std::cerr
        << "ERROR: Cannot parse option given to '--cleantype'. It's '"
        << typeclean << "'" << " but that none of the possiblities listed."
        << endl;

        exit(-1);
    }

    //XOR finding

    #ifdef USE_GAUSS
    if (vm.count("gaussuntil")) {
        gaussconfig.decision_until = vm["gaussuntil"].as<uint32_t>();
    }

    if (vm.count("nodisablegauss")) {
        gaussconfig.dontDisable = true;
    }

    if (vm.count("maxnummatrixes")) {
        gaussconfig.maxNumMatrixes = vm["maxnummatrixes"].as<uint32_t>();
    }

    if (vm.count("nosepmatrix")) {
        gaussconfig.doSeparateMatrixFind = false;
    }

    if (vm.count("noiterreduce")) {
        gaussconfig.iterativeReduce = false;
    }

    if (vm.count("noiterreduce")) {
        gaussconfig.iterativeReduce = false;
    }

    if (vm.count("noordercol")) {
        gaussconfig.orderCols = false;
    }

    if (vm.count("maxmatrixrows")) {
        gaussconfig.maxMatrixRows = vm["maxmatrixrows"].as<uint32_t>();
    }


    if (vm.count("minmatrixrows")) {
        gaussconfig.minMatrixRows = vm["minmatrixrows"].as<uint32_t>();
    }


    if (vm.count("savematrix")) {
        gaussconfig.only_nth_gauss_save = vm["savematrix"].as<uint32_t>();
    }
    #endif //USE_GAUSS

    if (vm.count("restart")) {
        std::string type = vm["restart"].as<std::string>();
        if (type == "geom")
            conf.restartType = geom_restart;
        else if (type == "glue")
            conf.restartType = glue_restart;
        else if (type == "agility")
            conf.restartType = agility_restart;
        else if (type == "glueagility")
            conf.restartType = glue_agility_restart;
        else if (type == "branchd")
            conf.restartType = branch_depth_delta_restart;
        else throw WrongParam("restart", "unknown restart type");
    }

    if (vm.count("nocalcreach")) {
        conf.doCalcReach = false;
    }

    if (numThreads < 1)
        throw WrongParam("threads", "Num threads must be at least 1");

    if (numThreads > 1)
        throw WrongParam("threads", "Currently, more than 1 thread is not supported. Sorry!");


    if (max_nr_of_solutions > 1)
        throw WrongParam("maxsolutions",  "More than one solution is currently not supported. Sorry.");

    if (vm.count("input")) {
        filesToRead = vm["input"].as<std::vector<std::string> >();
        fileNamePresent = true;
    } else {
        fileNamePresent = false;
    }

    if (conf.verbosity >= 1) {
        cout << "c Outputting solution to console" << endl;
    }

    if (!debugLib) conf.libraryUsage = false;
}

void Main::printVersionInfo()
{
    cout << "c CryptoMiniSat version " << Solver::getVersion() << endl;
    #ifdef __GNUC__
    cout << "c compiled with gcc version " << __VERSION__ << endl;
    #else
    cout << "c compiled with non-gcc compiler" << endl;
    #endif
}

int Main::solve()
{
    solver = new Solver(conf);
    solverToInterrupt = solver;

    if (conf.verbosity >= 1)
        printVersionInfo();
    parseInAllFiles();

    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    while(current_nr_of_solutions < max_nr_of_solutions && ret == l_True) {
        ret = solver->solve();
        current_nr_of_solutions++;

        if (ret == l_True && current_nr_of_solutions < max_nr_of_solutions) {
            if (conf.verbosity >= 1) cout << "c Prepare for next run..." << endl;
            printResultFunc(ret);

            if (doBanFoundSolution) {
                vector<Lit> lits;
                for (Var var = 0; var != solver->nVars(); var++) {
                    if (solver->model[var] != l_Undef) {
                        lits.push_back( Lit(var, (solver->model[var] == l_True)? true : false) );
                    }
                }
                solver->addClause(lits);
            }
        }
    }

    solver->dumpIfNeeded();

    if (ret == l_Undef && conf.verbosity >= 1) {
        cout
        << "c Not finished running -- signal caught or some maximum reached"
        << endl;
    }
    if (conf.verbosity >= 1)
        solver->printFullStats();

    printResultFunc(ret);

    return correctReturnValue(ret);
}

int Main::correctReturnValue(const lbool ret) const
{
    int retval = -1;
    if      (ret == l_True)  retval = 10;
    else if (ret == l_False) retval = 20;
    else if (ret == l_Undef) retval = 15;
    else {
        std::cerr << "Something is very wrong, output is neither l_Undef, nor l_False, nor l_True" << endl;
        exit(-1);
    }

    #ifdef NDEBUG
    // (faster than "return", which will invoke the destructor for 'Solver')
    exit(retval);
    #endif
    return retval;
}

int main(int argc, char** argv)
{
    Main main(argc, argv);

    try {
        main.parseCommandLine();
    } catch (boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::program_options::unknown_option> >& c) {
        cout
        << "ERROR! Some option you gave was wrong. Please give '--help' to get help"
        << endl;

        cout
        << "Unparsed option: '" << c.get_option_name()
        << "'"
        << endl;

        exit(-1);
    } catch (boost::bad_any_cast &e) {
        std::cerr
        << "ERROR! You probably gave a wrong argument type (Bad cast): "
        << e.what()
        << endl;

        exit(-1);
    } catch (boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::program_options::invalid_option_value> > what) {
        std::cerr << "ERROR! " << what.what() << endl;
        exit(-1);
    } catch (boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::program_options::multiple_occurrences> >& c) {
        std::cerr << "ERROR! Multiple occurrences of option: " << c.get_option_name() << endl;
        exit(-1);
    } catch (WrongParam& w) {
        std::cerr << "ERROR! Option parameter '" << w.getParam() << "' is wrong" << endl;
        std::cerr << "Specific error message: " << w.getMsg() << endl;
        exit(-1);
    }

    signal(SIGINT, SIGINT_handler);
    //signal(SIGHUP,SIGINT_handler);

    return main.solve();
}
