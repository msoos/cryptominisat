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
class CommandControl;
class RestartPrinter;

class ThreadControl : public Solver
{
    public:
        ThreadControl(const SolverConf& _conf);
        ~ThreadControl();

        //////////////////////////////
        //Solving
        const lbool solve(const int threads);
        void        setNeedToInterrupt();
        vector<lbool>  model;
        const lbool   modelValue (const Lit p) const;  ///<Found model value for lit

        //////////////////////////////
        // Problem specification:
        const Var  newVar(const bool dvar = true); ///< Add new variable
        const bool addClause (const vector<Lit>& ps);  ///< Add clause to the solver
        const bool addLearntClause(const vector<Lit>& ps, const uint32_t glue = 10);

        //////////////////////////
        //Stats
        const uint64_t getNumClauses() const;                 ///<Return number of ALL clauses: non-learnt, learnt, bin
        const uint32_t getNumUnsetVars() const;               ///<Return number of unset vars
        const uint32_t getNumElimSubsume() const;             ///<Get number of variables eliminated
        const uint32_t getNumXorTrees() const;                ///<Get the number of SCC trees
        const uint32_t getNumXorTreesCrownSize() const;       ///<Get the number of variables being replaced by other variables
        const double   getTotalTimeSubsumer() const;          ///<Get total time spent in norm-clause massaging
        const double   getTotalTimeFailedLitSearcher() const; ///<Get total time spend in failed-lit probing and related algos
        const double   getTotalTimeSCC() const;               ///<Get total time spent finding binary equi/antivalences
        const bool     getNeedToDumpLearnts() const;
        const bool     getNeedToDumpOrig() const;
        const uint64_t getNumTotalConflicts() const;
        const uint32_t getVerbosity() const;                  ///<Return verbosity level
        void           printStats();
        const uint32_t getNumDecisionVars() const;            ///<Get number of decision vars. May not be accurate TODO fix this
        const uint32_t getNumFreeVars() const;                ///<Get the number of non-set, non-elimed, non-replaced etc. vars. These are truly free
        const uint32_t getNewToReplaceVars() const;           ///<Return number of variables waiting to be replaced
        uint64_t getSumConflicts() const;
        uint64_t getNextCleanLimit() const;

        ///////////////////////////////////
        // State Dumping
        const vector<Clause*>& getLongLearnts() const;  ///<Get all learnt clauses that are >2 long
        const vector<Clause*>& getSortedLongLearnts();  ///<Return the set of learned clauses, sorted according to glue/activity
        void  dumpBinClauses(const bool alsoLearnt, const bool alsoNonLearnt, std::ostream& outfile) const;
        void  dumpSortedLearnts(std::ostream& os, const uint32_t maxSize); ///<Dump all learnt clauses into file
        void  dumpOrigClauses(std::ostream& os) const; ///<Dump "original" (simplified) problem to file

    private:

        /////////////////////
        //Solvers
        vector<CommandControl*> threads;
        vector<Clause*> longLearntsToAdd;
        vector<Lit> unitLearntsToAdd;
        vector<BinaryClause> binLearntsToAdd;

        //Control of other threads
        void            moveClausesHere();
        void            toDetachFree();
        vector<Clause*> toDetach;
        Clause*         newClauseByThread(const vector<Lit>& lits, const uint32_t glue, uint64_t& thisSumConflicts);

        virtual void  attachClause        (const Clause& c, const uint16_t point1 = 0, const uint16_t point2 = 1);
        virtual void  attachBinClause     (const Lit lit1, const Lit lit2, const bool learnt, const bool checkUnassignedFirst = true);
        virtual void  detachModifiedClause(const Lit lit1, const Lit lit2, const Lit lit3, const uint32_t origSize, const Clause* address);
        template<class T> Clause* addClauseInt(const T& ps, const bool learnt = false, const uint32_t glue = 10, const bool attach = true);
        const bool addXorClauseInt(const vector<Lit>& lits, bool rhs);
        const lbool simplifyProblem(const uint64_t numConfls);

        /////////////////////
        //Stats
        template<class T, class T2> void printStatsLine(string left, T value, T2 value2, string extra);
        template<class T> void printStatsLine(string left, T value, string extra = "");
        friend class RestartPrinter;
        RestartPrinter* restPrinter;

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
        // SAT solution verification
        const bool verifyModel() const;                            ///<Verify model[]
        const bool verifyBinClauses() const;                       ///<Verify model[] for binary clauses
        const bool verifyClauses(const vector<Clause*>& cs) const; ///<Verify model[] for normal clauses

        ///////////////////////////
        // Clause cleaning
        void        reduceDB();           ///<Reduce the set of learnt clauses.
        uint64_t    nbReduceDB;           ///<Number of times learnt clause have been cleaned
        uint64_t    numCleanedLearnts;    ///< Number of times learnt clauses have been removed through simplify() up until now
        uint32_t    nbClBeforeRed;        ///< Number of learnt clauses before learnt-clause cleaning
        struct reduceDBStruct
        {
            bool operator () (const Clause* x, const Clause* y);
        };

        /////////////////////
        // Data
        SolverConf           conf;
        ImplCache            implCache;
        const bool           cleanCache();
        vector<LitReachData> litReachable;
        void                 calcReachability();
        bool                 needToInterrupt;
        uint64_t             sumConflicts;
        uint64_t             nextCleanLimit;
        uint64_t             nextCleanLimitInc;
        uint32_t             numDecisionVars;
        void setDecisionVar(const uint32_t var);
        void unsetDecisionVar(const uint32_t var);

        /////////////////////
        // Clauses
        const bool          addClauseHelper(vector<Lit>& ps);
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
        void                waitAllThreads();

        /////////////////
        // Debug
        void testAllClauseAttach() const;
        const bool normClauseIsAttached(const Clause& c) const;
        void findAllAttach() const;
        const bool findClause(const Clause* c) const;
        void checkNoWrongAttach() const;
        void calcClauseDistrib();
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

inline const vector<Clause*>& ThreadControl::getSortedLongLearnts()
{
    std::sort(learnts.begin(), learnts.begin()+learnts.size(), reduceDBStruct());
    return learnts;
}

inline const bool ThreadControl::getNeedToDumpLearnts() const
{
    return conf.needToDumpLearnts;
}

inline const bool ThreadControl::getNeedToDumpOrig() const
{
    return conf.needToDumpOrig;
}

inline const uint64_t ThreadControl::getNumClauses() const
{
    return numBins + clauses.size() + learnts.size();
}

inline const uint32_t ThreadControl::getVerbosity() const
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

#endif //THREADCONTROL_H
