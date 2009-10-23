/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

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
