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

#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include "constants.h"
#include <vector>

#include "constants.h"
#include "solvertypes.h"
#include "implcache.h"
#include "propengine.h"
#include "searcher.h"
#include "GitSHA1.h"
#include <fstream>

namespace CMSat {

using std::vector;
using std::pair;
using std::string;

class VarReplacer;
class ClauseCleaner;
class Prober;
class Simplifier;
class SCCFinder;
class ClauseVivifier;
class CalcDefPolars;
class SolutionExtender;
class SQLStats;
class ImplCache;
class CompFinder;
class CompHandler;
class SubsumeStrengthen;

class LitReachData {
    public:
        LitReachData() :
            lit(lit_Undef)
            , numInCache(0)
        {}
        Lit lit;
        uint32_t numInCache;
};


class Solver : public Searcher
{
    public:
        Solver(const SolverConf _conf = SolverConf());
        virtual ~Solver();

        //////////
        //External stats
        void fileAdded(const string& filename);

        //////////////////////////////
        //Solving
        //
        lbool solve(const vector<Lit>* _assumptions = NULL);
        void        setNeedToInterrupt();
        vector<lbool>  model;
        lbool   modelValue (const Lit p) const;  ///<Found model value for lit

        //////////////////////////////
        // Problem specification:
        Var  newVar(const bool dvar = true); ///< Add new variable
        bool addClause(const vector<Lit>& ps);  ///< Add clause to the solver
        bool addXorClause(const vector<Var>& vars, bool rhs);
        bool addRedClause(
            const vector<Lit>& ps
            , const ClauseStats& stats = ClauseStats()
        );

        //////////////////////////
        //Stats
        static const char* getVersion();

        uint64_t getNumLongClauses() const;
        bool     getNeedToDumpReds() const;
        bool     getNeedToDumpIrredundant() const;
        int      getVerbosity() const;
        void     printStats() const;
        void     printClauseStats() const;
        void     print_value_kilo_mega(uint64_t value) const;
        void     addInPartialSolvingStat();
        size_t   getNumDecisionVars() const;
        size_t   getNumFreeVars() const;
        const SolverConf& getConf() const;
        const vector<string>& getFileNamesUsed() const;
        const BinTriStats& getBinTriStats() const;
        size_t   getNumLongIrredCls() const;
        size_t   getNumLongRedCls() const;
        const vector<Var>& getInterToOuterMain() const;
        size_t getNumVarsElimed() const;
        size_t getNumVarsReplaced() const;
        void dumpIfNeeded() const;
        void print_elimed_vars() const;
        Var numActiveVars() const;
        void printMemStats() const;
        uint64_t printWatchMemUsed(uint64_t totalMem) const;


        ///Return number of variables waiting to be replaced
        size_t getNewToReplaceVars() const;
        const Stats& getStats() const;
        uint64_t getNextCleanLimit() const;

        ///////////////////////////////////
        // State Dumping
        template<class T>
        vector<Lit> clauseBackNumbered(const T& cl) const;
        void dumpUnitaryClauses(std::ostream* os) const;
        void dumpEquivalentLits(std::ostream* os) const;
        void dumpBinClauses(
            const bool dumpRed
            , const bool dumpIrred
            , std::ostream* outfile
        ) const;

        void dumpTriClauses(
            const bool alsoRed
            , const bool alsoIrred
            , std::ostream* outfile
        ) const;

        ///Dump all redundant clauses into a file
        void dumpRedClauses(
            std::ostream* os
            , const uint32_t maxSize
        ) const;

        ///Dump irredundant clasues intto a file
        void dumpIrredClauses(
            std::ostream* os
        ) const;

        struct SolveStats
        {
            SolveStats() :
                numSimplify(0)
                , nbReduceDB(0)
                , subsBinWithBinTime(0)
                , subsBinWithBin(0)
                , numCallReachCalc(0)
            {}

            SolveStats& operator+=(const SolveStats& other)
            {
                numSimplify += other.numSimplify;
                nbReduceDB += other.nbReduceDB;
                subsBinWithBinTime += other.subsBinWithBinTime;
                subsBinWithBin += other.subsBinWithBin;
                numCallReachCalc += other.numCallReachCalc;

                return *this;
            }

            uint64_t numSimplify;
            uint64_t nbReduceDB;
            double subsBinWithBinTime;
            uint64_t subsBinWithBin;
            uint64_t numCallReachCalc;
        };
        const SolveStats& getSolveStats() const;


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
        void checkImplicitPropagated() const;
        void checkStats(const bool allowFreed = false) const;
        void checkImplicitStats() const;

    protected:

        //friend class SQLStats;
        SQLStats* sqlStats;
        vector<string> fileNamesUsed;

