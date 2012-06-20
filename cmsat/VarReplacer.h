/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef VARREPLACER_H
#define VARREPLACER_H

#include "cmsat/constants.h"
#include <map>
#include <vector>

#include "cmsat/Solver.h"
#include "cmsat/SolverTypes.h"
#include "cmsat/Clause.h"
#include "cmsat/Vec.h"

namespace CMSat {

using std::map;
using std::vector;

/**
@brief Replaces variables with their anti/equivalents
*/
class VarReplacer
{
    public:
        VarReplacer(Solver& solver);
        ~VarReplacer();
        bool performReplace(const bool always = false);
        bool needsReplace();
        template<class T>
        bool replace(T& ps, const bool xorEqualFalse, const bool addBinAsLearnt = false, const bool addToWatchLists = true);

        void extendModelPossible() const;
        void extendModelImpossible(Solver& solver2) const;

        uint32_t getNumReplacedLits() const;
        uint32_t getNumReplacedVars() const;
        uint32_t getNumLastReplacedVars() const;
        uint32_t getNewToReplaceVars() const;
        uint32_t getNumTrees() const;
        vector<Var> getReplacingVars() const;
        const vector<Lit>& getReplaceTable() const;
        bool varHasBeenReplaced(const Var var) const;
        bool replacingVar(const Var var) const;
        void newVar();

        vec<char> cannot_eliminate;

        //No need to update, only stores binary clauses, that
        //have been allocated within pool
        //friend class ClauseAllocator;

    private:
        bool performReplaceInternal();

        bool replace_set(vec<Clause*>& cs);
        bool replaceBins();
        bool replace_set(vec<XorClause*>& cs);
        bool handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2, const Lit origLit3);
        bool handleUpdatedClause(XorClause& c, const Var origVar1, const Var origVar2);
        void addBinaryXorClause(Lit lit1, Lit lit2, const bool addBinAsLearnt = false);

        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);

        vector<Lit> table; ///<Stores which variables have been replaced by which literals. Index by: table[VAR]
        map<Var, vector<Var> > reverseTable; ///<mapping of variable to set of variables it replaces

        uint32_t replacedLits; ///<Num literals replaced during var-replacement
        uint32_t replacedVars; ///<Num vars replaced during var-replacement
        uint32_t lastReplacedVars; ///<Last time performReplace() was called, "replacedVars" contained this
        Solver& solver; ///<The solver we are working with
};

inline bool VarReplacer::performReplace(const bool always)
{
    //uint32_t limit = std::min((uint32_t)((double)solver.order_heap.size()*PERCENTAGEPERFORMREPLACE), FIXCLEANREPLACE);
    uint32_t limit = (uint32_t)((double)solver.order_heap.size()*PERCENTAGEPERFORMREPLACE);
    if ((always && getNewToReplaceVars() > 0) || getNewToReplaceVars() > limit)
        return performReplaceInternal();

    return true;
}

inline bool VarReplacer::needsReplace()
{
    uint32_t limit = (uint32_t)((double)solver.order_heap.size()*PERCENTAGEPERFORMREPLACE);
    return (getNewToReplaceVars() > limit);
}

inline uint32_t VarReplacer::getNumReplacedLits() const
{
    return replacedLits;
}

inline uint32_t VarReplacer::getNumReplacedVars() const
{
    return replacedVars;
}

inline uint32_t VarReplacer::getNumLastReplacedVars() const
{
    return lastReplacedVars;
}

inline uint32_t VarReplacer::getNewToReplaceVars() const
{
    return replacedVars-lastReplacedVars;
}

inline const vector<Lit>& VarReplacer::getReplaceTable() const
{
    return table;
}

inline bool VarReplacer::varHasBeenReplaced(const Var var) const
{
    return table[var].var() != var;
}

inline bool VarReplacer::replacingVar(const Var var) const
{
    return (reverseTable.find(var) != reverseTable.end());
}

inline uint32_t VarReplacer::getNumTrees() const
{
    return reverseTable.size();
}

}

#endif //VARREPLACER_H
