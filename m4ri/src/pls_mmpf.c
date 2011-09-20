/*******************************************************************
*
*                 M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2008-2010 Martin Albrecht <M.R.Albrecht@rhul.ac.uk>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "misc.h"

#ifdef HAVE_SSE2
#include <emmintrin.h>
#endif

#include "pls_mmpf.h"
#include "brilliantrussian.h"
#include "grayflex.h"

static inline size_t _max_value(size_t *data, size_t length) {
  size_t max = 0;
  for(size_t i=0; i<length; i++) {
    max = MAX(max, data[i]);
  }
  return max;
}

size_t _mzd_pls_submatrix(mzd_t *A, size_t start_row, size_t stop_row, size_t start_col, int k, mzp_t *P, mzp_t *Q, size_t *done, size_t *done_row)  {
  size_t i, l, curr_pos;
  int found;

  word bm[4*MAXKAY];
  size_t os[4*MAXKAY];

  for(curr_pos=0; curr_pos < (size_t)k; curr_pos++) {
    os[curr_pos] = (start_col+curr_pos)/RADIX;
    bm[curr_pos] = ONE<<(RADIX-(start_col+curr_pos)%RADIX-1);
    found = 0;
    /* search for some pivot */
    for(i = start_row + curr_pos; i < stop_row; i++) {
      const word tmp = mzd_read_bits(A, i, start_col, curr_pos+1);
      if(tmp) {
        word *Arow = A->rows[i];
        /* clear before but preserve transformation matrix */
        for (l=0; l<curr_pos; l++)
          if(done[l] < i) {
            if(Arow[os[l]] & bm[l])
              mzd_row_add_offset(A, i, start_row + l, start_col + l + 1);
            done[l] = i; /* encode up to which row we added for l already */
          }
        if(mzd_read_bit(A, i, start_col + curr_pos)) {
          found = 1;
          break;
        }
      }
    }
    if(!found) {
      break;
    }

    P->values[start_row + curr_pos] = i;
    mzd_row_swap(A, i, start_row + curr_pos);

    Q->values[start_row + curr_pos] = start_col + curr_pos;
    done[curr_pos] = i;
  }
  
  /* finish submatrix */
  *done_row = _max_value(done, curr_pos);
  for(size_t c2=0; c2<curr_pos && start_col + c2 < A->ncols -1; c2++)
    for(size_t r2=done[c2]+1; r2<=*done_row; r2++)
      if(mzd_read_bit(A, r2, start_col + c2))
        mzd_row_add_offset(A, r2, start_row + c2, start_col + c2 + 1);
  return curr_pos;
}

/* create a table of all 2^k linear combinations */
void mzd_make_table_pls( mzd_t *M, size_t r, size_t c, int k, mzd_t *T, size_t *L) {
  assert(T->blocks[1].size == 0);
  const size_t blockoffset= c/RADIX;
  size_t i, rowneeded;
  size_t twokay= TWOPOW(k);
  size_t wide = T->width - blockoffset;

  word *ti, *ti1, *m;

  ti1 = T->rows[0] + blockoffset;
  ti = ti1 + T->width;
#ifdef HAVE_SSE2
  unsigned long incw = 0;
  if (T->width & 1) incw = 1;
  ti += incw;
#endif

  L[0]=0;
  for (i=1; i<twokay; i++) {
    rowneeded = r + codebook[k]->inc[i-1];
    m = M->rows[rowneeded] + blockoffset;

    /* Duff's device loop unrolling */
    register int n = (wide + 7) / 8;
    switch (wide % 8) {
    case 0: do { *(ti++) = *(m++) ^ *(ti1++);
    case 7:      *(ti++) = *(m++) ^ *(ti1++);
    case 6:      *(ti++) = *(m++) ^ *(ti1++);
    case 5:      *(ti++) = *(m++) ^ *(ti1++);
    case 4:      *(ti++) = *(m++) ^ *(ti1++);
    case 3:      *(ti++) = *(m++) ^ *(ti1++);
    case 2:      *(ti++) = *(m++) ^ *(ti1++);
    case 1:      *(ti++) = *(m++) ^ *(ti1++);
      } while (--n > 0);
    }
#ifdef HAVE_SSE2
    ti+=incw; ti1+=incw;
#endif
    ti += blockoffset;
    ti1 += blockoffset;

    /* U is a basis but not the canonical basis, so we need to read what
       element we just created from T*/
    L[(int)mzd_read_bits(T,i,c,k)] = i;
    
  }
  /* We need fix the table to update the transformation matrix
     correctly; e.g. if the first row has [1 0 1] and we clear a row
     below with [1 0 1] we need to encode that this row is cleared by
     adding the first row only ([1 0 0]).*/
  for(i=1; i < twokay; i++) {
    const word correction = (word)codebook[k]->ord[i];
    mzd_xor_bits(T, i,c, k, correction);
  }
}

