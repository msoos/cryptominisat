/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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
#include "constants.h"

#include "main.h"
#include "constants.h"
#include "time_mem.h"
#include "constants.h"
#include "dimacsparser.h"
#include "solver.h"


#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
using namespace CMSat;
using boost::lexical_cast;
namespace po = boost::program_options;
using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;

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
        debugLib (false)
        , debugNewVar (false)
        , printResult (true)
        , max_nr_of_solutions (1)
        , fileNamePresent (false)
        , argc(_argc)
        , argv(_argv)
        #ifdef DRUP
        , drupf(NULL)
        #endif
{
}

Solver* solverToInterrupt;

/**
@brief For correctly and gracefully exiting

It can happen that the user requests a dump of the redundant clauses. In this case,
the program must wait until it gets to a state where the redundant clauses are in
a correct state, then dump these and quit normally. This interrupt hander
is used to achieve this
*/
void SIGINT_handler(int)
{
    Solver* solver = solverToInterrupt;
    cout << "c " << endl;
    std::cerr << "*** INTERRUPTED ***" << endl;
    if (solver->getNeedToDumpReds() || solver->getNeedToDumpIrredundant()) {
        solver->setNeedToInterrupt();
        std::cerr
        << "*** Please wait. We need to interrupt cleanly" << endl
        << "*** This means we might need to finish some calculations"
        << endl;
    } else {
        if (solver->nVarsReal() > 0) {
            if (solver->getVerbosity() >= 1) {
                solver->addInPartialSolvingStat();
                if (solver->getVerbosity() >= 1) {
                    solver->printStats();
                }
            }
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
    }

    if (ret == l_True && (printResult || toFile)) {

        if(!toFile) *os << "v ";
        for (Var var = 0; var != solver->nVarsReal(); var++) {
            if (solver->model[var] != l_Undef)
                *os << ((solver->model[var] == l_True)? "" : "-") << var+1 << " ";
        }

        *os << "0" << endl;
    }
}

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

void Main::parseCommandLine()
{
    conf.verbosity = 2;
    conf.verbStats = 1;

    //Reconstruct the command line so we can emit it later if needed
    for(int i = 0; i < argc; i++) {
        commandLine += string(argv[i]);
        if (i+1 < argc) {
            commandLine += " ";
        }
    }

    string typeclean;
    #ifdef DRUP
    string drupfilname;
    int drupExistsCheck = 1;
    #endif

    // Declare the supported options.
    po::options_description generalOptions("Most important options");
    generalOptions.add_options()
    ("help,h", "Prints this help")
    ("version,v", "Print version number")
    ("input", po::value< vector<string> >(), "file(s) to read")
    ("random,r", po::value<uint32_t>(&conf.origSeed)->default_value(conf.origSeed)
        , "[0..] Sets random seed")
    ("threads,t", po::value<int>(&numThreads)->default_value(1)
        , "Number of threads to use")
    ("maxtime", po::value<double>(&conf.maxTime)->default_value(conf.maxTime)
        , "Stop solving after this much time, print stats and exit")
    ("maxconfl", po::value<uint64_t>(&conf.maxConfl)->default_value(conf.maxConfl)
        , "Stop solving after this many conflicts, print stats and exit")
    ("occsimp", po::value<int>(&conf.perform_occur_based_simp)->default_value(conf.perform_occur_based_simp)
        , "Perform occurrence-list-based optimisations (var-elim, subsumption, blocking, etc)")
    ("clbtwsimp", po::value<uint64_t>(&conf.numCleanBetweenSimplify)->default_value(conf.numCleanBetweenSimplify)
        , "Perform this many cleaning iterations between simplification rounds")
    ("recur", po::value<int>(&conf.doRecursiveMinim)->default_value(conf.doRecursiveMinim)
        , "Perform recursive minimisation")
    ("unsat", po::value<int>(&conf.optimiseUnsat)->default_value(conf.optimiseUnsat)
        , "Optimize for UNSAT solving")
    #ifdef DRUP
    ("drup,d", po::value<string>(&drupfilname)
        , "Put DRUP verification information into this file")
    ("drupexistscheck", po::value<int>(&drupExistsCheck)->default_value(drupExistsCheck)
        , "Check if the drup file provided already exists")
    ("drupdebug", po::bool_switch(&drupDebug)
        , "Output DRUP verification into the console. Helpful to see where DRUP fails -- use in conjunction with --verb 20. The --drup option must still be given")
    #endif
    //("greedyunbound", po::bool_switch(&conf.greedyUnbound)
    //    , "Greedily unbound variables that are not needed for SAT")
    ;

    std::ostringstream ssAgilG;
    ssAgilG << std::setprecision(6) << conf.agilityG;

    std::ostringstream ssAgilL;
    ssAgilL << std::setprecision(6) << conf.agilityLimit;

    po::options_description restartOptions("Restart options");
    restartOptions.add_options()
    ("agilg", po::value<double>(&conf.agilityG)->default_value(conf.agilityG, ssAgilG.str())
        , "See paper by Armin Biere on agilities")
    ("restart", po::value<string>()
        , "{geom, agility, glue, glueagility}  Restart strategy to follow.")
    ("agillim", po::value<double>(&conf.agilityLimit)->default_value(conf.agilityLimit, ssAgilL.str())
        , "The agility below which the agility is considered too low")
    ("agilviollim", po::value<uint64_t>(&conf.agilityViolationLimit)->default_value(conf.agilityViolationLimit)
        , "Number of agility limit violations over which to demand a restart")
    ("gluehist", po::value<uint32_t>(&conf.shortTermHistorySize)->default_value(conf.shortTermHistorySize)
        , "The size of the moving window for short-term glue history of redundant clauses. If higher, the minimal number of conflicts between restarts is longer")
    ;

    po::options_description reduceDBOptions("Red clause removal options");
    reduceDBOptions.add_options()
    ("ltclean", po::value<double>(&conf.ratioRemoveClauses)->default_value(conf.ratioRemoveClauses)
        , "Remove at least this ratio of redundant clauses when doing redundant clause-cleaning")
    ("clean", po::value<string>(&typeclean)->default_value(getNameOfCleanType(conf.clauseCleaningType))
        , "Metric to use to clean clauses: 'size', 'glue', 'activity' or 'propconfl' for sum of propagations and conflicts caused in last iteration")
    ("preclean", po::value<int>(&conf.doPreClauseCleanPropAndConfl)->default_value(conf.doPreClauseCleanPropAndConfl)
        , "Before cleaning clauses with whatever sorting strategy, remove redundant clauses whose sum of props&conflicts during last iteration is less than 'precleanlimit'")
    ("precleanlim", po::value<uint32_t>(&conf.preClauseCleanLimit)->default_value(conf.preClauseCleanLimit)
        , "Limit of sum of propagation&conflicts for pre-cleaning of clauses. See previous option")
    ("precleantime", po::value<uint32_t>(&conf.preCleanMinConflTime)->default_value(conf.preCleanMinConflTime)
        , "At least this many conflict must have passed since creation of the clause before preclean can remove it")
    ("clearstat", po::value<int>(&conf.doClearStatEveryClauseCleaning)->default_value(conf.doClearStatEveryClauseCleaning)
        , "Clear clause statistics data of each clause after clause cleaning")
    ("startclean", po::value<uint64_t>(&conf.startClean)->default_value(conf.startClean)
        , "Clean first time after this many conflicts")
    ("incclean", po::value<double>(&conf.increaseClean)->default_value(conf.increaseClean)
        , "Clean increment cleaning by this factor for next cleaning")
    ("maxredratio", po::value<double>(&conf.maxNumRedsRatio)->default_value(conf.maxNumRedsRatio)
        , "Don't ever have more than maxNumRedsRatio*(irred_clauses) redundant clauses")
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
    ("freq", po::value<double>(&conf.random_var_freq)->default_value(conf.random_var_freq)
        , "[0 - 1] freq. of picking var at random")
    ("polar", po::value<string>()->default_value("auto")
        , "{true,false,rnd,auto} Selects polarity mode. 'true' -> selects only positive polarity when branching. 'false' -> selects only negative polarity when brancing. 'auto' -> selects last polarity used (also called 'caching')")
    ("dompickf", po::value<uint32_t>(&conf.dominPickFreq)->default_value(conf.dominPickFreq)
        , "Use dominating literal every once in N when picking decision literal")
    ("flippolf", po::value<uint32_t>(&conf.flipPolarFreq)->default_value(conf.flipPolarFreq)
        , "Flip polarity frequency once every N, multiplied by avg. branch depth delta")
    ;


    po::options_description iterativeOptions("Iterative solve options");
    iterativeOptions.add_options()
    ("maxsol", po::value<uint32_t>(&max_nr_of_solutions)->default_value(max_nr_of_solutions)
        , "Search for given amount of solutions")
    ("dumpred", po::value<string>(&conf.redDumpFname)
        , "If stopped dump redundant clauses here")
    ("maxdump", po::value<uint32_t>(&conf.maxDumpRedsSize)
        , "Maximum length of redundant clause dumped")
    ("dumpirred", po::value<string>(&conf.irredDumpFname)
        , "If stopped, dump irred original problem here")
    ("debuglib", po::bool_switch(&debugLib)
        , "Solve at specific 'solve()' points in CNF file")
    ("debugnewvar", po::bool_switch(&debugNewVar)
        , "Add new vars at specific 'newVar()' points in 6CNF file")
    ("dumpresult", po::value<std::string>(&conf.resultFilename)
        , "Write result(s) to this file")
    ;

    po::options_description probeOptions("Probing options");
    probeOptions.add_options()
    ("bothprop", po::value<int>(&conf.doBothProp)->default_value(conf.doBothProp)
        , "Do propagations solely to propagate the same value twice")
    ("probe", po::value<int>(&conf.doProbe)->default_value(conf.doProbe)
        , "Carry out probing")
    ("probemultip", po::value<double>(&conf.probeMultiplier)->default_value(conf.probeMultiplier)
      , "Do this times more/less failed lit than default")
    ("transred", po::value<int>(&conf.doTransRed)->default_value(conf.doTransRed)
        , "Remove useless binary clauses (transitive reduction)")
    ;

    std::ostringstream ssERatio;
    ssERatio << std::setprecision(4) << conf.varElimRatioPerIter;

    po::options_description simplificationOptions("Simplification options");
    simplificationOptions.add_options()
    ("schedsimp", po::value<int>(&conf.regularly_simplify_problem)->default_value(conf.regularly_simplify_problem)
        , "Perform regular simplification rounds")
    ("presimp", po::value<int>(&conf.simplify_at_startup)->default_value(conf.simplify_at_startup)
        , "Perform simplification at the very start")
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
    ("block", po::value<int>(&conf.doBlockClauses)->default_value(conf.doBlockClauses)
        , "Do blocked-clause removal")
    ("asymmte", po::value<int>(&conf.doAsymmTE)->default_value(conf.doAsymmTE)
        , "Do asymmetric tautology elimination. See Armin Biere & collaborators' papers")
    ("noextbinsubs", po::value<int>(&conf.doExtBinSubs)->default_value(conf.doExtBinSubs)
        , "No extended subsumption with binary clauses")
    ("eratio", po::value<double>(&conf.varElimRatioPerIter)->default_value(conf.varElimRatioPerIter, ssERatio.str())
        , "Eliminate this ratio of free variables at most per variable elimination iteration")
    ("occredmax", po::value<unsigned>(&conf.maxRedLinkInSize)->default_value(conf.maxRedLinkInSize)
        , "Don't add to occur list any redundant clause larger than this")
    ;

    std::ostringstream sccFindPercent;
    sccFindPercent << std::fixed << std::setprecision(3) << conf.sccFindPercent;

    po::options_description xorOptions("XOR-related options");
    xorOptions.add_options()
    ("xor", po::value<int>(&conf.doFindXors)->default_value(conf.doFindXors)
        , "Discover long XORs")
    ("xorcache", po::value<int>(&conf.useCacheWhenFindingXors)->default_value(conf.useCacheWhenFindingXors)
        , "Use cache when finding XORs. Finds a LOT more XORs, but takes a lot more time")
    ("echelonxor", po::value<int>(&conf.doEchelonizeXOR)->default_value(conf.doEchelonizeXOR)
        , "Extract data from XORs through echelonization (TOP LEVEL ONLY)")
    ("maxxormat", po::value<uint64_t>(&conf.maxXORMatrix)->default_value(conf.maxXORMatrix)
        , "Maximum matrix size (=num elements) that we should try to echelonize")
    //Not implemented yet
    //("mix", po::value<int>(&conf.doMixXorAndGates)->default_value(conf.doMixXorAndGates)
    //    , "Mix XORs and OrGates for new truths")
    ;

    po::options_description eqLitOpts("Equivalent literal options");
    eqLitOpts.add_options()
    ("scc", po::value<int>(&conf.doFindAndReplaceEqLits)->default_value(conf.doFindAndReplaceEqLits)
        , "Find equivalent literals through SCC and replace them")
    ("extscc", po::value<int>(&conf.doExtendedSCC)->default_value(conf.doExtendedSCC)
        , "Perform SCC using cache")
    ("sccperc", po::value<double>(&conf.sccFindPercent)->default_value(conf.sccFindPercent, sccFindPercent.str())
        , "Perform SCC only if the number of new binary clauses is at least this many % of the number of free variables")
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
    ("maxgatesz", po::value<uint64_t>(&conf.maxGateSize)->default_value(conf.maxGateSize)
        , "Maximum gate size to discover")
    ("er", po::value<int>(&conf.doER)->default_value(conf.doER)
        , "Find gates to add, and perform Extended Resolution")
    ("printgatedot", po::value<int>(&conf.doPrintGateDot)->default_value(conf.doPrintGateDot)
        , "Print gate structure regularly to file 'gatesX.dot'")
    ;

    po::options_description conflOptions("Conflict options");
    conflOptions.add_options()
    ("moreminim", po::value<int>(&conf.doMinimRedMore)->default_value(conf.doMinimRedMore)
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
    ("binpri", po::value<int>(&conf.propBinFirst)->default_value(conf.propBinFirst)
        , "Propagated binary clauses strictly first")
    ("otfhyper", po::value<int>(&conf.otfHyperbin)->default_value(conf.otfHyperbin)
        , "Perform hyper-binary resolution at dec. level 1 after every restart and during probing")
    ;


    po::options_description stampOptions("Stamping options");
    stampOptions.add_options()
    ("stamp", po::value<int>(&conf.doStamp)->default_value(conf.doStamp)
        , "Use time stamping as per Heule&Jarvisalo&Biere paper")
    ("cache", po::value<int>(&conf.doCache)->default_value(conf.doCache)
        , "Use implication cache -- may use a lot of memory")
    ("cachesize", po::value<uint64_t>(&conf.maxCacheSizeMB)->default_value(conf.maxCacheSizeMB)
        , "Maximum size of the implication cache in MB. It may temporarily reach higher usage, but will be deleted&disabled if this limit is reached.")
    ("calcreach", po::value<int>(&conf.doCalcReach)->default_value(conf.doCalcReach)
        , "Calculate literal reachability")
    ("cachecutoff", po::value<uint64_t>(&conf.cacheUpdateCutoff)->default_value(conf.cacheUpdateCutoff)
        , "If the number of literals propagated by a literal is more than this, it's not included into the implication cache")
    ;

    po::options_description sqlOptions("SQL options");
    sqlOptions.add_options()
    ("sql", po::value<int>(&conf.doSQL)->default_value(conf.doSQL)
        , "Write to SQL")
    ("cldistribper", po::value<uint64_t>(&conf.dumpClauseDistribPer)->default_value(conf.dumpClauseDistribPer)
        , "Dump redundant clause size distribution every N conflicts")
    ("cldistmaxsize", po::value<uint64_t>(&conf.dumpClauseDistribMaxSize)->default_value(conf.dumpClauseDistribMaxSize)
        , "Dumped redundant clause size maximum -- longer will be 'truncated' in the statistics output")
    ("cldistmaxglue", po::value<uint64_t>(&conf.dumpClauseDistribMaxGlue)->default_value(conf.dumpClauseDistribMaxGlue)
        , "Dumped redundant clause glue maximum -- longer will be 'truncated' in the statistics output")
    ("prepstmtscatter", po::value<uint64_t>(&conf.preparedDumpSizeScatter)->default_value(conf.preparedDumpSizeScatter)
        , "When dumping scatter data, dump by chunks of this size (depends on SQL server, default should be safe)")
    ("topnvars", po::value<uint64_t>(&conf.dumpTopNVars)->default_value(conf.dumpTopNVars)
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
    ("verb", po::value<int>(&conf.verbosity)->default_value(conf.verbosity)
        , "[0-10] Verbosity of solver. 0 = only solution")
    ("verbstat", po::value<int>(&conf.verbStats)->default_value(conf.verbStats)
        , "Turns off verbose stats if needed")
    ("printfull", po::value<int>(&conf.printFullStats)->default_value(conf.printFullStats)
        , "Print more thorough, but different stats")
    ("printoften", po::value<int>(&conf.printAllRestarts)->default_value(conf.printAllRestarts)
        , "Print a stat line for every restart")
    ("printsol,s", po::value<int>(&printResult)->default_value(printResult)
        , "Print assignment if solution is SAT")
    ("printtrail", po::value<int>(&conf.doPrintLongestTrail)->default_value(conf.doPrintLongestTrail)
        , "Print longest decision trail of the last N conflicts. Value '0' means never print it")
    ("printbest", po::value<int>(&conf.doPrintBestRedClauses)->default_value(conf.doPrintBestRedClauses)
        , "Print the best N irredundant longer-than-3 learnt clauses. Value '0' means not to print anything.")
    ;

    po::options_description miscOptions("Misc options");
    miscOptions.add_options()
    //("noparts", "Don't find&solve subproblems with subsolvers")
    ("vivif", po::value<int>(&conf.doClausVivif)->default_value(conf.doClausVivif)
        , "Regularly execute clause vivification")
    ("sortwatched", po::value<int>(&conf.doSortWatched)->default_value(conf.doSortWatched)
        , "Sort watches according to size")
    ("renumber", po::value<int>(&conf.doRenumberVars)->default_value(conf.doRenumberVars)
        , "Renumber variables to increase CPU cache efficiency")
    ("savemem", po::value<int>(&conf.doSaveMem)->default_value(conf.doSaveMem)
        , "Save memory by deallocating variable space after renumbering. Only works if renumbering is active.")
    ("implicitmanip", po::value<int>(&conf.doStrSubImplicit)->default_value(conf.doStrSubImplicit)
        , "Subsume and strengthen implicit clauses with each other")
    ;

    po::options_description componentOptions("Component options");
    componentOptions.add_options()
    ("findcomp", po::value<int>(&conf.doFindComps)->default_value(conf.doFindComps)
        , "Find components, but do not treat them")
    ("comps", po::value<int>(&conf.doCompHandler)->default_value(conf.doCompHandler)
        , "Perform component-finding and separate handling")
    ("compsfrom", po::value<uint64_t>(&conf.handlerFromSimpNum)->default_value(conf.handlerFromSimpNum)
        , "Component finding only after this many simplification rounds")
    ("compsvar", po::value<uint64_t>(&conf.compVarLimit)->default_value(conf.compVarLimit)
        , "Only use components in case the number of variables is below this limit")
    ("compslimit", po::value<uint64_t>(&conf.compFindLimitMega)->default_value(conf.compFindLimitMega)
        , "Limit how much time is spent in component-finding");

    po::positional_options_description p;
    p.add("input", 1);
    #ifdef DRUP
    p.add("drup", 1);
    #endif

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
    .add(stampOptions)
    .add(simplificationOptions)
    .add(eqLitOpts)
    .add(componentOptions)
    #ifdef USE_M4RI
    .add(xorOptions)
    #endif
    .add(gateOptions)
    #ifdef USE_GAUSS
    .add(gaussOptions)
    #endif
    #ifdef USE_MYSQL
    .add(sqlOptions)
    #endif
    .add(miscOptions)
    ;

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
            exit(0);
        }

        po::notify(vm);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::unknown_option> >& c
    ) {
        cout << "Some option you gave was wrong. Please give '--help' to get help" << endl;
        cout << "Unkown option: " << c.what() << endl;
        exit(-1);
    } catch (boost::bad_any_cast &e) {
        std::cerr
        << "ERROR! You probably gave a wrong argument type (Bad cast): "
        << e.what()
        << endl;

        exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::invalid_option_value> > what
    ) {
        cerr
        << "Invalid value '" << what.what() << "'"
        << " given to option '" << what.get_option_name() << "'"
        << endl;

        exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::multiple_occurrences> > what
    ) {
        cerr
        << "Error: " << what.what() << " of option '"
        << what.get_option_name() << "'"
        << endl;

        exit(-1);
    } catch (boost::exception_detail::clone_impl<
        boost::exception_detail::error_info_injector<po::required_option> > what
    ) {
        cerr
        << "You forgot to give a required option '"
        << what.get_option_name() << "'"
        << endl;

        exit(-1);
    }

    if (conf.doLHBR
        && !conf.propBinFirst
    ) {
        cout
        << "You must NOT set both LHBR and any-order propagation." << endl
        << "LHBR needs binary clauses propagated first."
        << endl;

        exit(-1);
    }

    if (conf.shortTermHistorySize <= 0) {
        cout
        << "You MUST give a short term history size (\"--gluehist\")" << endl
        << "  greater than 0!"
        << endl;

        exit(-1);
    }

    if (vm.count("dumpresult")) {
        conf.needResultFile = true;
    }

    if (vm.count("version")) {
        printVersionInfo();
        exit(0);
    }

    if (vm.count("polar")) {
        string mode = vm["polar"].as<string>();

        if (mode == "true") conf.polarity_mode = PolarityMode::pos;
        else if (mode == "false") conf.polarity_mode = PolarityMode::neg;
        else if (mode == "rnd") conf.polarity_mode = PolarityMode::rnd;
        else if (mode == "auto") conf.polarity_mode = PolarityMode::automatic;
        else throw WrongParam(mode, "unknown polarity-mode");
    }

    if (conf.random_var_freq < 0 || conf.random_var_freq > 1) {
        WrongParam(lexical_cast<string>(conf.random_var_freq), "Illegal random var frequency ");
    }

    //Conflict
    if (vm.count("maxdump") && conf.redDumpFname.empty()) {
        throw WrongParam("maxdumpredss", "--dumpreds=<filename> must be first activated before issuing --maxdumpreds=<size>");
    }

    if (typeclean == "glue") {
        conf.clauseCleaningType = CLEAN_CLAUSES_GLUE_BASED;
    } else if (typeclean == "size") {
        conf.clauseCleaningType = CLEAN_CLAUSES_SIZE_BASED;
    } else if (typeclean == "propconfl") {
        conf.clauseCleaningType = CLEAN_CLAUSES_PROPCONFL_BASED;
    } else if (typeclean == "activity") {
        conf.clauseCleaningType = CLEAN_CLAUSES_ACTIVITY_BASED;
    } else {
        std::cerr
        << "ERROR: Cannot parse option given to '--clean'. It's '"
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
        string type = vm["restart"].as<string>();
        if (type == "geom")
            conf.restartType = Restart::geom;
        else if (type == "glue")
            conf.restartType = Restart::glue;
        else if (type == "agility")
            conf.restartType = Restart::agility;
        else if (type == "glueagility")
            conf.restartType = Restart::glue_agility;
        else throw WrongParam("restart", "unknown restart type");
    }

    if (numThreads < 1)
        throw WrongParam("threads", "Num threads must be at least 1");

    if (numThreads > 1)
        throw WrongParam("threads", "Currently, more than 1 thread is not supported. Sorry!");


    //If the number of solutions requested is more than 1, we need to disable blocking
    if (max_nr_of_solutions > 1) {
        conf.doBlockClauses = false;
        if (conf.verbosity >= 1) {
            cout
            << "c Blocking disabled because multiple solutions are needed"
            << endl;
        }
    }

    if (vm.count("input")) {
        filesToRead = vm["input"].as<vector<string> >();
        fileNamePresent = true;
    } else {
        fileNamePresent = false;
    }

    #ifdef DRUP
    if (vm.count("drup")) {
        if (drupDebug) {
            drupf = &std::cout;
        } else {
            if (drupExistsCheck && fileExists(drupfilname)) {
                cout
                << "ERROR! File selected for DRUP output, '"
                << drupfilname
                << "' already exists. Please delete the file or pick another"
                << endl
                << "DRUP filename"
                << endl;
                exit(-1);
            }
            std::ofstream* drupfTmp = new std::ofstream;
            drupfTmp->open(drupfilname.c_str(), std::ofstream::out);
            if (!*drupfTmp) {
                cout
                << "ERROR: Could not open DRUP file "
                << drupfilname
                << " for writing"
                << endl;

                exit(-1);
            }
            drupf = drupfTmp;
        }
    }

    if (!conf.otfHyperbin && drupf) {
        if (conf.verbosity >= 2) {
            cout
            << "c OTF hyper-bin is needed for BProp, turning it back"
            << endl;
        }
        conf.otfHyperbin = true;
    }

    if (conf.doFindXors && drupf) {
        if (conf.verbosity >= 2) {
            cout
            << "c XOR manipulation is not supported in DRUP, turning it off"
            << endl;
        }
        conf.doFindXors = false;
    }

    if (conf.doRenumberVars && drupf) {
        if (conf.verbosity >= 2) {
            cout
            << "c Variable renumbering is not supported during DRUP, turning it off"
            << endl;
        }
        conf.doRenumberVars = false;
    }

    if (conf.doCompHandler && drupf) {
        if (conf.verbosity >= 2) {
            cout
            << "c Component finding & solving is not supported during DRUP, turning it off"
            << endl;
        }
        conf.doCompHandler = false;
    }
    #endif

    if (conf.verbosity >= 1) {
        cout << "c Outputting solution to console" << endl;
    }

    if (debugLib) {
        //In case we are debugging the library, blocking must be disabled
        conf.doBlockClauses = false;
    }
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
    #ifdef DRUP
    solver->drup.setFile(drupf);
    #endif

    std::ofstream resultfile;

    //For dumping result into file
    if (conf.needResultFile) {
        resultfile.open(conf.resultFilename.c_str());
        if (!resultfile) {
            cout
            << "ERROR: Couldn't open file '"
            << conf.resultFilename
            << "' for writing!"
            << endl;
            exit(-1);
        }
    }

    //Print command line used to execute the solver: for options and inputs
    if (conf.verbosity >= 1) {
        printVersionInfo();
        cout
        << "c Executed with command line:"
        << commandLine
        << endl;
    }

    //Parse in DIMACS (maybe gzipped) files
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
            if (conf.needResultFile) {
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
            for (Var var = 0; var < solver->nVarsReal(); var++) {
                if (solver->model[var] != l_Undef) {
                    lits.push_back( Lit(var, (solver->model[var] == l_True)? true : false) );
                }
            }
            solver->addClause(lits);
        }
    }

    solver->dumpIfNeeded();

    if (ret == l_Undef && conf.verbosity >= 1) {
        cout
        << "c Not finished running -- signal caught or some maximum reached"
        << endl;
    }
    if (conf.verbosity >= 1) {
        solver->printStats();
    }

    //Final print of solution
    printResultFunc(&cout, false, ret, current_nr_of_solutions == 1);
    if (conf.needResultFile) {
        printResultFunc(&resultfile, true, ret, current_nr_of_solutions == 1);
    }

    //Delete solver
    delete solver;
    solver = NULL;

    #ifdef DRUP
    if (drupf) {
        //flush DRUP
        *drupf << std::flush;

        //If it's not stdout, we have to delete the ofstream
        if (drupf != &std::cout) {
            delete drupf;
        }
    }
    #endif

    return correctReturnValue(ret);
}

int Main::correctReturnValue(const lbool ret) const
{
    int retval = -1;
    if      (ret == l_True)  retval = 10;
    else if (ret == l_False) retval = 20;
    else if (ret == l_Undef) retval = 15;
    else {
        cerr
        << "Something is very wrong, output is neither l_Undef, nor l_False, nor l_True"
        << endl;

        exit(-1);
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
