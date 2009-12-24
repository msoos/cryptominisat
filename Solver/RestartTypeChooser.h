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

#ifndef RESTARTTYPECHOOSER_H
#define RESTARTTYPECHOOSER_H

#include "SolverTypes.h"
#include <vector>
#include <sys/types.h>
using std::vector;

class Solver;

class RestartTypeChooser
{
    public:
        RestartTypeChooser(const Solver* const S);
        const RestartType choose();
        
    private:
        void calcHeap();
        const double avg() const;
        
        const Solver* const S;
        const uint32_t topX;
        const uint32_t limit;
        vector<Var> sameIns;
        
        vector<Var> firstVars, firstVarsOld;
};

#endif //RESTARTTYPECHOOSER_H