        //Attaching-detaching clauses
        virtual void attachClause(
            const Clause& c
            , const bool checkAttach = true
        );
        virtual void attachBinClause(
            const Lit lit1
            , const Lit lit2
            , const bool red
            , const bool checkUnassignedFirst = true
        );
        virtual void attachTriClause(
            const Lit lit1
            , const Lit lit2
            , const Lit lit3
            , const bool red
        );
        virtual void detachTriClause(
            const Lit lit1
            , const Lit lit2
            , const Lit lit3
            , const bool red
        );
        virtual void detachBinClause(
            const Lit lit1
            , const Lit lit2
            , const bool red
        );
        virtual void  detachClause(const Clause& c, const bool removeDrup = true);
        virtual void  detachClause(const ClOffset offset, const bool removeDrup = true);
        virtual void  detachModifiedClause(
            const Lit lit1
            , const Lit lit2
            , const uint32_t origSize
            , const Clause* address
        );
        Clause* addClauseInt(
            const vector<Lit>& lits
            , const bool red = false
            , const ClauseStats stats = ClauseStats()
            , const bool attach = true
            , vector<Lit>* finalLits = NULL
            , bool addDrup = true
        );

    private:
        vector<Lit> origAssumptions;
        void checkDecisionVarCorrectness() const;
        bool enqueueThese(const vector<Lit>& toEnqueue);

        //Stats printing
        void printMinStats() const;
        void printFullStats() const;

        bool addXorClauseInt(
            const vector< Lit >& lits
            , bool rhs
            , const bool attach
        );
        lbool simplifyProblem();
        SolveStats solveStats;
        void check_minimization_effectiveness(lbool status);
        void check_recursive_minimization_effectiveness(const lbool status);
        void extend_solution();

        /////////////////////
        // Objects that help us accomplish the task
        friend class ClauseAllocator;
        friend class StateSaver;
        friend class SolutionExtender;
        friend class VarReplacer;
        friend class SCCFinder;
        friend class Prober;
        friend class ClauseVivifier;
        friend class Simplifier;
        friend class SubsumeStrengthen;
        friend class ClauseCleaner;
        friend class CompleteDetachReatacher;
        friend class CalcDefPolars;
        friend class ImplCache;
        friend class Searcher;
        friend class XorFinder;
        friend class GateFinder;
        friend class PropEngine;
        friend class CompFinder;
        friend class CompHandler;
        friend class TransCache;
        Prober              *prober;
        Simplifier          *simplifier;
        SCCFinder           *sCCFinder;
        ClauseVivifier      *clauseVivifier;
        ClauseCleaner       *clauseCleaner;
        VarReplacer         *varReplacer;
        CompHandler         *compHandler;
        MTRand              mtrand;           ///< random number generator

        /////////////////////////////
        // Temporary datastructs -- must be cleared before use
        mutable std::vector<Lit> tmpCl;

        /////////////////////////////
        //Renumberer
        void renumberVariables();
        void freeUnusedWatches();
        void saveVarMem(uint32_t newNumVars);
        void unSaveVarMem();

        /////////////////////////////
        // SAT solution verification
        bool verifyModel() const;
        bool verifyImplicitClauses() const;
        bool verifyClauses(const vector<ClOffset>& cs) const;

        ///////////////////////////
        // Clause cleaning
        void fullReduce();
        void clearClauseStats(vector<ClOffset>& clauseset);
        CleaningStats reduceDB();           ///<Reduce the set of redundant clauses.
        struct reduceDBStructGlue
        {
            reduceDBStructGlue(ClauseAllocator* _clAllocator) :
                clAllocator(_clAllocator)
            {}
            ClauseAllocator* clAllocator;

            bool operator () (const ClOffset x, const ClOffset y);
        };
        struct reduceDBStructSize
        {
            reduceDBStructSize(ClauseAllocator* _clAllocator) :
                clAllocator(_clAllocator)
            {}
            ClauseAllocator* clAllocator;

            bool operator () (const ClOffset x, const ClOffset y);
        };
        struct reduceDBStructActivity
        {
            reduceDBStructActivity(ClauseAllocator* _clAllocator) :
                clAllocator(_clAllocator)
            {}
            ClauseAllocator* clAllocator;

            bool operator () (const ClOffset x, const ClOffset y);
        };
        struct reduceDBStructPropConfl
        {
            reduceDBStructPropConfl(ClauseAllocator* _clAllocator) :
                clAllocator(_clAllocator)
            {}
            ClauseAllocator* clAllocator;

