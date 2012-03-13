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

#include "Main.h"
#include "constants.h"
#include "time_mem.h"
#include "constants.h"
#include "DimacsParser.h"
#include "ThreadControl.h"

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
using boost::lexical_cast;
namespace po = boost::program_options;
using std::cout;
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

ThreadControl* solverToInterrupt;

/**
@brief For correctly and gracefully exiting

It can happen that the user requests a dump of the learnt clauses. In this case,
the program must wait until it gets to a state where the learnt clauses are in
a correct state, then dump these and quit normally. This interrupt hander
is used to achieve this
*/
void SIGINT_handler(int)
{
    ThreadControl* control = solverToInterrupt;
    cout << "c " << endl;
    std::cerr << "*** INTERRUPTED ***" << endl;
    if (control->getNeedToDumpLearnts() || control->getNeedToDumpOrig()) {
        control->setNeedToInterrupt();
        std::cerr << "*** Please wait. We need to interrupt cleanly" << endl;
        std::cerr << "*** This means we might need to finish some calculations" << endl;
    } else {
        if (control->getVerbosity() >= 1)
            control->printStats();
        _exit(1);
    }
}

void Main::readInAFile(const std::string& filename)
{
    if (conf.verbosity >= 1) {
        cout << "c Reading file '" << filename << "'" << endl;
    }
    #ifdef DISABLE_ZLIB
        FILE * in = fopen(filename.c_str(), "rb");
    #else
        gzFile in = gzopen(filename.c_str(), "rb");
    #endif // DISABLE_ZLIB

    if (in == NULL) {
        cout << "ERROR! Could not open file '" << filename << "' for reading" << endl;
        exit(1);
    }

    DimacsParser parser(control, debugLib, debugNewVar);
    parser.parse_DIMACS(in);

    #ifdef DISABLE_ZLIB
        fclose(in);
    #else
        gzclose(in);
    #endif // DISABLE_ZLIB
}

void Main::readInStandardInput()
{
    if (control->getVerbosity()) {
        cout << "c Reading from standard input... Use '-h' or '--help' for help." << endl;
    }
    #ifdef DISABLE_ZLIB
        FILE * in = stdin;
    #else
        gzFile in = gzdopen(fileno(stdin), "rb");
    #endif // DISABLE_ZLIB

    if (in == NULL) {
        cout << "ERROR! Could not open standard input for reading" << endl;
        exit(1);
    }

    DimacsParser parser(control, debugLib, debugNewVar);
    parser.parse_DIMACS(in);

    #ifndef DISABLE_ZLIB
        gzclose(in);
    #endif // DISABLE_ZLIB
}

