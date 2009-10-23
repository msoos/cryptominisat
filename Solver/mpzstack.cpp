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

#include "mpzstack.h"
#include "malloc.h"
#include <boost/foreach.hpp>
#include "gmp.h"

MpzStack::~MpzStack()
{
    typedef mpz_t* mpz_p;
    
    BOOST_FOREACH(mpz_p p, real_stack)
    {
        mpz_clear(*p);
        free(p);
    }
}

mpz_t& MpzStack::pop(const bool clear)
{
    static __thread uint stack_size;
    stack_size = real_stack.size();
    if (stack_size == 0) {
        const uint enlarge = 1;

        real_stack.resize(enlarge);
        //mpz_t* a = (mpz_t*)malloc(sizeof(mpz_t)*enlarge);
        //mpz_array_init(*a, enlarge, num_bits);
        //for (uint i = 0; i < enlarge; i++)
        //    real_stack[i] = a + i;

        mpz_t* n = (mpz_t*)malloc(sizeof(mpz_t));
        mpz_init(*n);
        real_stack[0] = n;
        
        stack_size = enlarge;
    }
    static __thread mpz_t* ret;
    ret = real_stack[stack_size-1];
    real_stack.pop_back();
    if (clear) mpz_set_ui(*ret, 0);
    return *ret;
}

