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

#ifndef POPCNT__H
#define POPCNT__H

#if defined (_MSC_VER)
#include <intrin.h>
#endif

extern int popcnt_capable;

namespace CMSat {

#if defined (_MSC_VER)
int my_popcnt(uint64_t w)
{
    if (popcnt_capable) {
        return __popcnt(w);
    } else {
       uint64_t w1 = (w & 0x2222222222222222) + ((w+w) & 0x2222222222222222);
       uint64_t w2 = (w >> 1 & 0x2222222222222222) + (w >> 2 & 0x2222222222222222);
       w1 = w1 + (w1 >> 4) & 0x0f0f0f0f0f0f0f0f;
       w2 = w2 + (w2 >> 4) & 0x0f0f0f0f0f0f0f0f;
       return (w1 + w2) * 0x0101010101010101 >> 57;
    }
}
#else
#define my_popcnt(x) __builtin_popcount(x)
#endif

inline bool check_popcnt_capable()
{
    #if defined (_MSC_VER)
    int is_capable;
    int cpu_info[4];
    __cpuid(cpu_info, 1);
    is_capable = (cpu_info[2] >> 23) & 1;

    return is_capable;
    #else
    return true;
    #endif
}

}

#endif //POPCNT__H