void Main::parseInAllFiles()
{
    const double myTime = cpuTime();

    //First read normal extra files
    if ((debugLib || debugNewVar) && filesToRead.size() > 0) {
        cout << "debugNewVar and debugLib must both be OFF to parse in extra files" << endl;
        exit(-1);
    }

    for (vector<string>::const_iterator it = filesToRead.begin(), end = filesToRead.end(); it != end; it++) {
        readInAFile(it->c_str());
    }

    if (!fileNamePresent)
        readInStandardInput();

    if (conf.verbosity >= 1) {
        cout << "c Parsing time: "
        << std::fixed << std::setw(5) << std::setprecision(2) << (cpuTime() - myTime)
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
        for (Var var = 0; var != control->nVars(); var++)
            if (control->model[var] != l_Undef)
                toPrint << ((control->model[var] == l_True)? "" : "-") << var+1 << " ";
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
    ("input", po::value< std::vector<std::string> >(), "file(s) to read")
    ("verbosity", po::value<int>(&conf.verbosity)->default_value(conf.verbosity)
        , "[0-4] Verbosity of solver")
    ("randomize", po::value<uint32_t>(&conf.origSeed)->default_value(conf.origSeed)
        , "[0..] Sets random seed")
    ("restart", po::value<std::string>()->default_value("glue")
        , "{geom, glue, agility}  Restart strategy to follow.")
    ("threads,t", po::value<int>(&numThreads)->default_value(1)
        , "Number of threads to use")
    ("solprint", po::value<int>(&printResult)->default_value(printResult)
        , "Print assignment if solution is SAT")
    ("simplify", po::value<int>(&conf.doSchedSimp)->default_value(conf.doSchedSimp)
        , "Perform regular simplification rounds")
    ("nclbtwsimp", po::value<size_t>(&conf.numCleanBetweenSimplify)->default_value(conf.numCleanBetweenSimplify)
        , "Perform this many cleaning iterations between simplification rounds")
    //("greedyunbound", "Greedily unbound variables that are not needed for SAT")
    ;

    po::options_description reduceDBOptions("Learnt clause removal options");
    reduceDBOptions.add_options()
    ("ltclean", po::value<double>(&conf.ratioRemoveClauses)->default_value(conf.ratioRemoveClauses)
        , "Remove at least this ratio of learnt clauses when doing learnt clause-cleaning")
    ("typeclean", po::value<string>(&typeclean)->default_value(getNameOfCleanType(conf.clauseCleaningType))
        , "Metric to use to clean clauses: 'size', 'glue' or 'propconfl' for sum of propagations and conflicts caused in last iteration")
    ("preclean", po::value<int>(&conf.preClauseCleanPropAndConfl)->default_value(conf.preClauseCleanPropAndConfl)
        , "Before cleaning clauses with whatever sorting strategy, remove learnt clauses whose sum of props&conflicts during last iteration is less than 'precleanlimit'")
    ("precleanlimit", po::value<uint32_t>(&conf.preClauseCleanLimit)->default_value(conf.preClauseCleanLimit)
        , "Limit of sum of propagation&conflicts for pre-cleaning of clauses. See previous option")
    ("startClean", po::value<size_t>(&conf.startClean)->default_value(conf.startClean)
        , "Clean first time after this many conflicts")
    ("increaseClean", po::value<double>(&conf.increaseClean)->default_value(conf.increaseClean)
        , "Clean increment cleaning by this factor for next cleaning")
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
        , "{true,false,rnd,auto} Selects polarity mode")
    ("flippolarfreq", po::value<uint32_t>(&conf.flipPolarFreq)->default_value(conf.flipPolarFreq)
        , "How rarely, relative to the branch depth delta history should we flip polarities randomly? A higher value will flip it less often.")
    ("dompickf", po::value<uint32_t>(&conf.dominPickFreq)->default_value(conf.dominPickFreq)
        , "Use dominating literal every once in N when picking decision literal")
    ("flippolf", po::value<uint32_t>(&conf.flipPolarFreq)->default_value(conf.flipPolarFreq)
        , "Flip polarity frequency once every N, multilplied by avg. branch depth delta")
    ;


    po::options_description iterativeOptions("Iterative solve options");
    iterativeOptions.add_options()
    ("maxsolutions", po::value<uint32_t>(&max_nr_of_solutions)->default_value(max_nr_of_solutions)
        , "Search for given amount of solutions")
    ("dumplearnts", po::value<std::string>(&conf.learntsFilename)
        , "If stopped dump learnt clauses here")
    ("maxdump", po::value<uint32_t>(&conf.maxDumpLearntsSize)
        , "Maximum length of learnt clause dumped")
    ("dumporig", po::value<std::string>()
        , "If stopped, dump simplified original problem here")
    ("banfoundsol", po::value<int>(&doBanFoundSolution)->default_value(doBanFoundSolution)
     , "Ban solutions found")
    ("debuglib", po::bool_switch(&debugLib)
        , "Solve at specific 'solve()' points in CNF file")
    ("debugnewvar", po::bool_switch(&debugNewVar)
        , "Add new vars at specific 'newVar()' points in 6CNF file")
    ;

    po::options_description failedLitOptions("Failed lit options");
    failedLitOptions.add_options()
    ("failedlit", po::value<int>(&conf.doFailedLit)->default_value(conf.doFailedLit)
        , "Failed literal probing")
    ("failmultip", po::value<double>(&conf.failedLitMultiplier)->default_value(conf.failedLitMultiplier)
      , "Do this times more/less failed lit than default")
    ("hyperbinres", po::value<int>(&conf.doHyperBinRes)->default_value(conf.doHyperBinRes)
        , "Add binary clauses when doing failed lit probing.")
    ("removebins", po::value<int>(&conf.doRemUselessBins)->default_value(conf.doRemUselessBins)
        , "Remove useless binary clauses (transitive reduction)")
    ;

    po::options_description sateliteOptions("SatELite-type options");
    sateliteOptions.add_options()
    ("satelite", po::value<int>(&conf.doSatELite)->default_value(conf.doSatELite)
        , "Perform play with norm clauses at ALL (none below)")
    ("varelim", po::value<int>(&conf.doVarElim)->default_value(conf.doVarElim)
        , "Perform variable elimination as per Een and Biere")
    ("subsume1", po::value<int>(&conf.doSubsume1)->default_value(conf.doSubsume1)
        , "Don't perform clause contraction through resolution")
    ("block", po::value<int>(&conf.doBlockedClause)->default_value(conf.doBlockedClause)
        , "Do blocked-clause removal")
    ("asymmte", po::value<int>(&conf.doAsymmTE)->default_value(conf.doAsymmTE)
        , "Do asymmetric tautology elimination. See Armin Biere & collaborators' papers")
    ("noextbinsubs", po::value<int>(&conf.doExtBinSubs)->default_value(conf.doExtBinSubs)
        , "No extended subsumption with binary clauses")
    ("eratio", po::value<double>(&conf.varElimRatioPerIter)->default_value(conf.varElimRatioPerIter)
        , "Eliminate this ratio of free variables at most per variable elimination iteration")
    ;

    po::options_description xorSateliteOptions("XOR SatELite-type options");
    xorSateliteOptions.add_options()
    ("xor", po::value<int>(&conf.doFindXors)->default_value(conf.doFindXors)
        , "Discover long XORs")
    ("ehelonxor", po::value<int>(&conf.doEchelonizeXOR)->default_value(conf.doEchelonizeXOR)
        , "Extract data from XORs through echelonization (TOP LEVEL ONLY)")
    ("maxxormat", po::value<uint64_t>(&conf.maxXORMatrix)->default_value(conf.maxXORMatrix)
        , "Maximum matrix size (=num elements) that we should try to echelonize")
    ("mix", po::value<int>(&conf.doMixXorAndGates)->default_value(conf.doMixXorAndGates)
        , "Mix XORs and OrGates for new truths")
    ("binxorfind", po::value<int>(&conf.doFindEqLits)->default_value(conf.doFindEqLits)
        , "Find equivalent literals through SCC")
    ("extendedscc", po::value<int>(&conf.doExtendedSCC)->default_value(conf.doExtendedSCC)
        , "Perform SCC using cache")
    ("varreplace", po::value<int>(&conf.doReplace)->default_value(conf.doReplace)
        , "Perform variable replacement")
    ;

    po::options_description gatesOptions("Gates' options");
    gatesOptions.add_options()
    ("gates", po::value<int>(&conf.doGateFind)->default_value(conf.doGateFind)
        , "Don't find gates. Disable ALL below")
    ("gorshort", po::value<int>(&conf.doShortenWithOrGates)->default_value(conf.doShortenWithOrGates)
        , "Shorten clauses with OR gates")
    ("gandrem", po::value<int>(&conf.doRemClWithAndGates)->default_value(conf.doRemClWithAndGates)
        , "Remove clauses with AND gates")
    ("nogeqlit", po::value<int>(&conf.doFindEqLitsWithGates)->default_value(conf.doFindEqLitsWithGates)
        , "Don't find equivalent literals using gates")
    ("maxgatesz", po::value<size_t>(&conf.maxGateSize)->default_value(conf.maxGateSize)
        , "Maximum gate size to discover")
    ("er", po::value<int>(&conf.doER)->default_value(conf.doER)
        , "Don't find gates to add to do ER")
    ("printgatedot", po::value<int>(&conf.doPrintGateDot)->default_value(conf.doPrintGateDot)
        , "Print gate structure regularly to file 'gatesX.dot'")
    ;

    #ifdef USE_GAUSS
    po::options_description gaussOptions("Gaussian options");
    gaussOptions.add_options()
    ("gaussuntil", po::value<uint32_t>()->default_value(gaussconfig.decision_until), "Cutoff decision depth for gauss. Default IS ZERO, so Gauss is OFF!!!!!!")
    ("nosepmatrix", "Don't separate independent matrixes")
    ("noordercol", "Don't order variables in the columns. Disables iterative matrix reduction")
    ("noiterreduce", "Don't reduce iteratively the matrix")
    ("maxmatrixrows", po::value<uint32_t>()->default_value(gaussconfig.maxMatrixRows), "Max. num of rows for gaussian matrix")
    ("minmatrixrows", po::value<uint32_t>()->default_value(gaussconfig.minMatrixRows), "Min. num of rows for gaussian matrix")
    ("savematrix", po::value<uint32_t>()->default_value(gaussconfig.only_nth_gauss_save), "Leave matrix trail every Nth decision level")
    ("maxnummatrixes", po::value<uint32_t>()->default_value(gaussconfig.maxNumMatrixes), "Max. num matrixes")
    ;
    #endif //USE_GAUSS

    po::options_description conflOptions("Conflict options");
    conflOptions.add_options()
    ("recminim", po::value<int>(&conf.doRecursiveCCMin)->default_value(conf.doRecursiveCCMin)
        , "Perform MiniSat-type conflict-clause minim.")
    ("nolfminim", po::value<int>(&conf.doMinimLearntMore)->default_value(conf.doMinimLearntMore)
        , "Perform strong minimisation at conflict gen.")
    ("alwaysfmin", po::value<int>(&conf.doAlwaysFMinim)->default_value(conf.doAlwaysFMinim)
        , "Always strong-minimise clause")
    ("printimpldot", po::value<int>(&conf.doPrintConflDot)->default_value(conf.doPrintConflDot)
        , "Print implication graph DOT files (for input into graphviz package)")
    ;

    po::options_description miscOptions("Misc options");
    miscOptions.add_options()
    ("presimp", po::value<int>(&conf.doPerformPreSimp)->default_value(conf.doPerformPreSimp)
        , "Perform simplification at startup (turning this OFF can save you time for small instances)")
    //("noparts", "Don't find&solve subproblems with subsolvers")
    ("vivif", po::value<int>(&conf.doClausVivif)->default_value(conf.doClausVivif)
        , "Regularly execute clause vivification")
    ("sortwatched", po::value<int>(&conf.doSortWatched)->default_value(conf.doSortWatched)
        , "Don't sort watches according to size")
    ("calcreach", po::value<int>(&conf.doCalcReach)->default_value(conf.doCalcReach)
        , "Calculate literal reachability")
    ("cache", po::value<int>(&conf.doCache)->default_value(conf.doCache)
        , "Use implication cache. Less memory used, disables LOTS")
    ;

    po::positional_options_description p;
    p.add("input", -1);

    po::variables_map vm;
    po::options_description cmdline_options;
    cmdline_options
    .add(generalOptions)
    .add(reduceDBOptions)
    .add(varPickOptions)
    .add(conflOptions)
    .add(iterativeOptions)
    .add(failedLitOptions)
    .add(sateliteOptions)
    .add(xorSateliteOptions)
    //.add(xorSateliteOptions)
    .add(gatesOptions)
    #ifdef USE_GAUSS
    .add(gaussOptions)
    #endif
    .add(miscOptions)
    ;

    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout
        << "USAGE: " << argv[0] << " [options] <input-files>" << endl
        << " where input is "
        #ifdef DISABLE_ZLIB
        << "plain"
        #else
        << "plain or gzipped"
        #endif // DISABLE_ZLIB
        << " DIMACS." << endl;

        cout << cmdline_options << endl;
        exit(0);
    }

    if (vm.count("polarity")) {
        std::string mode = vm["polarity"].as<std::string>();

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

    if (vm.count("dumporig")) {
        conf.origFilename = vm["dumporig"].as<std::string>();
        conf.needToDumpOrig = true;
    }

    if (vm.count("maxdump")) {
        if (!conf.needToDumpLearnts)
            throw WrongParam("maxdumplearnts", "--dumplearnts=<filename> must be first activated before issuing --maxdumplearnts=<size>");
    }

    if (vm.count("greedyunbound")) {
        conf.greedyUnbound = true;
    }

    if (typeclean == "glue") {
        conf.clauseCleaningType = CLEAN_CLAUSES_GLUE_BASED;
    } else if (typeclean == "size") {
        conf.clauseCleaningType = CLEAN_CLAUSES_SIZE_BASED;
    } else if (typeclean == "propconfl") {
        conf.clauseCleaningType = CLEAN_CLAUSES_PROPCONFL_BASED;
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
        else if (type == "branchd")
            conf.restartType = branch_depth_delta_restart;
        else throw WrongParam("restart", "unknown restart type");
    }

    if (vm.count("printImplDot")) {
        conf.doPrintConflDot = true;
    }

    if (vm.count("nocalcreach")) {
        conf.doCalcReach = false;
    }

    if (vm.count("nocache")) {
        conf.doCache = false;
    }

    if (vm.count("nogates")) {
        conf.doGateFind = false;
    }

    if (numThreads < 1) throw WrongParam("threads", "Num threads must be at least 1");

    if (vm.count("input")) {
        filesToRead = vm["input"].as<std::vector<std::string> >();
        fileNamePresent = true;
    } else {
        fileNamePresent = false;
    }

    if (vm.count("output")) {
        outputFile = vm["output"].as<std::string>();
    }

    if (conf.verbosity >= 1) {
        cout << "c Outputting solution to console" << endl;
    }

    if (!debugLib) conf.libraryUsage = false;
}

