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

#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include <stdint.h>
#include <vector>

#include "constants.h"
#include "SolverTypes.h"
#include "ImplCache.h"
#include "SolverConf.h"
#include "Solver.h"
#include "CommandControl.h"

using std::vector;
using std::pair;
using std::string;

class VarReplacer;
class ClauseCleaner;
class FailedLitSearcher;
class Subsumer;
class SCCFinder;
class ClauseVivifier;
class CalcDefPolars;
class SolutionExtender;
class ImplCache;
class RestartPrinter;

class ThreadControl : public CommandControl
{
    public:
        ThreadControl(const SolverConf& _conf);
        ~ThreadControl();

        //////////////////////////////
        //Solving
        lbool solve();
        void        setNeedToInterrupt();
        vector<lbool>  model;
        lbool   modelValue (const Lit p) const;  ///<Found model value for lit

        //////////////////////////////
        // Problem specification:
        Var  newVar(const bool dvar = true); ///< Add new variable
        bool addClause (const vector<Lit>& ps);  ///< Add clause to the solver
        bool addLearntClause(const vector<Lit>& ps, const uint32_t glue = 10);

        //////////////////////////
        //Stats
        uint64_t getNumClauses() const;                 ///<Return number of ALL clauses: non-learnt, learnt, bin
        uint32_t getNumUnsetVars() const;               ///<Return number of unset vars
        uint32_t getNumElimSubsume() const;             ///<Get number of variables eliminated
        uint32_t getNumXorTrees() const;                ///<Get the number of SCC trees
        uint32_t getNumXorTreesCrownSize() const;       ///<Get the number of variables being replaced by other variables
        double   getTotalTimeSubsumer() const;          ///<Get total time spent in norm-clause massaging
        double   getTotalTimeFailedLitSearcher() const; ///<Get total time spend in failed-lit probing and related algos
        double   getTotalTimeSCC() const;               ///<Get total time spent finding binary equi/antivalences
        bool     getNeedToDumpLearnts() const;
        bool     getNeedToDumpOrig() const;
        uint64_t getNumTotalConflicts() const;
        uint32_t getVerbosity() const;                  ///<Return verbosity level
        void           printStats();
        uint32_t getNumDecisionVars() const;            ///<Get number of decision vars. May not be accurate TODO fix this
        uint32_t getNumFreeVars() const;                ///<Get the number of non-set, non-elimed, non-replaced etc. vars
        uint32_t getNumFreeVarsAdv(size_t tail_size_thread) const;                ///<Get the number of non-set, non-elimed, non-replaced etc. vars
        uint32_t getNewToReplaceVars() const;           ///<Return number of variables waiting to be replaced
        uint64_t getSumConflicts() const;
        uint64_t getNextCleanLimit() const;
        bool     getSavedPolarity(Var var) const;
        uint32_t getSavedActivity(const Var var) const;
        uint32_t getSavedActivityInc() const;

        ///////////////////////////////////
        // State Dumping
        const vector<Clause*>& getLongLearnts() const;  ///<Get all learnt clauses that are >2 long
        void  dumpBinClauses(const bool alsoLearnt, const bool alsoNonLearnt, std::ostream& outfile) const;
        void  dumpLearnts(std::ostream& os, const uint32_t maxSize); ///<Dump all learnt clauses into file
        void  dumpOrigClauses(std::ostream& os) const; ///<Dump "original" (simplified) problem to file

    private:

        //Control
        Clause*  newClauseByThread(
            const vector<Lit>& lits
            , const uint32_t glue
        );

        //Attaching-detaching clauses
        virtual void  attachClause        (const Clause& c);
        virtual void  attachBinClause     (const Lit lit1, const Lit lit2, const bool learnt, const bool checkUnassignedFirst = true);
        virtual void  detachModifiedClause(const Lit lit1, const Lit lit2, const Lit lit3, const uint32_t origSize, const Clause* address);
        template<class T> Clause* addClauseInt(
            const T& ps
            , const bool learnt = false
            , const uint32_t glue = 10
            , const bool attach = true
        );

        bool addXorClauseInt(const vector<Lit>& lits, bool rhs);
        lbool simplifyProblem(const uint64_t numConfls);

        /////////////////////
        //Stats
        template<class T, class T2> void printStatsLine(string left, T value, T2 value2, string extra);
        template<class T> void printStatsLine(string left, T value, string extra = "");
        friend class RestartPrinter;
        RestartPrinter* restPrinter;
        vector<uint32_t> backupActivity;
        vector<bool>     backupPolarity;
        uint32_t         backupActivityInc;

        /////////////////////
        // Objects that help us accomplish the task
        friend class StateSaver;
        friend class SolutionExtender;
        friend class VarReplacer;
        friend class SCCFinder;
        friend class FailedLitSearcher;
        friend class ClauseVivifier;
        friend class Subsumer;
        friend class ClauseCleaner;
        friend class CompleteDetachReatacher;
        friend class CalcDefPolars;
        friend class ImplCache;
        friend class CommandControl;
        friend class XorFinder;
        friend class GateFinder;
        FailedLitSearcher   *failedLitSearcher;
        Subsumer            *subsumer;
        SCCFinder           *sCCFinder;
        ClauseVivifier      *clauseVivifier;
        ClauseCleaner       *clauseCleaner;
        VarReplacer         *varReplacer;
        MTRand              mtrand;           ///< random number generator


