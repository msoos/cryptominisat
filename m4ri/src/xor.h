/**
 * \file xor.h
 * \brief Functions for adding vectors.
 *
 * \author Martin Albrecht <martinralbrecht@googlemail.com>
 *
 * \todo start counting at 0!
 */

#ifndef XOR_H
#define XOR_H

#ifdef HAVE_SSE2
#include <emmintrin.h>
#endif

 /*******************************************************************
 *
 *                 M4RI:  Linear Algebra over GF(2)
 *
 *    Copyright (C) 2008-2010  Martin Albrecht <martinralbrecht@googlemail.com>
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
 * Compute c[i] += t1[i] + t2[i] + t3[i] + t4[i] + t5[i] + t5[i] + t6[i] + t7[i] for 0 <= i < wide
 *
 * \todo the non SSE2 version of this code is slow, replace by code
 * from mzd_process_rows8
 */

#ifdef HAVE_SSE2
static inline void _mzd_combine8(word *c, word *t1, word *t2, word *t3, word *t4, word *t5, word *t6, word *t7, word *t8, size_t wide) {
  size_t i;
  /* assuming t1 ... t8 are aligned, but c might not be */
  if (ALIGNMENT(c,16)==0) {
    __m128i *__c = (__m128i*)c;
    __m128i *__t1 = (__m128i*)t1;
    __m128i *__t2 = (__m128i*)t2;
    __m128i *__t3 = (__m128i*)t3;
    __m128i *__t4 = (__m128i*)t4;
    __m128i *__t5 = (__m128i*)t5;
    __m128i *__t6 = (__m128i*)t6;
    __m128i *__t7 = (__m128i*)t7;
    __m128i *__t8 = (__m128i*)t8;
    const __m128i *eof = (__m128i*)((unsigned long)(c + wide) & ~0xF);
    __m128i xmm1;
    
    while(__c < eof) {
      xmm1 = _mm_xor_si128(*__c, *__t1++);
      xmm1 = _mm_xor_si128(xmm1, *__t2++);
      xmm1 = _mm_xor_si128(xmm1, *__t3++);
      xmm1 = _mm_xor_si128(xmm1, *__t4++);
      xmm1 = _mm_xor_si128(xmm1, *__t5++);
      xmm1 = _mm_xor_si128(xmm1, *__t6++);
      xmm1 = _mm_xor_si128(xmm1, *__t7++);
      xmm1 = _mm_xor_si128(xmm1, *__t8++);
      *__c++ = xmm1;
    }
    c  = (word*)__c;
    t1 = (word*)__t1;
    t2 = (word*)__t2;
    t3 = (word*)__t3;
    t4 = (word*)__t4;
    t5 = (word*)__t5;
    t6 = (word*)__t6;
    t7 = (word*)__t7;
    t8 = (word*)__t8;
    wide = ((sizeof(word)*wide)%16)/sizeof(word);
  }
  for(i=0; i<wide; i++) {
    c[i] ^= t1[i] ^ t2[i] ^ t3[i] ^ t4[i] ^ t5[i] ^ t6[i] ^ t7[i] ^ t8[i];
  }
}
#else

#define _mzd_combine8(c,t1,t2,t3,t4,t5,t6,t7,t8,wide) for(ii=0; ii<wide ; ii++) c[ii] ^= t1[ii] ^ t2[ii] ^ t3[ii] ^ t4[ii] ^ t5[ii] ^ t6[ii] ^ t7[ii] ^ t8[ii]

#endif

/**
 * Compute c[i] += t1[i] + t2[i] + t3[i] + t4[i] for 0 <= i < wide
 *
 * \todo the non SSE2 version of this code is slow, replace by code
 * from mzd_process_rows4
 */


#ifdef HAVE_SSE2
static inline void _mzd_combine4(word *c, word *t1, word *t2, word *t3, word *t4, size_t wide) {
  size_t i;
  /* assuming t1 ... t4 are aligned, but c might not be */
  if (ALIGNMENT(c,16)==0) {
    __m128i *__c = (__m128i*)c;
    __m128i *__t1 = (__m128i*)t1;
    __m128i *__t2 = (__m128i*)t2;
    __m128i *__t3 = (__m128i*)t3;
    __m128i *__t4 = (__m128i*)t4;
    const __m128i *eof = (__m128i*)((unsigned long)(c + wide) & ~0xF);
    __m128i xmm1;
    
    while(__c < eof) {
      xmm1 = _mm_xor_si128(*__c, *__t1++);
      xmm1 = _mm_xor_si128(xmm1, *__t2++);
      xmm1 = _mm_xor_si128(xmm1, *__t3++);
      xmm1 = _mm_xor_si128(xmm1, *__t4++);
      *__c++ = xmm1;
    }
    c  = (word*)__c;
    t1 = (word*)__t1;
    t2 = (word*)__t2;
    t3 = (word*)__t3;
    t4 = (word*)__t4;
    wide = ((sizeof(word)*wide)%16)/sizeof(word);
  }
  for(i=0; i<wide; i++) {
    c[i] ^= t1[i] ^ t2[i] ^ t3[i] ^ t4[i];
  }
}
#else

#define _mzd_combine4(c, t1, t2, t3, t4, wide) for(ii=0; ii<wide ; ii++) c[ii] ^= t1[ii] ^ t2[ii] ^ t3[ii] ^ t4[ii]

#endif //HAVE_SSE2

/**
 * Compute c[i] += t1[i] + t2[i] for 0 <= i < wide
 *
 * \todo the non SSE2 version of this code is slow, replace by code
 * from mzd_process_rows2
 */

#ifdef HAVE_SSE2
static inline void _mzd_combine2(word *c, word *t1, word *t2, size_t wide) {
  size_t i;
  /* assuming t1 ... t2 are aligned, but c might not be */
  if (ALIGNMENT(c,16)==0) {
    __m128i *__c = (__m128i*)c;
    __m128i *__t1 = (__m128i*)t1;
    __m128i *__t2 = (__m128i*)t2;
    const __m128i *eof = (__m128i*)((unsigned long)(c + wide) & ~0xF);
    __m128i xmm1;
    
    while(__c < eof) {
      xmm1 = _mm_xor_si128(*__c, *__t1++);
      xmm1 = _mm_xor_si128(xmm1, *__t2++);
      *__c++ = xmm1;
    }
    c  = (word*)__c;
    t1 = (word*)__t1;
    t2 = (word*)__t2;
    wide = ((sizeof(word)*wide)%16)/sizeof(word);
  }
  for(i=0; i<wide; i++) {
    c[i] ^= t1[i] ^ t2[i];
  }
}
#else

#define _mzd_combine2(c, t1, t2, wide) for(ii=0; ii<wide ; ii++) c[ii] ^= t1[ii] ^ t2[ii]

#endif //HAVE_SSE2


#ifdef M4RM_GRAY8
#define _MZD_COMBINE _mzd_combine8(c, t1, t2, t3, t4, t5, t6, t7, t8, wide)
#else //M4RM_GRAY8
#define _MZD_COMBINE _mzd_combine4(c, t1, t2, t3, t4, wide)
#endif //M4RM_GRAY8

#endif //XOR_H
