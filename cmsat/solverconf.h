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

#ifndef SOLVERCONF_H
#define SOLVERCONF_H

#include <fstream>
#include "solvertypes.h"
#include "constants.h"
#include "clause.h"

namespace CMSat {

class SolverConf
{
    public:
        SolverConf();

        //Variable activities
        uint32_t  var_inc_start;
        uint32_t  var_inc_multiplier;
        uint32_t  var_inc_divider;
        uint32_t  var_inc_variability;
        /**
         * The frequency with which the decision heuristic tries to choose
         * a random variable.
         * This is really strange. If the number of variables set is large
         * , then the random chance is in fact _far_ lower than this value.
         * This is because the algorithm tries to set one variable randomly,
         * but if that variable is already set, then it _silently_ fails
         * , and moves on (doing non-random flip)!
        **/
        double    random_var_freq;
        /**
         * Controls which polarity the decision heuristic chooses.
        **/
        PolarityMode polarity_mode;

        //Clause cleaning
        ClauseCleaningTypes clauseCleaningType;
        int       doPreClauseCleanPropAndConfl;
        uint32_t  preClauseCleanLimit;
        uint32_t  preCleanMinConflTime;
        int       doClearStatEveryClauseCleaning;
        double    ratioRemoveClauses; ///< Remove this ratio of clauses at every database reduction round
        uint64_t    numCleanBetweenSimplify; ///<Number of cleaning operations between simplify operations
        uint64_t    startClean;
        double    increaseClean;
        double    maxNumRedsRatio; ///<Number of red clauses must not be more than red*maxNumRedsRatio
        double    clauseDecayActivity;

        //For restarting
        uint64_t    restart_first;      ///<The initial restart limit.                                                                (default 100)
        double    restart_inc;        ///<The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
        uint64_t    burstSearchLen;
        Restart  restartType;   ///<If set, the solver will always choose the given restart strategy
        int       optimiseUnsat;

        //Clause minimisation
        int doRecursiveMinim;
        int doMinimRedMore;  ///<Perform learnt clause minimisation using watchists' binary and tertiary clauses? ("strong minimization" in PrecoSat)
        int doAlwaysFMinim; ///< Always try to minimise clause with cache&gates
        uint64_t moreMinimLimit;

        //Verbosity
        int  verbosity;  ///<Verbosity level. 0=silent, 1=some progress report, 2=lots of report, 3 = all report       (default 2) preferentiality is turned off (i.e. picked randomly between [0, all])
        int  doPrintGateDot; ///< Print DOT file of gates
        int  doPrintConflDot; ///< Print DOT file for each conflict
        int  printFullStats;
        int  verbStats;

        //Limits
        double   maxTime;
        uint64_t   maxConfl;

        //Agility
        double    agilityG; ///See paper by Armin Biere on agilities
        double    agilityLimit; ///The agility below which the agility is considered too low
        uint64_t  agilityViolationLimit;

        //Glues
        int       updateGlues;
        uint32_t  shortTermHistorySize; ///< Rolling avg. glue window size

        //OTF stuff
        int       otfHyperbin;
        int       doOTFSubsume;
        int       doOTFGateShorten; ///<Shorten redundant clauses on the fly with gates
        int       rewardShortenedClauseWithConfl; //Shortened through OTF subsumption

        //SQL
        int       doSQL;
        uint64_t    dumpTopNVars; //Only dump information about the "top" N active variables
        uint64_t    dumpClauseDistribPer;
        uint64_t    dumpClauseDistribMaxSize;
        uint64_t    dumpClauseDistribMaxGlue;
        uint64_t    preparedDumpSizeScatter;
        uint64_t    preparedDumpSizeVarData;
        string    sqlServer;
        string    sqlUser;
        string    sqlPass;
        string    sqlDatabase;

        //Var-elim
        int      doVarElim;          ///<Perform variable elimination
        int      updateVarElimComplexityOTF;
        int      varelimStrategy; ///<Guess varelim order, or calculate?
        int      varElimCostEstimateStrategy;
        double    varElimRatioPerIter;

        //Probing
        int      doProbe;
        double   probeMultiplier; //Increase failed lit time by this multiplier
        int      doBothProp;
        int      doTransRed;   ///<Should carry out transitive reduction
        int      doStamp;
        int      doCache;
        uint64_t   cacheUpdateCutoff;
        uint64_t   maxCacheSizeMB;

        //XORs
        int      doFindXors;
        int      maxXorToFind;
        int      useCacheWhenFindingXors;
        int      doEchelonizeXOR;
        uint64_t  maxXORMatrix;

        //Var-replacement
        int doFindAndReplaceEqLits;
        int doExtendedSCC;
        double sccFindPercent;

        //Propagation & searching
        int      doLHBR; ///<Do lazy hyper-binary resolution
        int      propBinFirst;
        uint32_t  dominPickFreq;
        uint32_t  flipPolarFreq;

        //Simplifier
        int      doSimplify;         ///<Should try to subsume & self-subsuming resolve & variable-eliminate & block-clause eliminate?
        int      doSchedSimpProblem;        ///<Should simplifyProblem() be scheduled regularly? (if set to FALSE, a lot of opmitisations are disabled)
        int      doPreSchedSimpProblem;          //Perform simplification at startup
        int      doSubsume1;         ///<Perform self-subsuming resolution
        int      doBlockClauses;    ///<Should try to remove blocked clauses
        int      doAsymmTE; ///< Do Asymtotic blocked clause elimination
        unsigned maxRedLinkInSize;
        uint64_t maxOccurIrredMB;
        uint64_t maxOccurRedMB;
        uint64_t maxOccurRedLitLinkedM;


        //Optimisations to do
        bool      printAllRestarts;
        int       doRenumberVars;
        int       doSaveMem;

        //Component handling
        int       doFindComps;
        int       doCompHandler;
        uint64_t    handlerFromSimpNum;
        uint64_t    compVarLimit;
        uint64_t  compFindLimitMega;


        int      doExtBinSubs;

        int      doClausVivif;      ///<Perform asymmetric branching at the beginning of the solving
        int      doSortWatched;      ///<Sort watchlists according to size&type: binary, tertiary, normal (>3-long), xor clauses
        int      doStrSubImplicit;

        //Gates
        int      doGateFind; ///< Find OR gates
        uint64_t    maxGateSize;
        unsigned maxGateBasedClReduceSize;

        int      doER; ///< Perform Extended Resolution (ER)
        int      doCalcReach; ///<Calculate reachability, and influence variable decisions with that


        int      doShortenWithOrGates; ///<Shorten clauses with or gates during subsumption
        int      doRemClWithAndGates; ///<Remove clauses using and gates during subsumption
        int      doFindEqLitsWithGates; ///<Find equivalent literals using gates during subsumption
        int      doMixXorAndGates; ///<Try to gain knowledge by mixing XORs and gates

        //interrupting & dumping
        bool      needResultFile;     ///<If set to TRUE, result will be written to a file
        std::string resultFilename;    ///<Write result to this file. Only active if "needResultFile" is set to TRUE
        std::string redDumpFname;    ///<Dump sorted redundant clauses to this file. Only active if "needToDumpReds" is set to TRUE
        std::string irredDumpFname;       ///<Dump irred original problem CNF to this file. Only active if "needToDumpOrig" is set to TRUE
        uint32_t  maxDumpRedsSize; ///<When dumping the redundant clauses, this is the maximum clause size that should be dumped

        uint32_t origSeed;
};

} //end namespace

#endif //SOLVERCONF_H
