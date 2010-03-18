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

#ifndef PARTHANDLER_H
#define PARTHANDLER_H

#include "Solver.h"
#include "PartFinder.h"
#include "Vec.h"
#include "SolverTypes.h"

#include <map>
#include <vector>
using std::map;
using std::vector;
using std::pair;

class PartHandler
{
    public:
        PartHandler(Solver& solver);
        const bool handle();
        const vec<lbool>& getSavedState();
        void newVar();
        void addSavedState();
        
    private:
        struct sort_pred {
            bool operator()(const std::pair<int,int> &left, const std::pair<int,int> &right) {
                return left.second < right.second;
            }
        };
        
        //For moving clauses
        void moveClauses(vec<XorClause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder);
        void moveClauses(vec<Clause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder);
        void moveLearntClauses(vec<Clause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder);
        
        Solver& solver;
        vec<lbool> savedState;
};

inline const vec<lbool>& PartHandler::getSavedState()
{
    return savedState;
}

inline void PartHandler::newVar()
{
    savedState.push(l_Undef);
}



#endif //PARTHANDLER_H