void Main::printVersionInfo(const uint32_t verbosity)
{
    if (verbosity >= 1) {
        cout << "c This is CryptoMiniSat " << VERSION << endl;
        #ifdef __GNUC__
        cout << "c compiled with gcc version " << __VERSION__ << endl;
        #else
        cout << "c compiled with non-gcc compiler" << endl;
        #endif
    }
}

int Main::solve()
{
    control = new ThreadControl(conf);
    solverToInterrupt = control;

    printVersionInfo(conf.verbosity);
    parseInAllFiles();

    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    while(current_nr_of_solutions < max_nr_of_solutions && ret == l_True) {
        ret = control->solve();
        current_nr_of_solutions++;

        if (ret == l_True && current_nr_of_solutions < max_nr_of_solutions) {
            if (conf.verbosity >= 1) cout << "c Prepare for next run..." << endl;
            printResultFunc(ret);

            if (doBanFoundSolution) {
                vector<Lit> lits;
                for (Var var = 0; var != control->nVars(); var++) {
                    if (control->model[var] != l_Undef) {
                        lits.push_back( Lit(var, (control->model[var] == l_True)? true : false) );
                    }
                }
                control->addClause(lits);
            }
        }
    }

    /*
    if (conf.needToDumpLearnts) {
        control->dumpSortedLearnts(conf.learntsFilename, conf.maxDumpLearntsSize);
        cout << "c Sorted learnt clauses dumped to file '" << conf.learntsFilename << "'" << endl;
    }
    if (conf.needToDumpOrig) {
        if (ret == l_False && conf.origFilename == "stdout") {
            cout << "p cnf 0 1" << endl;
            cout << "0";
        } else if (ret == l_True && conf.origFilename == "stdout") {
            cout << "p cnf " << control->model.size() << " " << control->model.size() << endl;
            for (uint32_t i = 0; i < control->model.size(); i++) {
                cout << (control->model[i] == l_True ? "" : "-") << i+1 << " 0" << endl;
            }
        } else {
            control->dumpOrigClauses(conf.origFilename);
            if (conf.verbosity >= 1)
                cout << "c Simplified original clauses dumped to file '"
                << conf.origFilename << "'" << endl;
        }
    }*/

    if (ret == l_Undef && conf.verbosity >= 1) {
        cout << "c Not finished running -- signal caught or maximum restart reached" << endl;
    }
    if (conf.verbosity >= 1) control->printStats();

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
        cout << "ERROR! Some option you gave was wrong. Please give '--help' to get help" << endl;
        cout << "Unparsed option: '" << c.get_option_name() << "'" << endl;
        exit(-1);
    } catch (boost::bad_any_cast &e) {
        std::cerr << "ERROR! You probably gave a wrong argument type (Bad cast). " << e.what() << endl;
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