            bool operator () (const ClOffset x, const ClOffset y);
        };
        void pre_clean_clause_db(CleaningStats& tmpStats, uint64_t sumConfl);
        void real_clean_clause_db(
            CleaningStats& tmpStats
            , uint64_t sumConflicts
            , uint64_t removeNum
        );
        uint64_t calc_how_many_to_remove();
        void sort_red_cls_as_required(CleaningStats& tmpStats);
        void print_best_irred_clauses_if_required() const;


        /////////////////////
        // Data
        bool                 needToInterrupt;
        uint64_t             nextCleanLimit;
        uint64_t             nextCleanLimitInc;
        uint32_t             numDecisionVars;
        void setDecisionVar(const uint32_t var);
        void unsetDecisionVar(const uint32_t var);
        size_t               zeroLevAssignsByCNF;
        size_t               zeroLevAssignsByThreads;
        vector<LitReachData> litReachable;
        void calcReachability();

        //Main up stats
        Stats sumStats;
        PropStats sumPropStats;
        CleaningStats cleaningStats;
        ReachabilityStats reachStats;

        /////////////////////
        // Clauses
        bool addClauseHelper(vector<Lit>& ps);
        void                reArrangeClauses();
        void                reArrangeClause(ClOffset offset);
        void                printAllClauses() const;
        void                consolidateMem();

        //////////////////
        // Stamping
        Lit updateLitForDomin(Lit lit) const;
        void updateDominators();

        /////////////////
        // Debug
        void testAllClauseAttach() const;
        bool normClauseIsAttached(const ClOffset offset) const;
        void findAllAttach() const;
        void findAllAttach(const vector<ClOffset>& cs) const;
        bool findClause(const ClOffset offset) const;
        void checkNoWrongAttach() const;
        void printWatchlist(watch_subarray_const ws, const Lit lit) const;
        void printClauseSizeDistrib();
        ClauseUsageStats sumClauseData(
            const vector<ClOffset>& toprint
            , bool red
        ) const;
        void printPropConflStats(
            std::string name
            , const vector<ClauseUsageStats>& stats
        ) const;

        void treatAssumptions(const vector<Lit>* _assumptions);
};

inline void Solver::setDecisionVar(const uint32_t var)
{
    if (!decisionVar[var]) {
        numDecisionVars++;
        decisionVar[var] = true;
        insertVarOrder(var);
    }
}

inline void Solver::unsetDecisionVar(const uint32_t var)
{
    if (decisionVar[var]) {
        numDecisionVars--;
        decisionVar[var] = false;
    }
}

inline bool Solver::getNeedToDumpReds() const
{
    return !conf.redDumpFname.empty();
}

inline bool Solver::getNeedToDumpIrredundant() const
{
    return !conf.irredDumpFname.empty();
}

inline uint64_t Solver::getNumLongClauses() const
{
    return longIrredCls.size() + longRedCls.size();
}

inline int Solver::getVerbosity() const
{
    return conf.verbosity;
}

inline const Searcher::Stats& Solver::getStats() const
{
    return sumStats;
}

inline uint64_t Solver::getNextCleanLimit() const
{
    return nextCleanLimit;
}

inline void Solver::addInPartialSolvingStat()
{
    Searcher::addInPartialSolvingStat();
    sumStats += Searcher::getStats();
    sumPropStats += propStats;
}

inline const Solver::SolveStats& Solver::getSolveStats() const
{
    return solveStats;
}

inline void Solver::fileAdded(const string& filename)
{
    fileNamesUsed.push_back(filename);
}

inline size_t Solver::getNumLongIrredCls() const
{
    return longIrredCls.size();
}

inline size_t Solver::getNumLongRedCls() const
{
    return longRedCls.size();
}

inline const vector<Var>& Solver::getInterToOuterMain() const
{
    return interToOuterMain;
}

inline const SolverConf& Solver::getConf() const
{
    return conf;
}

inline const vector<string>& Solver::getFileNamesUsed() const
{
    return fileNamesUsed;
}

inline const Solver::BinTriStats& Solver::getBinTriStats() const
{
    return binTri;
}

template<class T>
inline vector<Lit> Solver::clauseBackNumbered(const T& cl) const
{
    tmpCl.clear();
    for(size_t i = 0; i < cl.size(); i++) {
        tmpCl.push_back(getUpdatedLit(cl[i], interToOuterMain));
    }

    return tmpCl;
}

inline Var Solver::numActiveVars() const
{
    Var numActive = 0;
    for(Var var = 0; var < solver->nVars(); var++) {
        if (decisionVar[var]
            && (varData[var].removed == Removed::none
                || varData[var].removed == Removed::queued_replacer)
            && value(var) == l_Undef
        ) {
            numActive++;
        }
    }

    return numActive;
}

} //end namespace

#endif //THREADCONTROL_H
