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

#ifndef VARREPLACER_H
#define VARREPLACER_H

#include <map>
#include <vector>

#include "constants.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "Vec.h"

//#define VERBOSE_DEBUG

using std::map;
using std::vector;
class SolutionExtender;
class ThreadControl;

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
        VarReplacer(ThreadControl* control);
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
        const Lit getVarReplacedWith(Var var) const;
        const Lit getLitReplacedWith(Lit lit) const;
        const map<Var, vector<Var> >&getReverseTable() const;
        bool varHasBeenReplaced(const Var var) const;
        bool replacingVar(const Var var) const;
        void newVar();
        bool addLaterAddBinXor();
        void updateVars(
            const vector<uint32_t>& outerToInter
            , const vector<uint32_t>& interToOuter
        );

        //stats
        size_t getTotalZeroDepthAssigns() const;
        size_t getTotalReplacedLits() const;
        double getTotalTime() const;
        size_t getNumLastReplacedVars() const;
        size_t getNewToReplaceVars() const;
        size_t getNumTrees() const;
        size_t getNumReplacedVars() const;

    private:
        ThreadControl* control; ///<The solver we are working with

        bool replace_set(vector<Clause*>& cs);
        bool replaceBins();
        bool handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2, const Lit origLit3);

        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        vector<LaterAddBinXor> laterAddBinXor;

        //Mapping tables
        vector<Lit> table; ///<Stores which variables have been replaced by which literals. Index by: table[VAR]
        map<Var, vector<Var> > reverseTable; ///<mapping of variable to set of variables it replaces

        //Stats
        size_t replacedLits; ///<Num literals replaced during var-replacement
        size_t replacedVars; ///<Num vars replaced during var-replacement
        size_t lastReplacedVars; ///<Last time performReplace() was called, "replacedVars" contained this
        size_t totalZeroDepthAssigns;
        size_t totalReplacedLits;
        double totalTime;
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

inline const Lit VarReplacer::getVarReplacedWith(const Var var) const
{
    return table[var];
}

inline const Lit VarReplacer::getLitReplacedWith(const Lit lit) const
{
    return table[lit.var()] ^ lit.sign();
}

inline bool VarReplacer::varHasBeenReplaced(const Var var) const
{
    return table[var].var() != var;
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

#endif //VARREPLACER_H
