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

#ifndef SOLVERCONF_H
#define SOLVERCONF_H

#include "solvertypes.h"
#include "constants.h"
#include "clause.h"

class SolverConf
{
    public:
        SolverConf();

        //Variable activities
        uint32_t  var_inc_start;
        uint32_t  var_inc_multiplier;
        uint32_t  var_inc_divider;
        uint32_t  var_inc_variability;
        bool      rarely_bump_var_act;

	//Clause cleaning
        clauseCleaningTypes clauseCleaningType;
        int       doPreClauseCleanPropAndConfl;
        uint32_t  preClauseCleanLimit;
        uint32_t  preCleanMinConflTime;
        int       doClearStatEveryClauseCleaning;
        double    ratioRemoveClauses; ///< Remove this percentage of clauses at every database reduction round
        size_t    numCleanBetweenSimplify; ///<Number of cleaning operations between simplify operations
        size_t    startClean;
        double    increaseClean;
        double    maxNumLearntsRatio; ///<Number of red clauses must not be more than red*maxNumLearntsRatio
        double    clauseDecayActivity;

	//Branching
        double    random_var_freq;    ///<The frequency with which the decision heuristic tries to choose a random variable.        (default 0.02) NOTE: This is really strange. If the number of variables set is large, then the random chance is in fact _far_ lower than this value. This is because the algorithm tries to set one variable randomly, but if that variable is already set, then it _silently_ fails, and moves on (doing non-random flip)!

        //For static restart
        size_t    restart_first;      ///<The initial restart limit.                                                                (default 100)
        double    restart_inc;        ///<The factor with which the restart limit is multiplied in each restart.                    (default 1.5)

        size_t    burstSearchLen;
        RestType  restartType;   ///<If set, the solver will always choose the given restart strategy
        int       doRecursiveCCMin;    ///<Should clause minimisation by Sorensson&Biere be used?                                    (default TRUE)
        int       polarity_mode;      ///<Controls which polarity the decision heuristic chooses. Auto means Jeroslow-Wang          (default: polarity_auto)
        int       verbosity;          ///<Verbosity level. 0=silent, 1=some progress report, 2=lots of report, 3 = all report       (default 2) preferentiality is turned off (i.e. picked randomly between [0, all])

        //Agility
        double    agilityG; ///See paper by Armin Biere on agilities
        double    agilityLimit; ///The agility below which the agility is considered too low
        uint64_t  agilityViolationLimit;

        //Burst?
        int      doPerformPreSimp; //Perform simplification at startup


        //Glues
        int       updateGlues;
        uint32_t  shortTermHistorySize; ///< Rolling avg. glue window size

        //OTF stuff
        int       otfHyperbin;
        int       doOTFSubsume;
        int       doOTFGateShorten; ///<Shorten learnt clauses on the fly with gates
        int       rewardShortenedClauseWithConfl; //Shortened through OTF subsumption

        //SQL
        int       doSQL;
        size_t    dumpTopNVars; //Only dump information about the "top" N active variables
        size_t    dumpClauseDistribPer;
        size_t    dumpClauseDistribMaxSize;
        size_t    dumpClauseDistribMaxGlue;
        size_t    preparedDumpSizeScatter;
        size_t    preparedDumpSizeVarData;
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
        size_t   cacheUpdateCutoff;

        //XORs
        int      doFindXors;         ///<Automatically find non-binary xor clauses and convert them to xor clauses
        int      maxXorToFind;
        int      useCacheWhenFindingXors;
        int      doEchelonizeXOR;
        uint64_t  maxXORMatrix;

        //Var-replacement
        int doFindAndReplaceEqLits;
        int doExtendedSCC;
        double sccFindPercent;


        //Optimisations to do

        int       printFullStats;
        bool      printAllRestarts;
        int       doRenumberVars;
        uint32_t  dominPickFreq;
        uint32_t  flipPolarFreq;
        int      doSchedSimp;        ///<Should simplifyProblem() be scheduled regularly? (if set to FALSE, a lot of opmitisations are disabled)
        int      doSimplify;         ///<Should try to subsume & self-subsuming resolve & variable-eliminate & block-clause eliminate?
        int      doHyperBinRes;      ///<Should try carry out hyper-binary resolution
        int      doLHBR; ///<Do lazy hyper-binary resolution
        int      doBlockClauses;    ///<Should try to remove blocked clauses
        int      doExtBinSubs;
        int      doSubsume1;         ///<Perform self-subsuming resolution
        int      doClausVivif;      ///<Perform asymmetric branching at the beginning of the solving
        int      doSortWatched;      ///<Sort watchlists according to size&type: binary, tertiary, normal (>3-long), xor clauses
        int      doMinimLearntMore;  ///<Perform learnt-clause minimisation using watchists' binary and tertiary clauses? ("strong minimization" in PrecoSat)
        int      doGateFind; ///< Find OR gates
        size_t    maxGateSize;
        int      doAlwaysFMinim; ///< Always try to minimise clause with cache&gates
        int      doER; ///< Perform Extended Resolution (ER)
        int      doCalcReach; ///<Calculate reachability, and influence variable decisions with that
        int      doAsymmTE; ///< Do Asymtotic blocked clause elimination

        int      doShortenWithOrGates; ///<Shorten clauses with or gates during subsumption
        int      doRemClWithAndGates; ///<Remove clauses using and gates during subsumption
        int      doFindEqLitsWithGates; ///<Find equivalent literals using gates during subsumption
        int      doMixXorAndGates; ///<Try to gain knowledge by mixing XORs and gates

        //Verbosity
        int      doPrintGateDot; ///< Print DOT file of gates
        int      doPrintConflDot; ///< Print DOT file for each conflict

        //interrupting & dumping
        bool      needToDumpLearnts;  ///<If set to TRUE, learnt clauses will be dumped to the file speified by "learntsFilename"
        bool      needToDumpSimplified;     ///<If set to TRUE, a simplified version of the original clause-set will be dumped to the file speified by "origFilename". The solution to this file should perfectly satisfy the problem
        std::string learntsDumpFilename;    ///<Dump sorted learnt clauses to this file. Only active if "needToDumpLearnts" is set to TRUE
        std::string simplifiedDumpFilename;       ///<Dump simplified original problem CNF to this file. Only active if "needToDumpOrig" is set to TRUE
        uint32_t  maxDumpLearntsSize; ///<When dumping the learnt clauses, this is the maximum clause size that should be dumped
        bool      libraryUsage;       ///<Set to true if not used as a library. In fact, this is TRUE by default, and Main.cpp sets it to "FALSE". Disables some simplifications at the beginning of solving (mostly performStepsBeforeSolve() )
        bool      greedyUnbound;      ///<If set, then variables will be greedily unbounded (set to l_Undef). This is EXPERIMENTAL

        uint32_t origSeed;
};

#endif //SOLVERCONF_H
