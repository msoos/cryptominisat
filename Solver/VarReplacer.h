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
        const bool performReplace();
        const bool needsReplace();
        const bool replace(Lit lit1, Lit lit2, const bool xorEqualFalse);

        void extendModel(SolutionExtender* extender) const;

        const uint32_t getNumReplacedLits() const;
        const uint32_t getNumReplacedVars() const;
        const uint32_t getNumLastReplacedVars() const;
        const uint32_t getNewToReplaceVars() const;
        const uint32_t getNumTrees() const;
        const vector<Var> getReplacingVars() const;
        const vector<Lit>& getReplaceTable() const;
        const map<Var, vector<Var> >&getReverseTable() const;
        const bool varHasBeenReplaced(const Var var) const;
        const bool replacingVar(const Var var) const;
        void newVar();
        const bool addLaterAddBinXor();

        //No need to update, only stores binary clauses, that
        //have been allocated within pool
        //friend class ClauseAllocator;

    private:
        const bool replace_set(vector<Clause*>& cs);
        const bool replaceBins();
        const bool handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2, const Lit origLit3);
        void addBinaryXorClause(Lit lit1, Lit lit2);

        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        vector<LaterAddBinXor> laterAddBinXor;

        vector<Lit> table; ///<Stores which variables have been replaced by which literals. Index by: table[VAR]
        map<Var, vector<Var> > reverseTable; ///<mapping of variable to set of variables it replaces

        uint32_t replacedLits; ///<Num literals replaced during var-replacement
        uint32_t replacedVars; ///<Num vars replaced during var-replacement
        uint32_t lastReplacedVars; ///<Last time performReplace() was called, "replacedVars" contained this
        ThreadControl* control; ///<The solver we are working with
};

inline const uint32_t VarReplacer::getNumReplacedLits() const
{
    return replacedLits;
}

inline const uint32_t VarReplacer::getNumReplacedVars() const
{
    return replacedVars;
}

inline const uint32_t VarReplacer::getNumLastReplacedVars() const
{
    return lastReplacedVars;
}

inline const uint32_t VarReplacer::getNewToReplaceVars() const
{
    return replacedVars-lastReplacedVars;
}

inline const vector<Lit>& VarReplacer::getReplaceTable() const
{
    return table;
}

inline const bool VarReplacer::varHasBeenReplaced(const Var var) const
{
    return table[var].var() != var;
}

inline const bool VarReplacer::replacingVar(const Var var) const
{
    return (reverseTable.find(var) != reverseTable.end());
}

inline const uint32_t VarReplacer::getNumTrees() const
{
    return reverseTable.size();
}

inline const map<Var, vector<Var> >& VarReplacer::getReverseTable() const
{
    return reverseTable;
}

#endif //VARREPLACER_H
