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

#ifndef MPZSTACK_H
#define MPZSTACK_H

#include <vector>
#include "gmp.h"
#include "assert.h"
#include <sys/types.h>

using std::vector;

class MpzStack
{
public:
    //MpzStack() :
            //num_bits(0) {}
    ~MpzStack();

    mpz_t& pop(const bool clear = true);
    inline void push(mpz_t& f) {
        real_stack.push_back(&f);
    }
    /*inline void setNumBits(const uint _num_bits)
    {
        assert(num_bits == 0 || num_bits >= _num_bits);
        num_bits = _num_bits + 150;
    }

    inline const bool getNumBits() const
    {
        return num_bits;
    }*/

private:
    vector<mpz_t*> real_stack;
    //uint num_bits;
};

#endif
