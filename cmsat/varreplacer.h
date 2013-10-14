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

#ifndef VARREPLACER_H
#define VARREPLACER_H

#include <map>
#include <vector>

#include "constants.h"
#include "solvertypes.h"
#include "clause.h"
#include "vec.h"

namespace CMSat {

//#define VERBOSE_DEBUG

using std::map;
using std::vector;
class SolutionExtender;
class Solver;

class LaterAddBinXor
{
    public:
        LaterAddBinXor(const Lit _lit1, const Lit _lit2) :
            lit1(_lit1)
            , lit2(_lit2)
        {}

        Lit lit1;
        Lit lit2;
};

/**
@brief Replaces variables with their anti/equivalents
*/
class VarReplacer
{
    public:
        VarReplacer(Solver* solver);
        ~VarReplacer();
        bool performReplace();
        bool replace(
            Lit lit1
            , Lit lit2
            , const bool xorEqualFalse
            , bool addLaterAsTwoBins
        );

        void extendModel(SolutionExtender* extender) const;

        vector<Var> getReplacingVars() const;
        const vector<Lit>& getReplaceTable() const;
        Lit getLitReplacedWith(Lit lit) const;
        Var getVarReplacedWith(const Var var) const;
        const map<Var, vector<Var> >&getReverseTable() const;
        bool isReplaced(const Var var) const;
        bool isReplaced(const Lit lit) const;
        bool replacingVar(const Var var) const;
        void newVar();
        bool addLaterAddBinXor();
        void updateVars(
            const vector<uint32_t>& outerToInter
            , const vector<uint32_t>& interToOuter
        );
        void checkUnsetSanity();

        //Stats
        size_t getNumReplacedVars() const;
        size_t getNumLastReplacedVars() const;
        size_t getNewToReplaceVars() const;
        size_t getNumTrees() const;
        struct Stats
        {
            Stats() :
                numCalls(0)
                , cpu_time(0)
                , replacedLits(0)
                , zeroDepthAssigns(0)
                , actuallyReplacedVars(0)
                , removedBinClauses(0)
                , removedTriClauses(0)
                , removedLongClauses(0)
                , removedLongLits(0)
            {}

            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator+=(const Stats& other)
            {
                numCalls += other.numCalls;
                cpu_time += other.cpu_time;
                replacedLits += other.replacedLits;
                zeroDepthAssigns += other.zeroDepthAssigns;
                actuallyReplacedVars += other.actuallyReplacedVars;
                removedBinClauses += other.removedBinClauses;
                removedTriClauses += other.removedTriClauses;
                removedLongClauses += other.removedLongClauses;
                removedLongLits += other.removedLongLits;

                return *this;
            }

            void print(const size_t nVars) const
            {
                cout << "c --------- VAR REPLACE STATS ----------" << endl;
                printStatsLine("c time"
                    , cpu_time
                    , cpu_time/(double)numCalls
                    , "per call"
                );

                printStatsLine("c trees' crown"
                    , actuallyReplacedVars
                    , 100.0*(double)actuallyReplacedVars/(double)nVars
                    , "% of vars"
                );

                printStatsLine("c 0-depth assigns"
                    , zeroDepthAssigns
                    , (double)zeroDepthAssigns/(double)nVars*100.0
                    , "% vars"
                );

                printStatsLine("c lits replaced"
                    , replacedLits
                );

                printStatsLine("c bin cls removed"
                    , removedBinClauses
                );

                printStatsLine("c tri cls removed"
                    , removedTriClauses
                );

                printStatsLine("c long cls removed"
                    , removedLongClauses
                );

                printStatsLine("c long lits removed"
                    , removedLongLits
                );
                cout << "c --------- VAR REPLACE STATS END ----------" << endl;
            }

            void printShort() const
            {
                cout
                << "c vrep"
                << " vars " << actuallyReplacedVars
                << " lits " << replacedLits
                << " rem-bin-cls " << removedBinClauses
                << " rem-tri-cls " << removedTriClauses
                << " rem-long-cls " << removedLongClauses
                << " T: " << std::fixed << std::setprecision(2)
                << cpu_time << " s "
                << endl;
            }

            uint64_t numCalls;
            double cpu_time;
            uint64_t replacedLits; ///<Num literals replaced during var-replacement
            uint64_t zeroDepthAssigns;
            uint64_t actuallyReplacedVars;
            uint64_t removedBinClauses;
            uint64_t removedTriClauses;
            uint64_t removedLongClauses;
            uint64_t removedLongLits;
        };
        const Stats& getStats() const;
        size_t memUsed() const;