void mzd_process_rows2_pls(mzd_t *M, size_t startrow, size_t stoprow, size_t startcol, int k, mzd_t *T0, size_t *L0, mzd_t *T1, size_t *L1) {
  size_t r;
  const int ka = k/2;
  const int kb = k-k/2;
  const size_t blocknuma=startcol/RADIX;
  const size_t blocknumb=(startcol+ka)/RADIX;
  const size_t blockoffset = blocknumb - blocknuma;
  size_t wide = M->width - blocknuma;

  if(wide < 3) {
    mzd_process_rows(M, startrow, stoprow, startcol, ka, T0, L0);
    mzd_process_rows(M, startrow, stoprow, startcol + ka, kb, T1, L1);
    return;
  }

  wide -= 2;
#ifdef HAVE_OPENMP
#pragma omp parallel for private(r) shared(startrow, stoprow) schedule(dynamic,32) if(stoprow-startrow > 128)
#endif
  for(r=startrow; r<stoprow; r++) {
    const int x0 = L0[ (int)mzd_read_bits(M, r, startcol, ka) ];
    word *t0 = T0->rows[x0] + blocknuma;
    word *m0 = M->rows[r+0] + blocknuma;
    m0[0] ^= t0[0];
    m0[1] ^= t0[1];
    const int x1 = L1[ (int)mzd_read_bits(M, r, startcol+ka, kb) ];
    word *t1 = T1->rows[x1] + blocknumb;
    for(size_t i=blockoffset; i<2; i++) {
      m0[i] ^= t1[i-blockoffset];
    }

    t0+=2;
    t1+=2-blockoffset;
    m0+=2;

    register int n = (wide + 7) / 8;
    switch (wide % 8) {
    case 0: do { *m0++ ^= *t0++ ^ *t1++;
      case 7:    *m0++ ^= *t0++ ^ *t1++;
      case 6:    *m0++ ^= *t0++ ^ *t1++;
      case 5:    *m0++ ^= *t0++ ^ *t1++;
      case 4:    *m0++ ^= *t0++ ^ *t1++;
      case 3:    *m0++ ^= *t0++ ^ *t1++;
      case 2:    *m0++ ^= *t0++ ^ *t1++;
      case 1:    *m0++ ^= *t0++ ^ *t1++;
      } while (--n > 0);
    }
  }
}

