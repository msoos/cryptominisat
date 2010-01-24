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
class Solver;

class FailedVarSearcher {
    public:
        FailedVarSearcher(Solver& _solver);
    
        const lbool search(const uint64_t numProps);
        
    private:
        Solver& solver;
        bool finishedLastTime;
        uint32_t lastTimeWentUntil;
};


#endif //FAILEDVARSEARCHER_H