/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************************/

#ifndef FAILEDVARSEARCHER_H
#define FAILEDVARSEARCHER_H

#include "SolverTypes.h"
#include "Clause.h"
#include "BitArray.h"
class Solver;

class TwoLongXor
{
    public:
        const bool operator==(const TwoLongXor& other) const
        {
            if (var[0] == other.var[0] && var[1] == other.var[1] && inverted == other.inverted)
                return true;
            return false;
        }
        const bool operator<(const TwoLongXor& other) const
        {
            if (var[0] < other.var[0]) return true;
            if (var[0] > other.var[0]) return false;
            
            if (var[1] < other.var[1]) return true;
            if (var[1] > other.var[1]) return false;
            
            if (inverted < other.inverted) return true;
            if (inverted > other.inverted) return false;
            
            return false;
        }
        
        Var var[2];
        bool inverted;
};

class FailedVarSearcher {
    public:
        FailedVarSearcher(Solver& _solver);
    
        const bool search(uint64_t numProps);
        
    private:
        const TwoLongXor getTwoLongXor(const XorClause& c);
        void addFromSolver(const vec<XorClause*>& cs);
        
        template<class T>
        void cleanAndAttachClauses(vec<T*>& cs);
        const bool cleanClause(Clause& ps);
        const bool cleanClause(XorClause& ps);
        
        Solver& solver;
        
        vec<uint32_t> xorClauseSizes;
        vector<vector<uint32_t> > occur;
        void removeVarFromXors(const Var var);
        void addVarFromXors(const Var var);
        BitArray xorClauseTouched;
        vec<uint32_t> investigateXor;
        
        bool finishedLastTime;
        uint32_t lastTimeWentUntil;
        double numPropsMultiplier;
        uint32_t lastTimeFoundTruths;
};


#endif //FAILEDVARSEARCHER_H