void mzd_process_rows3_pls(mzd_t *M, size_t startrow, size_t stoprow, size_t startcol, int k, mzd_t *T0, size_t *L0, mzd_t *T1, size_t *L1, mzd_t *T2, size_t *L2) {
  size_t r;

  const int rem = k%3;
  const int ka = k/3 + ((rem>=2) ? 1 : 0);
  const int kb = k/3 + ((rem>=1) ? 1 : 0);
  const int kc = k/3;
  const size_t blocknuma=startcol/RADIX;
  const size_t blocknumb=(startcol+ka)/RADIX;
  const size_t blocknumc=(startcol+ka+kb)/RADIX;
  const size_t blockoffsetb = blocknumb - blocknuma;
  const size_t blockoffsetc = blocknumc - blocknuma;
  size_t wide = M->width - blocknuma;

  if(wide < 4) {
    mzd_process_rows(M, startrow, stoprow, startcol, ka, T0, L0);
    mzd_process_rows(M, startrow, stoprow, startcol + ka, kb, T1, L1);
    mzd_process_rows(M, startrow, stoprow, startcol + ka + kb, kc, T2, L2);
    return;
  }

  wide -= 3;
#ifdef HAVE_OPENMP
#pragma omp parallel for private(r) shared(startrow, stoprow) schedule(dynamic,32) if(stoprow-startrow > 128)
#endif
  for(r=startrow; r<stoprow; r++) {
    const int x0 = L0[ (int)mzd_read_bits(M, r, startcol, ka) ];
    word *t0 = T0->rows[x0] + blocknuma;
    word *m0 = M->rows[r] + blocknuma;
    m0[0] ^= t0[0];
    m0[1] ^= t0[1];
    m0[2] ^= t0[2];

    t0+=3;

    const int x1 = L1[ (int)mzd_read_bits(M, r, startcol+ka, kb) ];
    word *t1 = T1->rows[x1] + blocknumb;
    for(size_t i=blockoffsetb; i<3; i++) {
      m0[i] ^= t1[i-blockoffsetb];
    }
    t1+=3-blockoffsetb;

    const int x2 = L2[ (int)mzd_read_bits(M, r, startcol+ka+kb, kc) ];
    word *t2 = T2->rows[x2] + blocknumc;
    for(size_t i=blockoffsetc; i<3; i++) {
      m0[i] ^= t2[i-blockoffsetc];
    }
    t2+=3-blockoffsetc;

    m0+=3;

    register int n = (wide + 7) / 8;
    switch (wide % 8) {
    case 0: do { *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      case 7:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      case 6:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      case 5:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      case 4:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      case 3:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      case 2:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      case 1:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++;
      } while (--n > 0);
    }
  }
}

void mzd_process_rows4_pls(mzd_t *M, size_t startrow, size_t stoprow, size_t startcol, int k, mzd_t *T0, size_t *L0, mzd_t *T1, size_t *L1, mzd_t *T2, size_t *L2, mzd_t *T3, size_t *L3) {
  size_t r;

  const int rem = k%4;
  const int ka = k/4 + ((rem>=3) ? 1 : 0);
  const int kb = k/4 + ((rem>=2) ? 1 : 0);
  const int kc = k/4 + ((rem>=1) ? 1 : 0);
  const int kd = k/4;
  const size_t blocknuma=startcol/RADIX;
  const size_t blocknumb=(startcol+ka)/RADIX;
  const size_t blocknumc=(startcol+ka+kb)/RADIX;
  const size_t blocknumd=(startcol+ka+kb+kc)/RADIX;
  const size_t blockoffsetb = blocknumb - blocknuma;
  const size_t blockoffsetc = blocknumc - blocknuma;
  const size_t blockoffsetd = blocknumd - blocknuma;
  size_t wide = M->width - blocknuma;

  if(wide < 5) {
    mzd_process_rows(M, startrow, stoprow, startcol, ka, T0, L0);
    mzd_process_rows(M, startrow, stoprow, startcol + ka, kb, T1, L1);
    mzd_process_rows(M, startrow, stoprow, startcol + ka + kb, kc, T2, L2);
    mzd_process_rows(M, startrow, stoprow, startcol + ka + kb + kc, kd, T3, L3);
    return;
  }
  wide -= 4;
#ifdef HAVE_OPENMP
#pragma omp parallel for private(r) shared(startrow, stoprow) schedule(dynamic,32) if(stoprow-startrow > 128)
#endif
  for(r=startrow; r<stoprow; r++) {
    const int x0 = L0[ (int)mzd_read_bits(M, r, startcol, ka) ];
    word *t0 = T0->rows[x0] + blocknuma;
    word *m0 = M->rows[r] + blocknuma;
    m0[0] ^= t0[0];
    m0[1] ^= t0[1];
    m0[2] ^= t0[2];
    m0[3] ^= t0[3];

    t0+=4;

    const int x1 = L1[ (int)mzd_read_bits(M, r, startcol+ka, kb) ];
    word *t1 = T1->rows[x1] + blocknumb;
    for(size_t i=blockoffsetb; i<4; i++) {
      m0[i] ^= t1[i-blockoffsetb];
    }
    t1+=4-blockoffsetb;

    const int x2 = L2[ (int)mzd_read_bits(M, r, startcol+ka+kb, kc) ];
    word *t2 = T2->rows[x2] + blocknumc;
    for(size_t i=blockoffsetc; i<4; i++) {
      m0[i] ^= t2[i-blockoffsetc];
    }
    t2+=4-blockoffsetc;

    const int x3 = L3[ (int)mzd_read_bits(M, r, startcol+ka+kb+kc, kd) ];
    word *t3 = T3->rows[x3] + blocknumd;
    for(size_t i=blockoffsetd; i<4; i++) {
      m0[i] ^= t3[i-blockoffsetd];
    }
    t3+=4-blockoffsetd;

    m0+=4;

    register int n = (wide + 7) / 8;
    switch (wide % 8) {
    case 0: do { *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      case 7:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      case 6:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      case 5:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      case 4:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      case 3:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      case 2:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      case 1:    *m0++ ^= *t0++ ^ *t1++ ^ *t2++ ^ *t3++;
      } while (--n > 0);
    }
  }
}

