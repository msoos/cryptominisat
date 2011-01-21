/**************************************************************************
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
*****************************************************************************/

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "Vec.h"
#include "SolverTypes.h"

#include <vector>
#include <set>

class BinClause {
    public:
        BinClause() {};
        BinClause(const Lit _lit1, const Lit _lit2, const bool _learnt = true) :
        lit1(_lit1)
        , lit2(_lit2)
        , learnt(_learnt)
        {}
        Lit lit1;
        Lit lit2;
        bool learnt;
};

class TriClause {
    public:
        TriClause() {};
        TriClause(const Lit _lit1, const Lit _lit2, const Lit _lit3, const bool _learnt = true) :
            lit1(_lit1)
            , lit2(_lit2)
            , lit3(_lit3)
            , learnt(_learnt)
        {
            assert(lit1 < lit2);
            assert(lit2 < lit3);
        }
        Lit lit1;
        Lit lit2;
        Lit lit3;
        bool learnt;
};

class SharedData
{
    public:
        SharedData() :
            threadAddingVars(0)
            , EREnded(false)
            , numNewERVars(0)
            , lastNewERVars(0)
        {}
        vec<lbool> value;
        std::vector<std::vector<BinClause> > bins;
        std::vector<std::vector<TriClause> > tris;

        int       threadAddingVars;
        bool      EREnded;
        std::set<int>  othersSyncedER;
        uint32_t  numNewERVars;
        uint32_t  lastNewERVars;
};

#endif //SHARED_DATA_H