        /////////////////////////////
        //Renumberer
        vector<Var> outerToInter;
        vector<Var> interToOuter;
        void renumberVariables();

        /////////////////////////////
        // SAT solution verification
        bool verifyModel() const;                            ///<Verify model[]
        bool verifyBinClauses() const;                       ///<Verify model[] for binary clauses
        bool verifyClauses(const vector<Clause*>& cs) const; ///<Verify model[] for normal clauses

        ///////////////////////////
        // Clause cleaning
        void        fullReduce();
        void        clearPropConfl(vector<Clause*>& clauseset);
        void        reduceDB();           ///<Reduce the set of learnt clauses.
        uint64_t    nbReduceDB;           ///<Number of times learnt clause have been cleaned
        uint64_t    numCleanedLearnts;    ///< Number of times learnt clauses have been removed through simplify() up until now
        uint32_t    nbClBeforeRed;        ///< Number of learnt clauses before learnt-clause cleaning
        struct reduceDBStructGlue
        {
            bool operator () (const Clause* x, const Clause* y);
        };
        struct reduceDBStructSize
        {
            bool operator () (const Clause* x, const Clause* y);
        };
        struct reduceDBStructPropConfl
        {
            bool operator () (const Clause* x, const Clause* y);
        };

        /////////////////////
        // Data
        SolverConf           conf;
        ImplCache            implCache;
        vector<LitReachData> litReachable;
        void                 calcReachability();
        bool                 needToInterrupt;
        uint64_t             sumConflicts;
        uint64_t             nextCleanLimit;
        uint64_t             nextCleanLimitInc;
        uint32_t             numDecisionVars;
        void setDecisionVar(const uint32_t var);
        void unsetDecisionVar(const uint32_t var);
        size_t               zeroLevAssignsByCNF;
        size_t               zeroLevAssignsByThreads;

        /////////////////////
        // Clauses
        bool          addClauseHelper(vector<Lit>& ps);
        friend class        ClauseAllocator;
        vector<char>        decision_var;
        vector<Clause*>     clauses;          ///< List of problem clauses that are larger than 2
        vector<Clause*>     learnts;          ///< List of learnt clauses.
        uint64_t            clausesLits;  ///< Number of literals in non-learnt clauses
        uint64_t            learntsLits;  ///< Number of literals in learnt clauses
        uint64_t            numBins;
        vector<char>        locked; ///<Before reduceDB, threads fill this up (index by clause num)
        void                reArrangeClauses();
        void                reArrangeClause(Clause* clause);
        void                checkLiteralCount() const;
        void                printAllClauses() const;
        void                consolidateMem();

        /////////////////
        // Debug
        struct UsageStats
        {
            UsageStats() :
                num(0)
                , sumPropConfl(0)
                , sumLitVisited(0)
            {}

            size_t num;
            size_t sumPropConfl;
            size_t sumLitVisited;
        };
        void testAllClauseAttach() const;
        bool normClauseIsAttached(const Clause& c) const;
        void findAllAttach() const;
        bool findClause(const Clause* c) const;
        void checkNoWrongAttach() const;
        void calcClauseDistrib();
        uint64_t sumClauseData(
            const vector<Clause*>& toprint
            , bool learnt
        ) const;
        void printPropConflStats(
            std::string name
            , const vector<UsageStats>& stats
            , bool learnt
        ) const;

        void dumpIndividualPropConflStats(
            std::string name
            , const vector<UsageStats>& stats
            , const bool learnt
        ) const;
};

inline void ThreadControl::setDecisionVar(const uint32_t var)
{
    if (!decision_var[var]) {
        numDecisionVars++;
        decision_var[var] = true;
    }
}

inline void ThreadControl::unsetDecisionVar(const uint32_t var)
{
    if (decision_var[var]) {
        numDecisionVars--;
        decision_var[var] = false;
    }
}

inline const vector<Clause*>& ThreadControl::getLongLearnts() const
{
    return learnts;
}

inline bool ThreadControl::getNeedToDumpLearnts() const
{
    return conf.needToDumpLearnts;
}

inline bool ThreadControl::getNeedToDumpOrig() const
{
    return conf.needToDumpOrig;
}

inline uint64_t ThreadControl::getNumClauses() const
{
    return numBins + clauses.size() + learnts.size();
}

inline uint32_t ThreadControl::getVerbosity() const
{
    return conf.verbosity;
}

inline uint64_t ThreadControl::getSumConflicts() const
{
    return sumConflicts;
}

inline uint64_t ThreadControl::getNextCleanLimit() const
{
    return nextCleanLimit;
}

inline bool ThreadControl::getSavedPolarity(const Var var) const
{
    return backupPolarity[var];
}

inline uint32_t ThreadControl::getSavedActivity(const Var var) const
{
    return backupActivity[var];
}

inline uint32_t ThreadControl::getSavedActivityInc() const
{
    return backupActivityInc;
}

#endif //THREADCONTROL_H