/* extract U from A for table creation */
mzd_t *_mzd_pls_to_u(mzd_t *U, mzd_t *A, size_t r, size_t c, int k) {
  /* this function call is now rather cheap, but it could be avoided
     completetly if needed */
  assert(U->offset == 0);
  assert(A->offset == 0);
  size_t i, j;
  size_t startcol = (c/RADIX)*RADIX;
  mzd_submatrix(U, A, r, 0, r+k, A->ncols);

  for(i=0; i<(size_t)k; i++)
    for(j=startcol; j<c+i; j++) 
      mzd_write_bit(U, i, j,  0);
  return U;
}

/* method of many people factorisation */
size_t _mzd_pls_mmpf(mzd_t *A, mzp_t * P, mzp_t * Q, int k) {
  assert(A->offset == 0);
  const size_t nrows = A->nrows;//mzd_first_zero_row(A);
  const size_t ncols = A->ncols; 
  size_t curr_row = 0;
  size_t curr_col = 0;
  size_t kbar = 0;
  size_t done_row = 0;

  if(k == 0) {
    k = m4ri_opt_k(nrows, ncols, 0);
    if (k>=7)
      k = 7;
    if ( (4*(1<<k)*A->ncols / 8.0) > CPU_L2_CACHE / 2.0 )
      k -= 1;
  }

  int kk = 4*k;

  for(size_t i = 0; i<ncols; i++) 
    Q->values[i] = i;

  for(size_t i = 0; i<A->nrows; i++)
    P->values[i] = i;

  mzd_t *T0 = mzd_init(TWOPOW(k), ncols);
  mzd_t *T1 = mzd_init(TWOPOW(k), ncols);
  mzd_t *T2 = mzd_init(TWOPOW(k), ncols);
  mzd_t *T3 = mzd_init(TWOPOW(k), ncols);
  mzd_t *U = mzd_init(kk, ncols);

  size_t *L0 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L1 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L2 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *L3 = (size_t *)m4ri_mm_calloc(TWOPOW(k), sizeof(size_t));
  size_t *done = (size_t *)m4ri_mm_malloc(kk * sizeof(size_t));

  while(curr_col < ncols && curr_row < nrows) {
    if(curr_col + kk > ncols)
      kk = ncols - curr_col;

    /* 1. compute LQUP factorisation for a kxk submatrix */
    kbar = _mzd_pls_submatrix(A, curr_row, nrows, curr_col, kk, P, Q, done, &done_row);

    /* 2. extract U */
    _mzd_pls_to_u(U, A, curr_row, curr_col, kbar);

    if(kbar > (size_t)3*k) {
      const int rem = kbar%4;
  
      const int ka = kbar/4 + ((rem>=3) ? 1 : 0);
      const int kb = kbar/4 + ((rem>=2) ? 1 : 0);
      const int kc = kbar/4 + ((rem>=1) ? 1 : 0);
      const int kd = kbar/4;

      if (kbar==kk) {
        /* 2. generate table T */
        mzd_make_table_pls(U, 0,          curr_col,          ka, T0, L0);
        mzd_make_table_pls(U, 0+ka,       curr_col+ka,       kb, T1, L1);
        mzd_make_table_pls(U, 0+ka+kb,    curr_col+ka+kb,    kc, T2, L2);
        mzd_make_table_pls(U, 0+ka+kb+kc, curr_col+ka+kb+kc, kd, T3, L3);
        /* 3. use that table to process remaining rows below */
        mzd_process_rows4_pls(A, done_row + 1, nrows, curr_col, kbar, T0, L0, T1, L1, T2, L2, T3, L3);
      } else {
        curr_col += 1; 
      }

    } else if(kbar > (size_t)2*k) {
      const int rem = kbar%3;

      const int ka = kbar/3 + ((rem>=2) ? 1 : 0);
      const int kb = kbar/3 + ((rem>=1) ? 1 : 0);
      const int kc = kbar/3;

      if (kbar==kk) {
        /* 2. generate table T */
        mzd_make_table_pls(U, 0,       curr_col,       ka, T0, L0);
        mzd_make_table_pls(U, 0+ka,    curr_col+ka,    kb, T1, L1);
        mzd_make_table_pls(U, 0+ka+kb, curr_col+ka+kb, kc, T2, L2);
        /* 3. use that table to process remaining rows below */
        mzd_process_rows3_pls(A, done_row + 1, nrows, curr_col, kbar, T0, L0, T1, L1, T2, L2);
      } else {
        curr_col += 1; 
      }

    } else if(kbar > (size_t)k) {
      const int ka = kbar/2;
      const int kb = kbar - ka;

      if(kbar==kk) {
        /* 2. generate table T */
        mzd_make_table_pls(U, 0,    curr_col,    ka, T0, L0);
        mzd_make_table_pls(U, 0+ka, curr_col+ka, kb, T1, L1);
        /* 3. use that table to process remaining rows below */
        mzd_process_rows2_pls(A, done_row + 1, nrows, curr_col, kbar, T0, L0, T1, L1);
      } else {
        curr_col += 1; 
      }

    } else if(kbar > 0) {

      if(kbar==kk) {
        /* 2. generate table T */
        mzd_make_table_pls(U, 0, curr_col, kbar, T0, L0);
        /* 3. use that table to process remaining rows below */
        mzd_process_rows(A, done_row + 1, nrows, curr_col, kbar, T0, L0);
      } else {
        curr_col += 1; 
      }

    } else {
      curr_col += 1;
      size_t i = curr_row;
      size_t j = curr_col;
      int found = mzd_find_pivot(A, curr_row, curr_col, &i, &j);
      if(found) {
        P->values[curr_row] = i;
        Q->values[curr_row] = j;
        mzd_row_swap(A, curr_row, i);
        const size_t wrd = j/RADIX;
        const word bm = ONE<<(RADIX-(j%RADIX)-1);
        for(size_t l = curr_row+1; l<nrows; l++)
          if(A->rows[l][wrd] & bm)
            mzd_row_add_offset(A, l, curr_row, j + 1);
        curr_col = j + 1;
        curr_row++;
      } else {
        break;
      }
    }
    curr_col += kbar;
    curr_row += kbar;
    if (kbar > 0)
      if (kbar == kk && kk < 4*k)
        kk = kbar + 1;
      else
        kk = kbar;
    else if(kk>2)
      kk = kk/2;
  }

  /* Now compressing L*/
  for (size_t j = 0; j<curr_row; ++j){
    if (Q->values[j]>j) {
      mzd_col_swap_in_rows(A,Q->values[j], j, j, curr_row);
    }
  }
  mzp_t *Qbar = mzp_init_window(Q,0,curr_row);
  mzd_apply_p_right_trans_even_capped(A, Qbar, curr_row, 0);
  mzp_free_window(Qbar);

  mzd_free(U);
  mzd_free(T0);
  mzd_free(T1);
  mzd_free(T2);
  mzd_free(T3);
  m4ri_mm_free(L0);
  m4ri_mm_free(L1);
  m4ri_mm_free(L2);
  m4ri_mm_free(L3);
  m4ri_mm_free(done);
  return curr_row;
}

size_t _mzd_pluq_mmpf(mzd_t *A, mzp_t * P, mzp_t * Q, const int k) {
  size_t r  = _mzd_pls_mmpf(A,P,Q,k);
  mzd_apply_p_right_trans_tri(A, Q);
  return r;
}
