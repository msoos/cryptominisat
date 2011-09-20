#ifndef PARITY_H
#define PARITY_H
/*******************************************************************
*
*                 M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2008 David Harvey <dmharvey@cims.nyu.edu>
*
*  Distributed under the terms of the GNU General Public License (GPL)
*  version 2 or higher.
*
*    This code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*  The full text of the GPL is available at:
*
*                  http://www.gnu.org/licenses/
*
********************************************************************/

/**
 * \file parity.h
 *
 * \brief Compute the parity of 64 words in parallel.
 *
 * \author David Harvey
 */

/**
 * \brief Step for mixing two 64-bit words to compute their parity.
 */

#define MIX32(a, b) (((((a) << 32) ^ (a)) >> 32) + \
                     ((((b) >> 32) ^ (b)) << 32))

/**
 * \brief Step for mixing two 64-bit words to compute their parity.
 */

#define MIX16(a, b) (((((a) >> 16) ^ (a)) & 0x0000FFFF0000FFFFll) +     \
                     ((((b) << 16) ^ (b)) & 0xFFFF0000FFFF0000ll));
/**
 * \brief Step for mixing two 64-bit words to compute their parity.
 */

#define MIX8(a, b) (((((a) >> 8) ^ (a)) & 0x00FF00FF00FF00FFll) + \
                    ((((b) << 8) ^ (b)) & 0xFF00FF00FF00FF00ll));
/**
 * \brief Step for mixing two 64-bit words to compute their parity.
 */

#define MIX4(a, b) (((((a) >> 4) ^ (a)) & 0x0F0F0F0F0F0F0F0Fll) + \
                    ((((b) << 4) ^ (b)) & 0xF0F0F0F0F0F0F0F0ll));
/**
 * \brief Step for mixing two 64-bit words to compute their parity.
 */

#define MIX2(a, b) (((((a) >> 2) ^ (a)) & 0x3333333333333333ll) + \
                    ((((b) << 2) ^ (b)) & 0xCCCCCCCCCCCCCCCCll));
/**
 * \brief Step for mixing two 64-bit words to compute their parity.
 */

#define MIX1(a, b) (((((a) >> 1) ^ (a)) & 0x5555555555555555ll) + \
                    ((((b) << 1) ^ (b)) & 0xAAAAAAAAAAAAAAAAll));


/**
 * \brief See parity64.
 */

static inline word _parity64_helper(word* buf)
{
   word a0, a1, b0, b1, c0, c1;

   a0 = MIX32(buf[0x20], buf[0x00]);
   a1 = MIX32(buf[0x30], buf[0x10]);
   b0 = MIX16(a1, a0);

   a0 = MIX32(buf[0x28], buf[0x08]);
   a1 = MIX32(buf[0x38], buf[0x18]);
   b1 = MIX16(a1, a0);

   c0 = MIX8(b1, b0);

   a0 = MIX32(buf[0x24], buf[0x04]);
   a1 = MIX32(buf[0x34], buf[0x14]);
   b0 = MIX16(a1, a0);

   a0 = MIX32(buf[0x2C], buf[0x0C]);
   a1 = MIX32(buf[0x3C], buf[0x1C]);
   b1 = MIX16(a1, a0);

   c1 = MIX8(b1, b0);

   return MIX4(c1, c0);
}


/**
 * \brief Computes parity of each of buf[0], buf[1], ..., buf[63].
 * Returns single word whose bits are the parities of buf[0], ...,
 * buf[63].  Assumes 64-bit machine unsigned long.
 *
 * \param buf buffer of words of length 64
 */
static inline word parity64(word* buf)
{
   word d0, d1, e0, e1;

   d0 = _parity64_helper(buf);
   d1 = _parity64_helper(buf + 2);
   e0 = MIX2(d1, d0);

   d0 = _parity64_helper(buf + 1);
   d1 = _parity64_helper(buf + 3);
   e1 = MIX2(d1, d0);

   return MIX1(e1, e0);
}

#endif
