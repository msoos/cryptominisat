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
#include "GitSHA1.h"

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
        bool addLearntClause(
            const vector<Lit>& ps
            , const ClauseStats& stats = ClauseStats()
        );

        //////////////////////////
        //Stats
        static const char* getVersion();
        uint64_t getNumLongClauses() const;                 ///<Return number of ALL clauses: non-learnt, learnt, bin
        bool     getNeedToDumpLearnts() const;
        bool     getNeedToDumpOrig() const;
        uint32_t getVerbosity() const;                  ///<Return verbosity level
        void     printFullStats();
        void     printClauseStats();
        void     addInPartialSolvingStat();
        uint32_t getNumDecisionVars() const;            ///<Get number of decision vars. May not be accurate TODO fix this
        uint32_t getNumFreeVars() const;                ///<Get the number of non-set, non-elimed, non-replaced etc. vars
        uint32_t getNumFreeVarsAdv(size_t tail_size_thread) const;                ///<Get the number of non-set, non-elimed, non-replaced etc. vars
        uint32_t getNewToReplaceVars() const;           ///<Return number of variables waiting to be replaced
        const Stats& getStats() const;
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


        struct CleaningStats
        {
            CleaningStats() :
                cpu_time(0)
                //Before remove
                , origNumClauses(0)
                , origNumLits(0)

                //Pre-remove
                , preRemovedClauses(0)
                , preRemovedClausesLits(0)
                , preRemovedClausesGlue(0)

                //Type of clean
                , glueBasedClean(0)
                , sizeBasedClean(0)
                , propConflBasedClean(0)

                //Clause Cleaning
                , removedClauses(0)
                , removedClausesLits(0)
                , removedClausesGlue(0)

                , remainClauses(0)
                , remainClausesLits(0)
                , remainClausesGlue(0)
            {}

            CleaningStats& operator+=(const CleaningStats& other)
            {
                //Time
                cpu_time += other.cpu_time;

                //Before remove
                origNumClauses += other.origNumClauses;
                origNumLits += other.origNumLits;

                //Pre-remove
                preRemovedClauses += other.preRemovedClauses;
                preRemovedClausesLits += other.preRemovedClausesLits;
                preRemovedClausesGlue += other.preRemovedClausesGlue;

                //Type of clean
                glueBasedClean += other.glueBasedClean;
                sizeBasedClean += other.sizeBasedClean;
                propConflBasedClean += other.propConflBasedClean;

                //Clause Cleaning
                removedClauses += other.removedClauses;
                removedClausesLits += other.removedClausesLits;
                removedClausesGlue += other.removedClausesGlue;

                remainClauses += other.remainClauses;
                remainClausesLits += other.remainClausesLits;
                remainClausesGlue += other.remainClausesGlue;

                return *this;
            }

            void print(const size_t nbReduceDB) const
            {
                cout << "c ------ CLEANING STATS ---------" << endl;
                //Pre-clean
                printStatsLine("c pre-removed"
                    , preRemovedClauses
                    , (double)preRemovedClauses/(double)origNumClauses*100.0
                    , "% long learnt clauses"
                );

                printStatsLine("c pre-removed lits"
                    , preRemovedClausesLits
                    , (double)preRemovedClausesLits/(double)origNumLits*100.0
                    , "% long learnt lits"
                );
                printStatsLine("c pre-removed cl avg size"
                    , (double)preRemovedClausesLits/(double)preRemovedClauses
                );
                printStatsLine("c pre-removed cl avg glue"
                    , (double)preRemovedClausesGlue/(double)preRemovedClauses
                );

                //Types of clean
                printStatsLine("c clean by glue"
                    , glueBasedClean
                    , (double)glueBasedClean/(double)nbReduceDB*100.0
                    , "% cleans"
                );
                printStatsLine("c clean by size"
                    , sizeBasedClean
                    , (double)sizeBasedClean/(double)nbReduceDB*100.0
                    , "% cleans"
                );
                printStatsLine("c clean by prop&confl"
                    , propConflBasedClean
                    , (double)propConflBasedClean/(double)nbReduceDB*100.0
                    , "% cleans"
                );

                //--- Actual clean --

                //-->CLEAN
                printStatsLine("c cleaned cls"
                    , removedClauses
                    , (double)removedClauses/(double)origNumClauses*100.0
                    , "% long learnt clauses"
                );
                printStatsLine("c cleaned lits"
                    , removedClausesLits
                    , (double)removedClausesLits/(double)origNumLits*100.0
                    , "% long learnt lits"
                );
                printStatsLine("c cleaned cl avg size"
                    , (double)removedClausesLits/(double)removedClauses
                );
                printStatsLine("c cleaned avg glue"
                    , (double)removedClausesGlue/(double)removedClauses
                );

                //--> REMAIN
                printStatsLine("c remain cls"
                    , remainClauses
                    , (double)remainClauses/(double)origNumClauses*100.0
                    , "% long learnt clauses"
                );
                printStatsLine("c remain lits"
                    , remainClausesLits
                    , (double)remainClausesLits/(double)origNumLits*100.0
                    , "% long learnt lits"
                );
                printStatsLine("c remain cl avg size"
                    , (double)remainClausesLits/(double)remainClauses
                );
                printStatsLine("c remain avg glue"
                    , (double)remainClausesGlue/(double)remainClauses
                );

                cout << "c ------ CLEANING STATS END ---------" << endl;
            }

            void printShort() const
            {
                //Pre-clean
                cout
                << "c [DBclean]"
                << " Pre-removed: "
                << preRemovedClauses
                << " next by " << getNameOfCleanType(clauseCleaningType)
                << endl;

                cout
                << "c [DBclean]"
                << " rem " << removedClauses

                << " avgGlue " << std::fixed << std::setprecision(2)
                << ((double)removedClausesGlue/(double)removedClauses)

                << " avgSize "
                << std::fixed << std::setprecision(2)
                << ((double)removedClausesLits/(double)removedClauses)
                << endl;

                cout
                << "c [DBclean]"
                << " remain " << remainClauses

                << " avgGlue " << std::fixed << std::setprecision(2)
                << ((double)remainClausesGlue/(double)remainClauses)

                << " avgSize " << std::fixed << std::setprecision(2)
                << ((double)remainClausesLits/(double)remainClauses)
                << endl;
            }

            //Time
            double cpu_time;

            //Before remove
            uint64_t origNumClauses;
            uint64_t origNumLits;

            //Clause Cleaning --pre-remove
            uint64_t preRemovedClauses;
            uint64_t preRemovedClausesLits;
            uint64_t preRemovedClausesGlue;

            //Clean type
            clauseCleaningTypes clauseCleaningType;
            size_t glueBasedClean;
            size_t sizeBasedClean;
            size_t propConflBasedClean;

            //Clause Cleaning
            uint64_t removedClauses;
            uint64_t removedClausesLits;
            uint64_t removedClausesGlue;

            uint64_t remainClauses;
            uint64_t remainClausesLits;
            uint64_t remainClausesGlue;
        };


        struct ReachabilityStats
        {
            ReachabilityStats() :
                cpu_time(0)
                , numLits(0)
                , dominators(0)
                , numLitsDependent(0)
            {}

            ReachabilityStats& operator+=(const ReachabilityStats& other)
            {
                cpu_time += other.cpu_time;

                numLits += other.numLits;
                dominators += other.dominators;
                numLitsDependent += other.numLitsDependent;

                return *this;
            }

            void print() const
            {
                cout << "c ------- REACHABILITY STATS -------" << endl;
                printStatsLine("c time"
                    , cpu_time
                );

                printStatsLine("c dominator lits"
                    , (double)dominators/(double)numLits*100.0
                    , "% of unknowns lits"
                );

                printStatsLine("c dependent lits"
                    , (double)(numLitsDependent)/(double)numLits*100.0
                    , "% of unknown lits"
                );

                printStatsLine("c avg num. dominated lits"
                    , (double)numLitsDependent/(double)dominators
                );

                cout << "c ------- REACHABILITY STATS END -------" << endl;
            }

            void printShort() const
            {
                cout
                << "c [reach]"
                << " dom lits: " << std::fixed << std::setprecision(2)
                << (double)dominators/(double)numLits*100.0
                << " %"

                << " dep-lits: " << std::fixed << std::setprecision(2)
                << (double)numLitsDependent/(double)numLits*100.0
                << " %"

                << " dep-lits/dom-lits : " << std::fixed << std::setprecision(2)
                << (double)numLitsDependent/(double)dominators

                << " T: " << std::fixed << std::setprecision(2)
                << cpu_time << " s"
                << endl;
            }

            double cpu_time;

            size_t numLits;
            size_t dominators;
            size_t numLitsDependent;
        };

        //Checks
        void checkStats() const;

    private:

        //Control
        Clause*  newClauseByThread(
            const vector<Lit>& lits
            , const uint32_t glue
        );

        //Attaching-detaching clauses
        virtual void  attachClause        (const Clause& c);
        virtual void  attachBinClause     (const Lit lit1, const Lit lit2, const bool learnt, const bool checkUnassignedFirst = true);
        virtual void  detachClause        (const Clause& c);
        virtual void  detachModifiedClause(const Lit lit1, const Lit lit2, const Lit lit3, const uint32_t origSize, const Clause* address);
        template<class T> Clause* addClauseInt(
            const T& ps
            , const bool learnt = false
            , const ClauseStats& stats = ClauseStats()
            , const bool attach = true
        );

        bool addXorClauseInt(const vector<Lit>& lits, bool rhs);
        lbool simplifyProblem(const uint64_t numConfls);

        /////////////////////
        //Stats
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
        vector<Var> outerToInterMain;
        vector<Var> interToOuterMain;
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
        uint64_t             nextCleanLimit;
        uint64_t             nextCleanLimitInc;
        uint32_t             numDecisionVars;
        void setDecisionVar(const uint32_t var);
        void unsetDecisionVar(const uint32_t var);
        size_t               zeroLevAssignsByCNF;
        size_t               zeroLevAssignsByThreads;

        //Main up stats
        Stats sumStats;
        CleaningStats cleaningStats;
        ReachabilityStats reachStats;
        size_t numCallReachCalc;

        /////////////////////
        // Clauses
        bool          addClauseHelper(vector<Lit>& ps);
        friend class        ClauseAllocator;
        vector<char>        decision_var;
        vector<Clause*>     clauses;          ///< List of problem clauses that are larger than 2
        vector<Clause*>     learnts;          ///< List of learnt clauses.
        uint64_t            clausesLits;  ///< Number of literals in non-learnt clauses
        uint64_t            learntsLits;  ///< Number of literals in learnt clauses
        uint64_t            numBinsNonLearnt;
        uint64_t            numBinsLearnt;
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
                , sumLookedAt(0)
            {}

            size_t num;
            size_t sumPropConfl;
            size_t sumLitVisited;
            size_t sumLookedAt;

            UsageStats& operator+=(const UsageStats& other)
            {
                num += other.num;
                sumPropConfl += other.sumPropConfl;
                sumLitVisited += other.sumLitVisited;
                sumLookedAt += other.sumLookedAt;

                return *this;
            }
        };
        void testAllClauseAttach() const;
        bool normClauseIsAttached(const Clause& c) const;
        void findAllAttach() const;
        bool findClause(const Clause* c) const;
        void checkNoWrongAttach() const;
        void printClauseSizeDistrib();
        UsageStats sumClauseData(
            const vector<Clause*>& toprint
            , bool learnt
        ) const;
        void printPropConflStats(
            std::string name
            , const vector<UsageStats>& stats
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

inline uint64_t ThreadControl::getNumLongClauses() const
{
    return clauses.size() + learnts.size();
}

inline uint32_t ThreadControl::getVerbosity() const
{
    return conf.verbosity;
}

inline const CommandControl::Stats& ThreadControl::getStats() const
{
    return sumStats;
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

inline void ThreadControl::addInPartialSolvingStat()
{
    CommandControl::addInPartialSolvingStat();
    sumStats += CommandControl::getStats();
}

#endif //THREADCONTROL_H
