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

