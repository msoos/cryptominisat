/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Vegard Nossum. All rights reserved.
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

#include "solvertypes.h"

namespace CMSat
{

// Use 1G extra for clause re-learning bitmap.
static const uint32_t hash_bits = 28;
static const uint32_t hash_size = 1 << hash_bits;
static const uint32_t hash_mask = hash_size - 1;

static inline uint64_t rotl(uint64_t x, uint64_t n)
{
        return (x << n) | (x >> (8 * sizeof(x) - n));
}

#define HASH_MULT_CONST 0x61C8864680B583EBULL

static inline uint64_t clause_hash(vector<Lit> &clause)
{
    //assert((sizeof(Lit) * clause.size()) % sizeof(unsigned long) == 0);

    uint64_t x = 0;
    uint64_t y = 0;

    for (const Lit l: clause) {
        x = x ^ l.toInt();
        y = x ^ y;
        x = rotl(x, 12);
        x = x + y;
        y = rotl(y, 45);
        y = 9 * y;
    }

    y = y ^ (x * HASH_MULT_CONST);
    y = y * HASH_MULT_CONST;
    return y;
}

}