    private:
        Solver* solver; ///<The solver we are working with

        bool replace_set(vector<ClOffset>& cs);
        void update_vardata_and_decisionvar(
            const Var orig
            , const Var replaced
        );
        bool enqueueDelayedEnqueue();

        //Helpers for replace()
        void replaceChecks(const Lit lit1, const Lit lit2) const;
        bool handleAlreadyReplaced(const Lit lit1, const Lit lit2);
        bool handleBothSet(
            const Lit lit1
            , const lbool val1
            , const Lit lit2
            , const lbool val2
        );
        bool handleOneSet(
            const Lit lit1
            , const lbool val1
            , const Lit lit2
            , const lbool val2
        );

        //Temporary used in replaceImplicit
        vector<BinaryClause> delayedAttach;
        bool replaceImplicit();
        struct ImplicitTmpStats
        {
            ImplicitTmpStats() :
                removedRedBin(0)
                , removedIrredBin(0)
                , removedRedTri(0)
                , removedIrredTri(0)
            {
            }

            void remove(const Watched& ws)
            {
                if (ws.isTri()) {
                    if (ws.red()) {
                        removedRedTri++;
                    } else {
                        removedIrredTri++;
                    }
                } else if (ws.isBinary()) {
                    if (ws.red()) {
                        removedRedBin++;
                    } else {
                        removedIrredBin++;
                    }
                } else {
                    assert(false);
                }
            }

            void clear()
            {
                *this = ImplicitTmpStats();
            }

            size_t removedRedBin;
            size_t removedIrredBin;
            size_t removedRedTri;
            size_t removedIrredTri;
        };
        ImplicitTmpStats impl_tmp_stats;
        void updateTri(
            vec<Watched>::iterator& i
            , vec<Watched>::iterator& j
            , const Lit origLit1
            , const Lit origLit2
            , Lit lit1
            , Lit lit2
        );
        void updateBin(
            vec<Watched>::iterator& i
            , vec<Watched>::iterator& j
            , const Lit origLit1
            , const Lit origLit2
            , Lit lit1
            , Lit lit2
        );
        void newBinClause(
            Lit origLit1
            , Lit origLit2
            , Lit origLit3
            , Lit lit1
            , Lit lit2
            , bool red
        );
        void updateStatsFromImplStats();

        bool handleUpdatedClause(
            Clause& c
            , const Lit origLit1
            , const Lit origLit2
        );

         //While replacing the implicit clauses we cannot enqeue
        vector<Lit> delayedEnqueue;

        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        vector<LaterAddBinXor> laterAddBinXor;

        //Mapping tables
        vector<Lit> table; ///<Stores which variables have been replaced by which literals. Index by: table[VAR]
        map<Var, vector<Var> > reverseTable; ///<mapping of variable to set of variables it replaces

        //Stats
        void printReplaceStats() const;
        uint64_t replacedVars; ///<Num vars replaced during var-replacement
        uint64_t lastReplacedVars;
        Stats runStats;
        Stats globalStats;
};

inline size_t VarReplacer::getNumReplacedVars() const
{
    return replacedVars;
}

inline size_t VarReplacer::getNumLastReplacedVars() const
{
    return lastReplacedVars;
}

inline size_t VarReplacer::getNewToReplaceVars() const
{
    return replacedVars-lastReplacedVars;
}

inline const vector<Lit>& VarReplacer::getReplaceTable() const
{
    return table;
}

inline Lit VarReplacer::getLitReplacedWith(const Lit lit) const
{
    return table[lit.var()] ^ lit.sign();
}

inline Var VarReplacer::getVarReplacedWith(const Var var) const
{
    return table[var].var();
}

inline bool VarReplacer::replacingVar(const Var var) const
{
    return (reverseTable.find(var) != reverseTable.end());
}

inline size_t VarReplacer::getNumTrees() const
{
    return reverseTable.size();
}

inline const map<Var, vector<Var> >& VarReplacer::getReverseTable() const
{
    return reverseTable;
}

inline const VarReplacer::Stats& VarReplacer::getStats() const
{
    return globalStats;
}

inline bool VarReplacer::isReplaced(const Var var) const
{
    return getReplaceTable()[var].var() != var;
}

inline bool VarReplacer::isReplaced(const Lit lit) const
{
    return isReplaced(lit.var());
}

} //end namespace

#endif //VARREPLACER_H
