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

#ifndef __XORFINDERABST_H__
#define __XORFINDERABST_H__

namespace CMSat {

class OccSimplifier;
class Solver;

class XorFinderAbst
{
    public:
        virtual bool do_all_with_xors()
        {
            return true;
        }
        virtual ~XorFinderAbst()
        {}
        virtual size_t mem_used() const
        {
            return 0;
        }
};

}

#endif //__XORFINDERABST_H__
