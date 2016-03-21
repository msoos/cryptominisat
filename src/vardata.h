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

#ifndef __VARDATA_H__
#define __VARDATA_H__

#include "constants.h"
#include "propby.h"
#include "avgcalc.h"

namespace CMSat
{
using namespace CMSat;

struct VarData
{
    ///contains the decision level at which the assignment was made.
    uint32_t level = 0;

    //Reason this got propagated. NULL means decision/toplevel
    PropBy reason = PropBy();

    ///The preferred polarity of each variable.
    bool polarity = false;

    ///Whether var has been eliminated (var-elim, different component, etc.)
    Removed removed = Removed::none;
    bool is_bva = false;
};

}

#endif //__VARDATA_H__
