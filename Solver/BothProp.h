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

#ifndef __BOTHPROP_H__
#define __BOTHPROP_H__

#include "BitArray.h"
#include "SolverTypes.h"
#include <vector>

class Solver;

class BothProp
{
public:
    BothProp(Solver* solver);

    bool tryBothProp();

private:
    Solver* solver;

    bool tryBoth(const Lit lit);

    //'Stats'
    size_t numFailed;
    size_t bothSameAdded;
    size_t binXorAdded;
    size_t tried;

    vector<uint32_t> propagatedBitSet;
    BitArray propagated; ///<These lits have been propagated by propagating the lit picked
    BitArray propValue; ///<The value (0 or 1) of the lits propagated set in "propagated"

    vector<Lit> BothSame;
    size_t extraTime;

    class BinXorToAdd
    {
        public:
            BinXorToAdd(const Lit _lit1, const Lit _lit2, const bool _isEqualTrue) :
                lit1(_lit1)
                , lit2(_lit2)
                , isEqualTrue(_isEqualTrue)
            {}
            Lit lit1;
            Lit lit2;
            bool isEqualTrue;
    };
    vector<BinXorToAdd> binXorToAdd;
    vector<Lit> bothSame;
    vector<Lit> tmpPs;
};

#endif //__BOTHPROP_H__
