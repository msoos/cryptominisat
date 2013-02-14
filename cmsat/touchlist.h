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


#include <vector>
#include "solvertypes.h"

class TouchList
{
public:
    void touch(const Lit lit)
    {
        touch(lit.var());
    }

    void touch(const Var var)
    {
        if (touchedBitset.size() <= var)
            touchedBitset.resize(var+1, 0);

        if (touchedBitset[var] == 0) {
            touched.push_back(var);
            touchedBitset[var] = 1;
        }
    }

    const vector<Var>& getTouchedList() const
    {
        return touched;
    }

    void clear()
    {
        //Clear touchedBitset
        for(vector<Var>::const_iterator
            it = touched.begin(), end = touched.end()
            ; it != end
            ; it++
        ) {
            touchedBitset[*it] = 0;
        }

        //Clear touched
        touched.clear();
    }

private:
    vector<Var> touched;
    vector<char> touchedBitset;
};