/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
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

#ifndef _XOR_H_
#define _XOR_H_

#include "solvertypes.h"

#include <vector>
#include <set>
#include <iostream>
#include <algorithm>

using std::vector;

namespace CMSat {

class Xor
{
    public:
        Xor(const vector<Lit>& cl, const bool _rhs) :
            rhs(_rhs)
        {
            for (uint32_t i = 0; i < cl.size(); i++) {
                vars.push_back(cl[i].var());
            }
        }

        bool operator==(const Xor& other) const
        {
            return (rhs == other.rhs && vars == other.vars);
        }

        vector<uint32_t> vars;
        size_t size() const
        {
            return vars.size();
        }
        bool rhs;
        bool getRemoved() const
        {
            return removed;
        }


    private:
        bool removed;
};

inline std::ostream& operator<<(std::ostream& os, const Xor& thisXor)
{
    for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
        os << Lit(thisXor.vars[i], false);

        if (i+1 < thisXor.vars.size())
            os << " + ";
    }
    os << " =  " << std::boolalpha << thisXor.rhs << std::noboolalpha;

    return os;
}

}

#endif //_XOR_H_
