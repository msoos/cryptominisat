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

#include "clauseusagestats.h"

#include <iostream>
#include <iomanip>

using std::cout;
using std::endl;

using namespace CMSat;

void ClauseUsageStats::print() const
{
    cout
    #ifdef STATS_NEEDED
    << " lits visit: "
    << std::setw(8) << sumLitVisited/1000UL
    << "K"

    << " cls visit: "
    << std::setw(7) << sumLookedAt/1000UL
    << "K"
    #endif

    << " prop: "
    << std::setw(5) << sumProp/1000UL
    << "K"

    << " conf: "
    << std::setw(5) << sumConfl/1000UL
    << "K"

    << " UIP used: "
    << std::setw(5) << sumUsedUIP/1000UL
    << "K"
    << endl;
}
