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

#include "SolverTypes.h"
#include "constants.h"

class SolverConf
{
    public:
        SolverConf();

        //Most important parameters
        uint32_t  var_inc_start;
        uint32_t  var_inc_multiplier;
        uint32_t  var_inc_divider;
        double    ratioRemoveClauses; ///< Remove this percentage of clauses at every database reduction round
        double    random_var_freq;    ///<The frequency with which the decision heuristic tries to choose a random variable.        (default 0.02) NOTE: This is really strange. If the number of variables set is large, then the random chance is in fact _far_ lower than this value. This is because the algorithm tries to set one variable randomly, but if that variable is already set, then it _silently_ fails, and moves on (doing non-random flip)!
        size_t    restart_first;      ///<The initial restart limit.                                                                (default 100)
        double    restart_inc;        ///<The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
        size_t    burstSearchLen;
        RestType  restartType;   ///<If set, the solver will always choose the given restart strategy
        double    learntsize_factor;  ///<The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
        bool      expensive_ccmin;    ///<Should clause minimisation by Sorensson&Biere be used?                                    (default TRUE)
        int       polarity_mode;      ///<Controls which polarity the decision heuristic chooses. Auto means Jeroslow-Wang          (default: polarity_auto)
        int       verbosity;          ///<Verbosity level. 0=silent, 1=some progress report, 2=lots of report, 3 = all report       (default 2) preferentiality is turned off (i.e. picked randomly between [0, all])

        //Agility
        double    agilityG; ///See paper by Armin Biere on agilities
        double    agilityLimit; ///The agility below which the agility is considered too low

        //Burst?
        uint32_t  simpBurstSConf;
        uint64_t  maxConflBtwSimp;
        bool      doPerformPreSimp;
        double    failedLitMultiplier;
        uint32_t  nbClBeforeRedStart; ///< Start number of learnt clauses before learnt-clause cleaning

        //Glues
        uint32_t  shortTermGlueHistorySize; ///< Rolling avg. glue window size

        //Optimisations to do
        bool      doFindXors;         ///<Automatically find non-binary xor clauses and convert them to xor clauses
        bool      doFindEqLits;       ///<Automatically find binary xor clauses (i.e. variable equi- and antivalences)
        bool      doReplace;          ///<Should var-replacing be performed? If set to FALSE, equi- and antivalent variables will not be replaced with one another. NOTE: This precludes using a lot of the algorithms!
        bool      doConglXors;        ///<Do variable elimination at the XOR-level (xor-ing 2 xor clauses thereby removing a variable)
        bool      doSchedSimp;        ///<Should simplifyProblem() be scheduled regularly? (if set to FALSE, a lot of opmitisations are disabled)
        bool      doSatELite;         ///<Should try to subsume & self-subsuming resolve & variable-eliminate & block-clause eliminate?
        bool      doXorSatelite;      ///<Should try to subsume & local-substitute xor clauses
        bool      doHyperBinRes;      ///<Should try carry out hyper-binary resolution
        bool      doBlockedClause;    ///<Should try to remove blocked clauses
        bool      doExtBinSubs;
        bool      doVarElim;          ///<Perform variable elimination
        bool      doSubsume1;         ///<Perform self-subsuming resolution
        bool      doClausVivif;      ///<Perform asymmetric branching at the beginning of the solving
        bool      doSortWatched;      ///<Sort watchlists according to size&type: binary, tertiary, normal (>3-long), xor clauses
        bool      doMinimLearntMore;  ///<Perform learnt-clause minimisation using watchists' binary and tertiary clauses? ("strong minimization" in PrecoSat)
        bool      doFailedLit;        ///<Carry out Failed literal probing + doubly propagated literal detection + 2-long xor clause detection during failed literal probing + hyper-binary resoolution
        bool      doRemUselessBins;   ///<Should try to remove useless binary clauses at the beginning of solving?
        bool      doCache; ///< Allow storing the cache
        bool      doExtendedSCC; ///< Allow extending SCC with the cache
        bool      doGateFind; ///< Find OR gates
        size_t    maxGateSize;
        bool      doAlwaysFMinim; ///< Always try to minimise clause with cache&gates
        bool      doER; ///< Perform Extended Resolution (ER)
        bool      doCalcReach; ///<Calculate reachability, and influence variable decisions with that
        bool      doAsymmTE; ///< Do Asymtotic blocked clause elimination
        bool      doOTFGateShorten; ///<Shorten learnt clauses on the fly with gates
        bool      doShortenWithOrGates; ///<Shorten clauses with or gates during subsumption
        bool      doRemClWithAndGates; ///<Remove clauses using and gates during subsumption
        bool      doFindEqLitsWithGates; ///<Find equivalent literals using gates during subsumption
        bool      doMixXorAndGates; ///<Try to gain knowledge by mixing XORs and gates

        //Verbosity
        bool      verboseSubsumer; ///< Make subsumer verbose?
        bool      doPrintGateDot; ///< Print DOT file of gates
        bool      doPrintConflDot; ///< Print DOT file for each conflict

        //interrupting & dumping
        bool      needToDumpLearnts;  ///<If set to TRUE, learnt clauses will be dumped to the file speified by "learntsFilename"
        bool      needToDumpOrig;     ///<If set to TRUE, a simplified version of the original clause-set will be dumped to the file speified by "origFilename". The solution to this file should perfectly satisfy the problem
        std::string learntsFilename;    ///<Dump sorted learnt clauses to this file. Only active if "needToDumpLearnts" is set to TRUE
        std::string origFilename;       ///<Dump simplified original problem CNF to this file. Only active if "needToDumpOrig" is set to TRUE
        uint32_t  maxDumpLearntsSize; ///<When dumping the learnt clauses, this is the maximum clause size that should be dumped
        bool      libraryUsage;       ///<Set to true if not used as a library. In fact, this is TRUE by default, and Main.cpp sets it to "FALSE". Disables some simplifications at the beginning of solving (mostly performStepsBeforeSolve() )
        bool      greedyUnbound;      ///<If set, then variables will be greedily unbounded (set to l_Undef). This is EXPERIMENTAL

        uint32_t origSeed;
};

#endif //SOLVERCONF_H
