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

#ifndef __CL_ABSTRACTION__H__
#define __CL_ABSTRACTION__H__

typedef uint32_t cl_abst_type;
static const int cl_abst_modulo = 29;

inline cl_abst_type abst_var(const uint32_t v)
{
    return 1UL << (v % cl_abst_modulo);
}

template <class T>
cl_abst_type calcAbstraction(const T& ps)
{
    cl_abst_type abstraction = 0;
    if (ps.size() > 100) {
        return ~((cl_abst_type)(0ULL));
    }

    for (auto l: ps)
        abstraction |= abst_var(l.var());

    return abstraction;
}

#endif //__CL_ABSTRACTION__H__
