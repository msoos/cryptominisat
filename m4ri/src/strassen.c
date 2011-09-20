/*******************************************************************
*
*                 M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2008 Martin Albrecht <M.R.Albrecht@rhul.ac.uk>
*    Copyright (C) 2008 Clement Pernet <pernet@math.washington.edu>
*    Copyright (C) 2008 Marco Bodrato <bodrato@mail.dm.unipi.it>
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


#include "grayflex.h"
#include "strassen.h"
#include "misc.h"
#include "parity.h"
#define CLOSER(a,b,target) (abs((long)a-(long)target)<abs((long)b-(long)target))
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifdef HAVE_OPENMP
#include <omp.h>
#endif

/**
 * Simple blockwise product
 */
mzd_t *_mzd_addmul_mp_even(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff);


mzd_t *_mzd_mul_even_orig(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff) {
  size_t a,b,c;
  size_t anr, anc, bnr, bnc;
  
  a = A->nrows;
  b = A->ncols;
  c = B->ncols;

  if(C->nrows == 0 || C->ncols == 0)
    return C;

  /* handle case first, where the input matrices are too small already */
  if (CLOSER(A->nrows, A->nrows/2, cutoff) || CLOSER(A->ncols, A->ncols/2, cutoff) || CLOSER(B->ncols, B->ncols/2, cutoff)) {
    /* we copy the matrix first since it is only constant memory
       overhead and improves data locality, if you remove it make sure
       there are no speed regressions */
    /* C = _mzd_mul_m4rm(C, A, B, 0, TRUE); */
    /* return C; */
    mzd_t *Cbar = mzd_init(C->nrows, C->ncols);
    Cbar = _mzd_mul_m4rm(Cbar, A, B, 0, FALSE);
    mzd_copy(C, Cbar);
    mzd_free(Cbar);
    return C;
  }

  /* adjust cutting numbers to work on words */
  unsigned long mult = 1;
  long width = MIN(MIN(a,b),c);
  while (width > 2*cutoff) {
    width/=2;
    mult*=2;
  }
  a -= a%(RADIX*mult);
  b -= b%(RADIX*mult);
  c -= c%(RADIX*mult);

  anr = ((a/RADIX) >> 1) * RADIX;
  anc = ((b/RADIX) >> 1) * RADIX;
  bnr = anc;
  bnc = ((c/RADIX) >> 1) * RADIX;

  mzd_t *A00 = mzd_init_window(A,   0,   0,   anr,   anc);
  mzd_t *A01 = mzd_init_window(A,   0, anc,   anr, 2*anc);
  mzd_t *A10 = mzd_init_window(A, anr,   0, 2*anr,   anc);
  mzd_t *A11 = mzd_init_window(A, anr, anc, 2*anr, 2*anc);

  mzd_t *B00 = mzd_init_window(B,   0,   0,   bnr,   bnc);
  mzd_t *B01 = mzd_init_window(B,   0, bnc,   bnr, 2*bnc);
  mzd_t *B10 = mzd_init_window(B, bnr,   0, 2*bnr,   bnc);
  mzd_t *B11 = mzd_init_window(B, bnr, bnc, 2*bnr, 2*bnc);

  mzd_t *C00 = mzd_init_window(C,   0,   0,   anr,   bnc);
  mzd_t *C01 = mzd_init_window(C,   0, bnc,   anr, 2*bnc);
  mzd_t *C10 = mzd_init_window(C, anr,   0, 2*anr,   bnc);
  mzd_t *C11 = mzd_init_window(C, anr, bnc, 2*anr, 2*bnc);
  
  /**
   * \note See Jean-Guillaume Dumas, Clement Pernet, Wei Zhou; "Memory
   * efficient scheduling of Strassen-Winograd's matrix multiplication
   * algorithm"; http://arxiv.org/pdf/0707.2347v3 for reference on the
   * used operation scheduling.
   */

  /* change this to mzd_init(anr, MAX(bnc,anc)) to fix the todo below */
  mzd_t *X0 = mzd_init(anr, anc);
  mzd_t *X1 = mzd_init(bnr, bnc);
  
  _mzd_add(X0, A00, A10);              /*1    X0 = A00 + A10 */
  _mzd_add(X1, B11, B01);              /*2    X1 = B11 + B01 */
  _mzd_mul_even_orig(C10, X0, X1, cutoff);  /*3   C10 = X0*X1 */

  _mzd_add(X0, A10, A11);              /*4    X0 = A10 + A11 */
  _mzd_add(X1, B01, B00);              /*5    X1 = B01 + B00*/
  _mzd_mul_even_orig(C11, X0, X1, cutoff);  /*6   C11 = X0*X1 */

  _mzd_add(X0, X0, A00);               /*7    X0 = X0 + A00 */
  _mzd_add(X1, X1, B11);               /*8    X1 = B11 + X1 */
  _mzd_mul_even_orig(C01, X0, X1, cutoff);  /*9   C01 = X0*X1 */

  _mzd_add(X0, X0, A01);               /*10   X0 = A01 + X0 */
  _mzd_mul_even_orig(C00, X0, B11, cutoff); /*11  C00 = X0*B11 */

  /**
   * \todo ideally we would use the same X0 throughout the function
   * but some called function doesn't like that and we end up with a
   * wrong result if we use virtual X0 matrices. Ideally, this should
   * be fixed not worked around. The check whether the bug has been
   * fixed, use only one X0 and check if mzd_mul(4096, 3528, 4096,
   * 1024) still returns the correct answer.
   */

  mzd_free(X0);
  X0 = mzd_mul(NULL, A00, B00, cutoff);/*12  X0 = A00*B00*/

  _mzd_add(C01, X0, C01);              /*13  C01 =  X0 + C01 */
  _mzd_add(C10, C01, C10);             /*14  C10 = C01 + C10 */
  _mzd_add(C01, C01, C11);             /*15  C01 = C01 + C11 */
  _mzd_add(C11, C10, C11);             /*16  C11 = C10 + C11 */
  _mzd_add(C01, C01, C00);             /*17  C01 = C01 + C00 */
  _mzd_add(X1, X1, B10);               /*18   X1 = X1 + B10 */
  _mzd_mul_even_orig(C00, A11, X1, cutoff); /*19  C00 = A11*X1 */

  _mzd_add(C10, C10, C00);             /*20  C10 = C10 + C00 */
  _mzd_mul_even_orig(C00, A01, B10, cutoff);/*21  C00 = A01*B10 */

  _mzd_add(C00, C00, X0);              /*22  C00 = X0 + C00 */

  /* deal with rest */
  if (B->ncols > (int)(2*bnc)) {
    mzd_t *B_last_col = mzd_init_window(B, 0, 2*bnc, A->ncols, B->ncols); 
    mzd_t *C_last_col = mzd_init_window(C, 0, 2*bnc, A->nrows, C->ncols);
    _mzd_mul_m4rm(C_last_col, A, B_last_col, 0, TRUE);
    mzd_free_window(B_last_col);
    mzd_free_window(C_last_col);
  }
  if (A->nrows > (int)(2*anr)) {
    mzd_t *A_last_row = mzd_init_window(A, 2*anr, 0, A->nrows, A->ncols);
    mzd_t *C_last_row = mzd_init_window(C, 2*anr, 0, C->nrows, C->ncols);
    _mzd_mul_m4rm(C_last_row, A_last_row, B, 0, TRUE);
    mzd_free_window(A_last_row);
    mzd_free_window(C_last_row);
  }
  if (A->ncols > (int)(2*anc)) {
    mzd_t *A_last_col = mzd_init_window(A,     0, 2*anc, 2*anr, A->ncols);
    mzd_t *B_last_row = mzd_init_window(B, 2*bnr,     0, B->nrows, 2*bnc);
    mzd_t *C_bulk = mzd_init_window(C, 0, 0, 2*anr, bnc*2);
    mzd_addmul_m4rm(C_bulk, A_last_col, B_last_row, 0);
    mzd_free_window(A_last_col);
    mzd_free_window(B_last_row);
    mzd_free_window(C_bulk);
  }

  /* clean up */
  mzd_free_window(A00); mzd_free_window(A01);
  mzd_free_window(A10); mzd_free_window(A11);

  mzd_free_window(B00); mzd_free_window(B01);
  mzd_free_window(B10); mzd_free_window(B11);

  mzd_free_window(C00); mzd_free_window(C01);
  mzd_free_window(C10); mzd_free_window(C11);
  
  mzd_free(X0);
  mzd_free(X1);

  return C;
}


mzd_t *_mzd_mul_even(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff) {
  size_t m,k,n;
  size_t mmm, kkk, nnn;
  
  if(C->nrows == 0 || C->ncols == 0)
    return C;

  m = A->nrows;
  k = A->ncols;
  n = B->ncols;

  /* handle case first, where the input matrices are too small already */
  if (CLOSER(m, m/2, cutoff) || CLOSER(k, k/2, cutoff) || CLOSER(n, n/2, cutoff)) {
    /* we copy the matrix first since it is only constant memory
       overhead and improves data locality, if you remove it make sure
       there are no speed regressions */
    /* C = _mzd_mul_m4rm(C, A, B, 0, TRUE); */
    mzd_t *Cbar = mzd_init(m, n);
    _mzd_mul_m4rm(Cbar, A, B, 0, FALSE);
    mzd_copy(C, Cbar);
    mzd_free(Cbar);
    return C;
  }

#ifdef HAVE_OPENMP
  if (omp_get_num_threads() < omp_get_max_threads()) {
    mzd_set_ui(C, 0);
    return _mzd_addmul_mp_even(C, A, B, cutoff);
  }
#endif

  /* adjust cutting numbers to work on words */
  {
    unsigned long mult = RADIX;
    unsigned long width = MIN(MIN(m,n),k)/2;
    while (width > (unsigned long)cutoff) {
      width>>=1;
      mult<<=1;
    }

    mmm = (((m - m%mult)/RADIX) >> 1) * RADIX;
    kkk = (((k - k%mult)/RADIX) >> 1) * RADIX;
    nnn = (((n - n%mult)/RADIX) >> 1) * RADIX;
  }
  /*         |A |   |B |   |C |
   * Compute |  | x |  | = |  | */
  {
    mzd_t *A11 = mzd_init_window(A,   0,   0,   mmm,   kkk);
    mzd_t *A12 = mzd_init_window(A,   0, kkk,   mmm, 2*kkk);
    mzd_t *A21 = mzd_init_window(A, mmm,   0, 2*mmm,   kkk);
    mzd_t *A22 = mzd_init_window(A, mmm, kkk, 2*mmm, 2*kkk);

    mzd_t *B11 = mzd_init_window(B,   0,   0,   kkk,   nnn);
    mzd_t *B12 = mzd_init_window(B,   0, nnn,   kkk, 2*nnn);
    mzd_t *B21 = mzd_init_window(B, kkk,   0, 2*kkk,   nnn);
    mzd_t *B22 = mzd_init_window(B, kkk, nnn, 2*kkk, 2*nnn);

    mzd_t *C11 = mzd_init_window(C,   0,   0,   mmm,   nnn);
    mzd_t *C12 = mzd_init_window(C,   0, nnn,   mmm, 2*nnn);
    mzd_t *C21 = mzd_init_window(C, mmm,   0, 2*mmm,   nnn);
    mzd_t *C22 = mzd_init_window(C, mmm, nnn, 2*mmm, 2*nnn);
  
    /**
     * \note See Marco Bodrato; "A Strassen-like Matrix Multiplication
     * Suited for Squaring and Highest Power Computation";
     * http://bodrato.it/papres/#CIVV2008 for reference on the used
     * sequence of operations.
     */

    /* change this to mzd_init(mmm, MAX(nnn,kkk)) to fix the todo below */
    mzd_t *Wmk = mzd_init(mmm, kkk);
    mzd_t *Wkn = mzd_init(kkk, nnn);

    _mzd_add(Wkn, B22, B12);		 /* Wkn = B22 + B12 */
    _mzd_add(Wmk, A22, A12);		 /* Wmk = A22 + A12 */
    _mzd_mul_even(C21, Wmk, Wkn, cutoff);/* C21 = Wmk * Wkn */

    _mzd_add(Wmk, A22, A21);		 /* Wmk = A22 - A21 */
    _mzd_add(Wkn, B22, B21);		 /* Wkn = B22 - B21 */
    _mzd_mul_even(C22, Wmk, Wkn, cutoff);/* C22 = Wmk * Wkn */

    _mzd_add(Wkn, Wkn, B12);		 /* Wkn = Wkn + B12 */
    _mzd_add(Wmk, Wmk, A12);		 /* Wmk = Wmk + A12 */
    _mzd_mul_even(C11, Wmk, Wkn, cutoff);/* C11 = Wmk * Wkn */

    _mzd_add(Wmk, Wmk, A11);		 /* Wmk = Wmk - A11 */
    _mzd_mul_even(C12, Wmk, B12, cutoff);/* C12 = Wmk * B12 */
    _mzd_add(C12, C12, C22);		 /* C12 = C12 + C22 */

    /**
     * \todo ideally we would use the same Wmk throughout the function
     * but some called function doesn't like that and we end up with a
     * wrong result if we use virtual Wmk matrices. Ideally, this should
     * be fixed not worked around. The check whether the bug has been
     * fixed, use only one Wmk and check if mzd_mul(4096, 3528,
     * 4096, 2124) still returns the correct answer.
     */

    mzd_free(Wmk);
    Wmk = mzd_mul(NULL, A12, B21, cutoff);/*Wmk = A12 * B21 */

    _mzd_add(C11, C11, Wmk);		  /* C11 = C11 + Wmk */
    _mzd_add(C12, C11, C12);		  /* C12 = C11 - C12 */
    _mzd_add(C11, C21, C11);		  /* C11 = C21 - C11 */
    _mzd_add(Wkn, Wkn, B11);		  /* Wkn = Wkn - B11 */
    _mzd_mul_even(C21, A21, Wkn, cutoff);/* C21 = A21 * Wkn */
    mzd_free(Wkn);

    _mzd_add(C21, C11, C21);		  /* C21 = C11 - C21 */
    _mzd_add(C22, C22, C11);		  /* C22 = C22 + C11 */
    _mzd_mul_even(C11, A11, B11, cutoff);/* C11 = A11 * B11 */

    _mzd_add(C11, C11, Wmk);		  /* C11 = C11 + Wmk */

    /* clean up */
    mzd_free_window(A11); mzd_free_window(A12);
    mzd_free_window(A21); mzd_free_window(A22);

    mzd_free_window(B11); mzd_free_window(B12);
    mzd_free_window(B21); mzd_free_window(B22);

    mzd_free_window(C11); mzd_free_window(C12);
    mzd_free_window(C21); mzd_free_window(C22);

    mzd_free(Wmk);
  }
  /* deal with rest */
  nnn*=2;
  if (n > nnn) {
    /*         |AA|   | B|   | C|
     * Compute |AA| x | B| = | C| */
    mzd_t *B_last_col = mzd_init_window(B, 0, nnn, k, n); 
    mzd_t *C_last_col = mzd_init_window(C, 0, nnn, m, n);
    _mzd_mul_m4rm(C_last_col, A, B_last_col, 0, TRUE);
    mzd_free_window(B_last_col);
    mzd_free_window(C_last_col);
  }
  mmm*=2;
  if (m > mmm) {
    /*         |  |   |B |   |  |
     * Compute |AA| x |B | = |C | */
    mzd_t *A_last_row = mzd_init_window(A, mmm, 0, m, k);
    mzd_t *B_first_col= mzd_init_window(B,   0, 0, k, nnn);
    mzd_t *C_last_row = mzd_init_window(C, mmm, 0, m, nnn);
    _mzd_mul_m4rm(C_last_row, A_last_row, B_first_col, 0, TRUE);
    mzd_free_window(A_last_row);
    mzd_free_window(B_first_col);
    mzd_free_window(C_last_row);
  }
  kkk*=2;
  if (k > kkk) {
    /* Add to  |  |   | B|   |C |
     * result  |A | x |  | = |  | */
    mzd_t *A_last_col = mzd_init_window(A,   0, kkk, mmm, k);
    mzd_t *B_last_row = mzd_init_window(B, kkk,   0,   k, nnn);
    mzd_t *C_bulk = mzd_init_window(C, 0, 0, mmm, nnn);
    mzd_addmul_m4rm(C_bulk, A_last_col, B_last_row, 0);
    mzd_free_window(A_last_col);
    mzd_free_window(B_last_row);
    mzd_free_window(C_bulk);
  }

  return C;
}

mzd_t *_mzd_sqr_even(mzd_t *C, mzd_t *A, int cutoff) {
  size_t m;
  size_t mmm;
  
  m = A->nrows;
  /* handle case first, where the input matrices are too small already */
  if (CLOSER(m, m/2, cutoff)) {
    /* we copy the matrix first since it is only constant memory
       overhead and improves data locality, if you remove it make sure
       there are no speed regressions */
    /* C = _mzd_mul_m4rm(C, A, B, 0, TRUE); */
    mzd_t *Cbar = mzd_init(m, m);
    _mzd_mul_m4rm(Cbar, A, A, 0, FALSE);
    mzd_copy(C, Cbar);
    mzd_free(Cbar);
    return C;
  }

  /* adjust cutting numbers to work on words */
  {
    unsigned long mult = RADIX;
    unsigned long width = m>>1;
    while (width > (unsigned long)cutoff) {
      width>>=1;
      mult<<=1;
    }

    mmm = (((m - m%mult)/RADIX) >> 1) * RADIX;
  }
  /*         |A |   |A |   |C |
   * Compute |  | x |  | = |  | */
  {
    mzd_t *A11 = mzd_init_window(A,   0,   0,   mmm,   mmm);
    mzd_t *A12 = mzd_init_window(A,   0, mmm,   mmm, 2*mmm);
    mzd_t *A21 = mzd_init_window(A, mmm,   0, 2*mmm,   mmm);
    mzd_t *A22 = mzd_init_window(A, mmm, mmm, 2*mmm, 2*mmm);

    mzd_t *C11 = mzd_init_window(C,   0,   0,   mmm,   mmm);
    mzd_t *C12 = mzd_init_window(C,   0, mmm,   mmm, 2*mmm);
    mzd_t *C21 = mzd_init_window(C, mmm,   0, 2*mmm,   mmm);
    mzd_t *C22 = mzd_init_window(C, mmm, mmm, 2*mmm, 2*mmm);
  
    /**
     * \note See Marco Bodrato; "A Strassen-like Matrix Multiplication
     * Suited for Squaring and Highest Power Computation";
     * http://bodrato.it/papres/#CIVV2008 for reference on the used
     * sequence of operations.
     */

    mzd_t *Wmk;
    mzd_t *Wkn = mzd_init(mmm, mmm);

    _mzd_add(Wkn, A22, A12);                 /* Wkn = A22 + A12 */
    _mzd_sqr_even(C21, Wkn, cutoff);     /* C21 = Wkn^2 */

    _mzd_add(Wkn, A22, A21);                 /* Wkn = A22 - A21 */
    _mzd_sqr_even(C22, Wkn, cutoff);     /* C22 = Wkn^2 */

    _mzd_add(Wkn, Wkn, A12);                 /* Wkn = Wkn + A12 */
    _mzd_sqr_even(C11, Wkn, cutoff);     /* C11 = Wkn^2 */

    _mzd_add(Wkn, Wkn, A11);                 /* Wkn = Wkn - A11 */
    _mzd_mul_even(C12, Wkn, A12, cutoff);/* C12 = Wkn * A12 */
    _mzd_add(C12, C12, C22);		  /* C12 = C12 + C22 */

    Wmk = mzd_mul(NULL, A12, A21, cutoff);/*Wmk = A12 * A21 */

    _mzd_add(C11, C11, Wmk);		  /* C11 = C11 + Wmk */
    _mzd_add(C12, C11, C12);		  /* C12 = C11 - C12 */
    _mzd_add(C11, C21, C11);		  /* C11 = C21 - C11 */
    _mzd_mul_even(C21, A21, Wkn, cutoff);/* C21 = A21 * Wkn */
    mzd_free(Wkn);

    _mzd_add(C21, C11, C21);		  /* C21 = C11 - C21 */
    _mzd_add(C22, C22, C11);		  /* C22 = C22 + C11 */
    _mzd_sqr_even(C11, A11, cutoff);     /* C11 = A11^2 */

    _mzd_add(C11, C11, Wmk);		  /* C11 = C11 + Wmk */

    /* clean up */
    mzd_free_window(A11); mzd_free_window(A12);
    mzd_free_window(A21); mzd_free_window(A22);

    mzd_free_window(C11); mzd_free_window(C12);
    mzd_free_window(C21); mzd_free_window(C22);

    mzd_free(Wmk);
  }
  /* deal with rest */
  mmm*=2;
  if (m > mmm) {
    /*         |AA|   | A|   | C|
     * Compute |AA| x | A| = | C| */
    {
      mzd_t *A_last_col = mzd_init_window(A, 0, mmm, m, m);
      mzd_t *C_last_col = mzd_init_window(C, 0, mmm, m, m);
      _mzd_mul_m4rm(C_last_col, A, A_last_col, 0, TRUE);
      mzd_free_window(A_last_col);
      mzd_free_window(C_last_col);
    }
    /*         |  |   |A |   |  |
     * Compute |AA| x |A | = |C | */
    {
      mzd_t *A_last_row = mzd_init_window(A, mmm, 0, m, m);
      mzd_t *A_first_col= mzd_init_window(A,   0, 0, m, mmm);
      mzd_t *C_last_row = mzd_init_window(C, mmm, 0, m, mmm);
      _mzd_mul_m4rm(C_last_row, A_last_row, A_first_col, 0, TRUE);
      mzd_free_window(A_last_row);
      mzd_free_window(A_first_col);
      mzd_free_window(C_last_row);
    }
    /* Add to  |  |   | A|   |C |
     * result  |A | x |  | = |  | */
    {
      mzd_t *A_last_col = mzd_init_window(A,   0, mmm, mmm, m);
      mzd_t *A_last_row = mzd_init_window(A, mmm,   0,   m, mmm);
      mzd_t *C_bulk = mzd_init_window(C, 0, 0, mmm, mmm);
      mzd_addmul_m4rm(C_bulk, A_last_col, A_last_row, 0);
      mzd_free_window(A_last_col);
      mzd_free_window(A_last_row);
      mzd_free_window(C_bulk);
    }
  }

  return C;
}


#ifdef HAVE_OPENMP
mzd_t *_mzd_addmul_mp_even(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff) {
  /**
   * \todo make sure not to overwrite crap after ncols and before width*RADIX
   */
  size_t a,b,c;
  size_t anr, anc, bnr, bnc;
  
  a = A->nrows;
  b = A->ncols;
  c = B->ncols;
  /* handle case first, where the input matrices are too small already */
  if (CLOSER(A->nrows, A->nrows/2, cutoff) || CLOSER(A->ncols, A->ncols/2, cutoff) || CLOSER(B->ncols, B->ncols/2, cutoff)) {
    /* we copy the matrix first since it is only constant memory
       overhead and improves data locality, if you remove it make sure
       there are no speed regressions */
    /* C = _mzd_mul_m4rm(C, A, B, 0, TRUE); */
    mzd_t *Cbar = mzd_init(C->nrows, C->ncols);
    Cbar = _mzd_mul_m4rm(Cbar, A, B, 0, FALSE);
    mzd_copy(C, Cbar);
    mzd_free(Cbar);
    return C;
  }

  /* adjust cutting numbers to work on words */
  unsigned long mult = 2;
/*   long width = a; */
/*   while (width > 2*cutoff) { */
/*     width/=2; */
/*     mult*=2; */
/*   } */
  a -= a%(RADIX*mult);
  b -= b%(RADIX*mult);
  c -= c%(RADIX*mult);

  anr = ((a/RADIX) >> 1) * RADIX;
  anc = ((b/RADIX) >> 1) * RADIX;
  bnr = anc;
  bnc = ((c/RADIX) >> 1) * RADIX;

  mzd_t *A00 = mzd_init_window(A,   0,   0,   anr,   anc);
  mzd_t *A01 = mzd_init_window(A,   0, anc,   anr, 2*anc);
  mzd_t *A10 = mzd_init_window(A, anr,   0, 2*anr,   anc);
  mzd_t *A11 = mzd_init_window(A, anr, anc, 2*anr, 2*anc);

  mzd_t *B00 = mzd_init_window(B,   0,   0,   bnr,   bnc);
  mzd_t *B01 = mzd_init_window(B,   0, bnc,   bnr, 2*bnc);
  mzd_t *B10 = mzd_init_window(B, bnr,   0, 2*bnr,   bnc);
  mzd_t *B11 = mzd_init_window(B, bnr, bnc, 2*bnr, 2*bnc);

  mzd_t *C00 = mzd_init_window(C,   0,   0,   anr,   bnc);
  mzd_t *C01 = mzd_init_window(C,   0, bnc,   anr, 2*bnc);
  mzd_t *C10 = mzd_init_window(C, anr,   0, 2*anr,   bnc);
  mzd_t *C11 = mzd_init_window(C, anr, bnc, 2*anr, 2*bnc);
  
#pragma omp parallel sections num_threads(4)
  {
#pragma omp section
    {
      _mzd_addmul_even(C00, A00, B00, cutoff);
      _mzd_addmul_even(C00, A01, B10, cutoff);
    }
#pragma omp section 
    {
      _mzd_addmul_even(C01, A00, B01, cutoff);
      _mzd_addmul_even(C01, A01, B11, cutoff);
    }
#pragma omp section
    {
      _mzd_addmul_even(C10, A10, B00, cutoff);
      _mzd_addmul_even(C10, A11, B10, cutoff);
    }
#pragma omp section
    {
      _mzd_addmul_even(C11, A10, B01, cutoff);
      _mzd_addmul_even(C11, A11, B11, cutoff);
    }
  }

  /* deal with rest */
  if (B->ncols > 2*bnc) {
    mzd_t *B_last_col = mzd_init_window(B, 0, 2*bnc, A->ncols, B->ncols); 
    mzd_t *C_last_col = mzd_init_window(C, 0, 2*bnc, A->nrows, C->ncols);
    mzd_addmul_m4rm(C_last_col, A, B_last_col, 0);
    mzd_free_window(B_last_col);
    mzd_free_window(C_last_col);
  }
  if (A->nrows > 2*anr) {
    mzd_t *A_last_row = mzd_init_window(A, 2*anr, 0, A->nrows, A->ncols);
    mzd_t *B_bulk = mzd_init_window(B, 0, 0, B->nrows, 2*bnc);
    mzd_t *C_last_row = mzd_init_window(C, 2*anr, 0, C->nrows, 2*bnc);
    mzd_addmul_m4rm(C_last_row, A_last_row, B_bulk, 0);
    mzd_free_window(A_last_row);
    mzd_free_window(B_bulk);
    mzd_free_window(C_last_row);
  }
  if (A->ncols > 2*anc) {
    mzd_t *A_last_col = mzd_init_window(A,     0, 2*anc, 2*anr, A->ncols);
    mzd_t *B_last_row = mzd_init_window(B, 2*bnr,     0, B->nrows, 2*bnc);
    mzd_t *C_bulk = mzd_init_window(C, 0, 0, 2*anr, bnc*2);
    mzd_addmul_m4rm(C_bulk, A_last_col, B_last_row, 0);
    mzd_free_window(A_last_col);
    mzd_free_window(B_last_row);
    mzd_free_window(C_bulk);
  }

  /* clean up */
  mzd_free_window(A00); mzd_free_window(A01);
  mzd_free_window(A10); mzd_free_window(A11);

  mzd_free_window(B00); mzd_free_window(B01);
  mzd_free_window(B10); mzd_free_window(B11);

  mzd_free_window(C00); mzd_free_window(C01);
  mzd_free_window(C10); mzd_free_window(C11);
  
  return C;
}
#endif

mzd_t *mzd_mul(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff) {
  if(A->ncols != B->nrows)
    m4ri_die("mzd_mul: A ncols (%d) need to match B nrows (%d).\n", A->ncols, B->nrows);
  
  if (cutoff < 0)
    m4ri_die("mzd_mul: cutoff must be >= 0.\n");

  if(cutoff == 0) {
    cutoff = STRASSEN_MUL_CUTOFF;
  }

  cutoff = cutoff/RADIX * RADIX;
  if (cutoff < RADIX) {
    cutoff = RADIX;
  };

  if (C == NULL) {
    C = mzd_init(A->nrows, B->ncols);
  } else if (C->nrows != A->nrows || C->ncols != B->ncols){
    m4ri_die("mzd_mul: C (%d x %d) has wrong dimensions, expected (%d x %d)\n",
	     C->nrows, C->ncols, A->nrows, B->ncols);
  }

  if(A->offset || B->offset || C->offset) {
    mzd_set_ui(C, 0);
    mzd_addmul(C, A, B, cutoff);
    return C;
  }

  C = (A==B)?_mzd_sqr_even(C, A, cutoff):_mzd_mul_even(C, A, B, cutoff);
  return C;
}

mzd_t *_mzd_addmul_even_orig(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff) {
  /**
   * \todo make sure not to overwrite crap after ncols and before width*RADIX
   */

  size_t a,b,c;
  size_t anr, anc, bnr, bnc;
  
  if(C->nrows == 0 || C->ncols == 0)
    return C;

  a = A->nrows;
  b = A->ncols;
  c = B->ncols;
  /* handle case first, where the input matrices are too small already */
  if (CLOSER(A->nrows, A->nrows/2, cutoff) || CLOSER(A->ncols, A->ncols/2, cutoff) || CLOSER(B->ncols, B->ncols/2, cutoff)) {
    /* we copy the matrix first since it is only constant memory
       overhead and improves data locality, if you remove it make sure
       there are no speed regressions */
    mzd_t *Cbar = mzd_copy(NULL, C);
    mzd_addmul_m4rm(Cbar, A, B, 0);
    mzd_copy(C, Cbar);
    mzd_free(Cbar);
    return C;
  }

  /* adjust cutting numbers to work on words */
  unsigned long mult = 1;
  long width = MIN(MIN(a,b),c);
  while (width > 2*cutoff) {
    width/=2;
    mult*=2;
  }
  a -= a%(RADIX*mult);
  b -= b%(RADIX*mult);
  c -= c%(RADIX*mult);

  anr = ((a/RADIX) >> 1) * RADIX;
  anc = ((b/RADIX) >> 1) * RADIX;
  bnr = anc;
  bnc = ((c/RADIX) >> 1) * RADIX;

  mzd_t *A00 = mzd_init_window(A,   0,   0,   anr,   anc);
  mzd_t *A01 = mzd_init_window(A,   0, anc,   anr, 2*anc);
  mzd_t *A10 = mzd_init_window(A, anr,   0, 2*anr,   anc);
  mzd_t *A11 = mzd_init_window(A, anr, anc, 2*anr, 2*anc);

  mzd_t *B00 = mzd_init_window(B,   0,   0,   bnr,   bnc);
  mzd_t *B01 = mzd_init_window(B,   0, bnc,   bnr, 2*bnc);
  mzd_t *B10 = mzd_init_window(B, bnr,   0, 2*bnr,   bnc);
  mzd_t *B11 = mzd_init_window(B, bnr, bnc, 2*bnr, 2*bnc);

  mzd_t *C00 = mzd_init_window(C,   0,   0,   anr,   bnc);
  mzd_t *C01 = mzd_init_window(C,   0, bnc,   anr, 2*bnc);
  mzd_t *C10 = mzd_init_window(C, anr,   0, 2*anr,   bnc);
  mzd_t *C11 = mzd_init_window(C, anr, bnc, 2*anr, 2*bnc);

  /**
   * \note See Jean-Guillaume Dumas, Clement Pernet, Wei Zhou; "Memory
   * efficient scheduling of Strassen-Winograd's matrix multiplication
   * algorithm"; http://arxiv.org/pdf/0707.2347v3 for reference on the
   * used operation scheduling.
   */

  mzd_t *X0 = mzd_init(anr, anc);
  mzd_t *X1 = mzd_init(bnr, bnc);
  mzd_t *X2 = mzd_init(anr, bnc);
  
  _mzd_add(X0, A10, A11);                  /* 1  S1 = A21 + A22        X1 */
  _mzd_add(X1, B01, B00);                  /* 2  T1 = B12 - B11        X2 */
  _mzd_mul_even_orig(X2, X0, X1, cutoff);       /* 3  P5 = S1 T1            X3 */
  
  _mzd_add(C11, X2, C11);                  /* 4  C22 = P5 + C22       C22 */
  _mzd_add(C01, X2, C01);                  /* 5  C12 = P5 + C12       C12 */
  _mzd_add(X0, X0, A00);                   /* 6  S2 = S1 - A11         X1 */
  _mzd_add(X1, B11, X1);                   /* 7  T2 = B22 - T1         X2 */
  _mzd_mul_even_orig(X2, A00, B00, cutoff);     /* 8  P1 = A11 B11          X3 */
  
  _mzd_add(C00, X2, C00);                  /* 9  C11 = P1 + C11       C11 */
  _mzd_addmul_even_orig(X2, X0, X1, cutoff);    /* 10 U2 = S2 T2 + P1       X3 */

  _mzd_addmul_even_orig(C00, A01, B10, cutoff); /* 11 U1 = A12 B21 + C11   C11 */
  
  _mzd_add(X0, A01, X0);                   /* 12 S4 = A12 - S2         X1 */
  _mzd_add(X1, X1, B10);                   /* 13 T4 = T2 - B21         X2 */
  _mzd_addmul_even_orig(C01, X0, B11, cutoff);  /* 14 C12 = S4 B22 + C12   C12 */
  
  _mzd_add(C01, X2, C01);                  /* 15 U5 = U2 + C12        C12 */
  _mzd_addmul_even_orig(C10, A11, X1, cutoff);  /* 16 P4 = A22 T4 - C21    C21 */
  
  _mzd_add(X0, A00, A10);                  /* 17 S3 = A11 - A21        X1 */
  _mzd_add(X1, B11, B01);                  /* 18 T3 = B22 - B12        X2 */
  _mzd_addmul_even_orig(X2, X0, X1, cutoff);    /* 19 U3 = S3 T3 + U2       X3 */
  
  _mzd_add(C11, X2, C11);                  /* 20 U7 = U3 + C22        C22 */
  _mzd_add(C10, X2, C10);                  /* 21 U6 = U3 - C21        C21 */

  /* deal with rest */
  if (B->ncols > 2*bnc) {
    mzd_t *B_last_col = mzd_init_window(B, 0, 2*bnc, A->ncols, B->ncols); 
    mzd_t *C_last_col = mzd_init_window(C, 0, 2*bnc, A->nrows, C->ncols);
    mzd_addmul_m4rm(C_last_col, A, B_last_col, 0);
    mzd_free_window(B_last_col);
    mzd_free_window(C_last_col);
  }
  if (A->nrows > 2*anr) {
    mzd_t *A_last_row = mzd_init_window(A, 2*anr, 0, A->nrows, A->ncols);
    mzd_t *B_bulk = mzd_init_window(B, 0, 0, B->nrows, 2*bnc);
    mzd_t *C_last_row = mzd_init_window(C, 2*anr, 0, C->nrows, 2*bnc);
    mzd_addmul_m4rm(C_last_row, A_last_row, B_bulk, 0);
    mzd_free_window(A_last_row);
    mzd_free_window(B_bulk);
    mzd_free_window(C_last_row);
  }
  if (A->ncols > 2*anc) {
    mzd_t *A_last_col = mzd_init_window(A,     0, 2*anc, 2*anr, A->ncols);
    mzd_t *B_last_row = mzd_init_window(B, 2*bnr,     0, B->nrows, 2*bnc);
    mzd_t *C_bulk = mzd_init_window(C, 0, 0, 2*anr, bnc*2);
    mzd_addmul_m4rm(C_bulk, A_last_col, B_last_row, 0);
    mzd_free_window(A_last_col);
    mzd_free_window(B_last_row);
    mzd_free_window(C_bulk);
  }

  /* clean up */
  mzd_free_window(A00); mzd_free_window(A01);
  mzd_free_window(A10); mzd_free_window(A11);

  mzd_free_window(B00); mzd_free_window(B01);
  mzd_free_window(B10); mzd_free_window(B11);

  mzd_free_window(C00); mzd_free_window(C01);
  mzd_free_window(C10); mzd_free_window(C11);
  
  mzd_free(X0);
  mzd_free(X1);
  mzd_free(X2);

  return C;
}

mzd_t *_mzd_addmul_even(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff) {
  /**
   * \todo make sure not to overwrite crap after ncols and before width*RADIX
   */

  size_t m,k,n;
  size_t mmm, kkk, nnn;
  
  if(C->nrows == 0 || C->ncols == 0)
    return C;

  m = A->nrows;
  k = A->ncols;
  n = B->ncols;

  /* handle case first, where the input matrices are too small already */
  if (CLOSER(m, m/2, cutoff) || CLOSER(k, k/2, cutoff) || CLOSER(n, n/2, cutoff)) {
    /* we copy the matrix first since it is only constant memory
       overhead and improves data locality, if you remove it make sure
       there are no speed regressions */
    mzd_t *Cbar = mzd_copy(NULL, C);
    mzd_addmul_m4rm(Cbar, A, B, 0);
    mzd_copy(C, Cbar);
    mzd_free(Cbar);
    return C;
  }

#ifdef HAVE_OPENMP
  if (omp_get_num_threads() < omp_get_max_threads()) {
    return _mzd_addmul_mp_even(C, A, B, cutoff);
  }
#endif

  /* adjust cutting numbers to work on words */
  {
    unsigned long mult = RADIX;
    unsigned long width = MIN(MIN(m,n),k)/2;
    while (width > (unsigned long)cutoff) {
      width>>=1;
      mult<<=1;
    }

    mmm = (((m - m%mult)/RADIX) >> 1) * RADIX;
    kkk = (((k - k%mult)/RADIX) >> 1) * RADIX;
    nnn = (((n - n%mult)/RADIX) >> 1) * RADIX;
  }

  /*         |C |    |A |   |B | 
   * Compute |  | += |  | x |  |  */
  {
    mzd_t *A11 = mzd_init_window(A,   0,   0,   mmm,   kkk);
    mzd_t *A12 = mzd_init_window(A,   0, kkk,   mmm, 2*kkk);
    mzd_t *A21 = mzd_init_window(A, mmm,   0, 2*mmm,   kkk);
    mzd_t *A22 = mzd_init_window(A, mmm, kkk, 2*mmm, 2*kkk);

    mzd_t *B11 = mzd_init_window(B,   0,   0,   kkk,   nnn);
    mzd_t *B12 = mzd_init_window(B,   0, nnn,   kkk, 2*nnn);
    mzd_t *B21 = mzd_init_window(B, kkk,   0, 2*kkk,   nnn);
    mzd_t *B22 = mzd_init_window(B, kkk, nnn, 2*kkk, 2*nnn);

    mzd_t *C11 = mzd_init_window(C,   0,   0,   mmm,   nnn);
    mzd_t *C12 = mzd_init_window(C,   0, nnn,   mmm, 2*nnn);
    mzd_t *C21 = mzd_init_window(C, mmm,   0, 2*mmm,   nnn);
    mzd_t *C22 = mzd_init_window(C, mmm, nnn, 2*mmm, 2*nnn);
  
    /**
     * \note See Marco Bodrato; "A Strassen-like Matrix Multiplication
     * Suited for Squaring and Highest Power Computation";
     * http://bodrato.it/papres/#CIVV2008 for reference on the used
     * sequence of operations.
     */

    mzd_t *S = mzd_init(mmm, kkk);
    mzd_t *T = mzd_init(kkk, nnn);
    mzd_t *U = mzd_init(mmm, nnn);

    _mzd_add(S, A22, A21);                   /* 1  S = A22 - A21       */
    _mzd_add(T, B22, B21);                   /* 2  T = B22 - B21       */
    _mzd_mul_even(U, S, T, cutoff);         /* 3  U = S*T             */
    _mzd_add(C22, U, C22);                   /* 4  C22 = U + C22       */
    _mzd_add(C12, U, C12);                   /* 5  C12 = U + C12       */

    _mzd_mul_even(U, A12, B21, cutoff);     /* 8  U = A12*B21         */
    _mzd_add(C11, U, C11);                   /* 9  C11 = U + C11       */

    _mzd_addmul_even(C11, A11, B11, cutoff);/* 11 C11 = A11*B11 + C11 */

    _mzd_add(S, S, A12);                     /* 6  S = S - A12         */
    _mzd_add(T, T, B12);                     /* 7  T = T - B12         */
    _mzd_addmul_even(U, S, T, cutoff);      /* 10 U = S*T + U         */
    _mzd_add(C12, C12, U);                   /* 15 C12 = U + C12       */

    _mzd_add(S, A11, S);                     /* 12 S = A11 - S         */
    _mzd_addmul_even(C12, S, B12, cutoff);  /* 14 C12 = S*B12 + C12   */

    _mzd_add(T, B11, T);                     /* 13 T = B11 - T         */
    _mzd_addmul_even(C21, A21, T, cutoff);  /* 16 C21 = A21*T + C21   */

    _mzd_add(S, A22, A12);                   /* 17 S = A22 + A21       */
    _mzd_add(T, B22, B12);                   /* 18 T = B22 + B21       */
    _mzd_addmul_even(U, S, T, cutoff);      /* 19 U = U - S*T         */
    _mzd_add(C21, C21, U);                   /* 20 C21 = C21 - U3      */
    _mzd_add(C22, C22, U);                   /* 21 C22 = C22 - U3      */

    /* clean up */
    mzd_free_window(A11); mzd_free_window(A12);
    mzd_free_window(A21); mzd_free_window(A22);

    mzd_free_window(B11); mzd_free_window(B12);
    mzd_free_window(B21); mzd_free_window(B22);

    mzd_free_window(C11); mzd_free_window(C12);
    mzd_free_window(C21); mzd_free_window(C22);

    mzd_free(S);
    mzd_free(T);
    mzd_free(U);
  }
  /* deal with rest */
  nnn*=2;
  if (n > nnn) {
    /*         | C|    |AA|   | B|
     * Compute | C| += |AA| x | B| */
    mzd_t *B_last_col = mzd_init_window(B, 0, nnn, k, n); 
    mzd_t *C_last_col = mzd_init_window(C, 0, nnn, m, n);
    mzd_addmul_m4rm(C_last_col, A, B_last_col, 0);
    mzd_free_window(B_last_col);
    mzd_free_window(C_last_col);
  }
  mmm*=2;
  if (m > mmm) {
    /*         |  |    |  |   |B |
     * Compute |C | += |AA| x |B | */
    mzd_t *A_last_row = mzd_init_window(A, mmm, 0, m, k);
    mzd_t *B_first_col= mzd_init_window(B,   0, 0, k, nnn);
    mzd_t *C_last_row = mzd_init_window(C, mmm, 0, m, nnn);
    mzd_addmul_m4rm(C_last_row, A_last_row, B_first_col, 0);
    mzd_free_window(A_last_row);
    mzd_free_window(B_first_col);
    mzd_free_window(C_last_row);
  }
  kkk*=2;
  if (k > kkk) {
    /* Add to  |  |   | B|   |C |
     * result  |A | x |  | = |  | */
    mzd_t *A_last_col = mzd_init_window(A,   0, kkk, mmm, k);
    mzd_t *B_last_row = mzd_init_window(B, kkk,   0,   k, nnn);
    mzd_t *C_bulk = mzd_init_window(C, 0, 0, mmm, nnn);
    mzd_addmul_m4rm(C_bulk, A_last_col, B_last_row, 0);
    mzd_free_window(A_last_col);
    mzd_free_window(B_last_row);
    mzd_free_window(C_bulk);
  }

  return C;
}

mzd_t *_mzd_addsqr_even(mzd_t *C, mzd_t *A, int cutoff) {
  /**
   * \todo make sure not to overwrite crap after ncols and before width*RADIX
   */

  size_t m;
  size_t mmm;
  
  if(C->nrows == 0)
    return C;

  m = A->nrows;

  /* handle case first, where the input matrices are too small already */
  if (CLOSER(m, m/2, cutoff)) {
    /* we copy the matrix first since it is only constant memory
       overhead and improves data locality, if you remove it make sure
       there are no speed regressions */
    mzd_t *Cbar = mzd_copy(NULL, C);
    mzd_addmul_m4rm(Cbar, A, A, 0);
    mzd_copy(C, Cbar);
    mzd_free(Cbar);
    return C;
  }

  /* adjust cutting numbers to work on words */
  {
    unsigned long mult = RADIX;
    unsigned long width = m>>1;
    while (width > (unsigned long)cutoff) {
      width>>=1;
      mult<<=1;
    }

    mmm = (((m - m%mult)/RADIX) >> 1) * RADIX;
  }

  /*         |C |    |A |   |B | 
   * Compute |  | += |  | x |  |  */
  {
    mzd_t *A11 = mzd_init_window(A,   0,   0,   mmm,   mmm);
    mzd_t *A12 = mzd_init_window(A,   0, mmm,   mmm, 2*mmm);
    mzd_t *A21 = mzd_init_window(A, mmm,   0, 2*mmm,   mmm);
    mzd_t *A22 = mzd_init_window(A, mmm, mmm, 2*mmm, 2*mmm);

    mzd_t *C11 = mzd_init_window(C,   0,   0,   mmm,   mmm);
    mzd_t *C12 = mzd_init_window(C,   0, mmm,   mmm, 2*mmm);
    mzd_t *C21 = mzd_init_window(C, mmm,   0, 2*mmm,   mmm);
    mzd_t *C22 = mzd_init_window(C, mmm, mmm, 2*mmm, 2*mmm);
  
    /**
     * \note See Marco Bodrato; "A Strassen-like Matrix Multiplication
     * Suited for Squaring and Highest Power Computation"; on-line v.
     * http://bodrato.it/papres/#CIVV2008 for reference on the used
     * sequence of operations.
     */

    mzd_t *S = mzd_init(mmm, mmm);
    mzd_t *U = mzd_init(mmm, mmm);

    _mzd_add(S, A22, A21);                   /* 1  S = A22 - A21       */
    _mzd_sqr_even(U, S, cutoff);            /* 3  U = S^2             */
    _mzd_add(C22, U, C22);                   /* 4  C22 = U + C22       */
    _mzd_add(C12, U, C12);                   /* 5  C12 = U + C12       */

    _mzd_mul_even(U, A12, A21, cutoff);     /* 8  U = A12*A21         */
    _mzd_add(C11, U, C11);                   /* 9  C11 = U + C11       */

    _mzd_addsqr_even(C11, A11, cutoff);     /* 11 C11 = A11^2 + C11   */

    _mzd_add(S, S, A12);                     /* 6  S = S + A12         */
    _mzd_addsqr_even(U, S, cutoff);         /* 10 U = S^2 + U         */
    _mzd_add(C12, C12, U);                   /* 15 C12 = U + C12       */

    _mzd_add(S, A11, S);                     /* 12 S = A11 - S         */
    _mzd_addmul_even(C12, S, A12, cutoff);  /* 14 C12 = S*B12 + C12   */

    _mzd_addmul_even(C21, A21, S, cutoff);  /* 16 C21 = A21*T + C21   */

    _mzd_add(S, A22, A12);                   /* 17 S = A22 + A21       */
    _mzd_addsqr_even(U, S, cutoff);         /* 19 U = U - S^2         */
    _mzd_add(C21, C21, U);                   /* 20 C21 = C21 - U3      */
    _mzd_add(C22, C22, U);                   /* 21 C22 = C22 - U3      */

    /* clean up */
    mzd_free_window(A11); mzd_free_window(A12);
    mzd_free_window(A21); mzd_free_window(A22);

    mzd_free_window(C11); mzd_free_window(C12);
    mzd_free_window(C21); mzd_free_window(C22);

    mzd_free(S);
    mzd_free(U);
  }
  /* deal with rest */
  mmm*=2;
  if (m > mmm) {
    /*         | C|    |AA|   | B|
     * Compute | C| += |AA| x | B| */
    {
      mzd_t *A_last_col = mzd_init_window(A, 0, mmm, m, m); 
      mzd_t *C_last_col = mzd_init_window(C, 0, mmm, m, m);
      mzd_addmul_m4rm(C_last_col, A, A_last_col, 0);
      mzd_free_window(A_last_col);
      mzd_free_window(C_last_col);
    }
    /*         |  |    |  |   |B |
     * Compute |C | += |AA| x |B | */
    {
      mzd_t *A_last_row = mzd_init_window(A, mmm, 0, m, m);
      mzd_t *A_first_col= mzd_init_window(A,   0, 0, m, mmm);
      mzd_t *C_last_row = mzd_init_window(C, mmm, 0, m, mmm);
      mzd_addmul_m4rm(C_last_row, A_last_row, A_first_col, 0);
      mzd_free_window(A_last_row);
      mzd_free_window(A_first_col);
      mzd_free_window(C_last_row);
    }
    /* Add to  |  |   | B|   |C |
     * result  |A | x |  | = |  | */
    {
      mzd_t *A_last_col = mzd_init_window(A,   0, mmm, mmm, m);
      mzd_t *A_last_row = mzd_init_window(A, mmm,   0,   m, mmm);
      mzd_t *C_bulk = mzd_init_window(C, 0, 0, mmm, mmm);
      mzd_addmul_m4rm(C_bulk, A_last_col, A_last_row, 0);
      mzd_free_window(A_last_col);
      mzd_free_window(A_last_row);
      mzd_free_window(C_bulk);
    }
  }

  return C;
}

mzd_t *_mzd_addmul(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff){
  /**
   * Assumes that B and C are aligned in the same manner (as in a Schur complement)
   */
  
  if (!A->offset){
    if (!B->offset) /* A even, B even */
      return (A==B) ? _mzd_addsqr_even(C, A, cutoff) : _mzd_addmul_even(C, A, B, cutoff);
    else {  /* A even, B weird */
      size_t bnc = RADIX - B->offset;
      if (B->ncols <= bnc){
	_mzd_addmul_even_weird  (C,  A, B, cutoff);
      } else {
	mzd_t * B0 = mzd_init_window (B, 0, 0, B->nrows, bnc);
	mzd_t * C0 = mzd_init_window (C, 0, 0, C->nrows, bnc);
	mzd_t * B1 = mzd_init_window (B, 0, bnc, B->nrows, B->ncols);
	mzd_t * C1 = mzd_init_window (C, 0, bnc, C->nrows, C->ncols);
	_mzd_addmul_even_weird  (C0,  A, B0, cutoff);
	_mzd_addmul_even(C1, A, B1, cutoff);
	mzd_free_window (B0); mzd_free_window (B1);
	mzd_free_window (C0); mzd_free_window (C1);
      }
    }
  } else if (B->offset) { /* A weird, B weird */
    size_t anc = RADIX - A->offset;
    size_t bnc = RADIX - B->offset;
    if (B->ncols <= bnc){
      if (A->ncols <= anc)
	_mzd_addmul_weird_weird (C, A, B, cutoff);
      else {
	mzd_t * A0  = mzd_init_window (A, 0, 0, A->nrows, anc);
	mzd_t * A1  = mzd_init_window (A, 0, anc, A->nrows, A->ncols);
	mzd_t * B0  = mzd_init_window (B, 0, 0, anc, B->ncols);
	mzd_t * B1  = mzd_init_window (B, anc, 0, B->nrows, B->ncols);
	_mzd_addmul_weird_weird (C, A0, B0, cutoff);
	_mzd_addmul_even_weird  (C, A1, B1, cutoff);
	mzd_free_window (A0);  mzd_free_window (A1);
	mzd_free_window (B0);  mzd_free_window (B1);
      }
    } else if (A->ncols <= anc) {
      mzd_t * B0 = mzd_init_window (B, 0, 0, B->nrows, bnc);
      mzd_t * B1 = mzd_init_window (B, 0, bnc, B->nrows, B->ncols);
      mzd_t * C0 = mzd_init_window (C, 0, 0, C->nrows, bnc);
      mzd_t * C1 = mzd_init_window (C, 0, bnc, C->nrows, C->ncols);
      _mzd_addmul_weird_weird (C0, A, B0, cutoff);
      _mzd_addmul_weird_even  (C1, A, B1, cutoff);
      mzd_free_window (B0); mzd_free_window (B1);
      mzd_free_window (C0); mzd_free_window (C1);
    } else {
      mzd_t * A0  = mzd_init_window (A, 0, 0, A->nrows, anc);
      mzd_t * A1  = mzd_init_window (A, 0, anc, A->nrows, A->ncols);
      mzd_t * B00 = mzd_init_window (B, 0, 0, anc, bnc);
      mzd_t * B01 = mzd_init_window (B, 0, bnc, anc, B->ncols);
      mzd_t * B10 = mzd_init_window (B, anc, 0, B->nrows, bnc);
      mzd_t * B11 = mzd_init_window (B, anc, bnc, B->nrows, B->ncols);
      mzd_t * C0 = mzd_init_window (C, 0, 0, C->nrows, bnc);
      mzd_t * C1 = mzd_init_window (C, 0, bnc, C->nrows, C->ncols);
      
      _mzd_addmul_weird_weird (C0, A0, B00, cutoff);
      _mzd_addmul_even_weird  (C0,  A1, B10, cutoff);
      _mzd_addmul_weird_even  (C1,  A0, B01, cutoff);
      _mzd_addmul_even  (C1,  A1, B11, cutoff);

      mzd_free_window (A0);  mzd_free_window (A1);
      mzd_free_window (C0);  mzd_free_window (C1);
      mzd_free_window (B00); mzd_free_window (B01);
      mzd_free_window (B10); mzd_free_window (B11);
    }
  } else { /* A weird, B even */
    size_t anc = RADIX - A->offset;
    if (A->ncols <= anc){
      _mzd_addmul_weird_even  (C,  A, B, cutoff);
    } else {
      mzd_t * A0  = mzd_init_window (A, 0, 0, A->nrows, anc);
      mzd_t * A1  = mzd_init_window (A, 0, anc, A->nrows, A->ncols);
      mzd_t * B0  = mzd_init_window (B, 0, 0, anc, B->ncols);
      mzd_t * B1  = mzd_init_window (B, anc, 0, B->nrows, B->ncols);
      _mzd_addmul_weird_even (C, A0, B0, cutoff);
      _mzd_addmul_even  (C, A1, B1, cutoff);
      mzd_free_window (A0); mzd_free_window (A1);
      mzd_free_window (B0); mzd_free_window (B1);
    }
  }
  return C;
}

mzd_t *_mzd_addmul_weird_even (mzd_t *C, mzd_t *A, mzd_t *B, int cutoff){
  mzd_t * tmp = mzd_init (A->nrows, MIN(RADIX- A->offset, A->ncols));
  for (size_t i=0; i < A->nrows; ++i){
    tmp->rows[i][0] = (A->rows[i][0] << A->offset);
  }
  _mzd_addmul_even (C, tmp, B, cutoff);
  mzd_free(tmp);
  return C;
}

 mzd_t *_mzd_addmul_even_weird (mzd_t *C, mzd_t *A, mzd_t *B, int cutoff){
   mzd_t * tmp = mzd_init (B->nrows, RADIX);
   size_t offset = C->offset;
   size_t cncols = C->ncols;
   C->offset=0;
   C->ncols = RADIX;
   word mask = ((ONE << B->ncols) - 1) << (RADIX-B->offset - B->ncols);
   for (size_t i=0; i < B->nrows; ++i)
     tmp->rows[i][0] = B->rows[i][0] & mask;
   _mzd_addmul_even (C, A, tmp, cutoff);
   C->offset=offset;
   C->ncols = cncols;
   mzd_free (tmp);
   return C;
}

 mzd_t* _mzd_addmul_weird_weird (mzd_t* C, mzd_t* A, mzd_t *B, int cutoff){
   mzd_t *BT;
   word* temp;
   BT = mzd_init( B->ncols, B->nrows );
   
   for (size_t i = 0; i < B->ncols; ++i) {
     temp = BT->rows[i];
     for (size_t k = 0; k < B->nrows; k++) {
      *temp |= ((word)mzd_read_bit (B, k, i)) << (RADIX-1-k-A->offset);

     }
   }
   
   word parity[64];
   for (size_t i = 0; i < 64; i++) {
     parity[i] = 0;
   }
   for (size_t i = 0; i < A->nrows; ++i) {
     word * a = A->rows[i];
     word * c = C->rows[i];
     for (size_t k=0; k< C->ncols; k++) {
       word *b = BT->rows[k];
       parity[k+C->offset] = (*a) & (*b);
     }
     word par = parity64(parity);
     *c ^= par;//parity64(parity);
   }
   mzd_free (BT);
   return C;
 }

 mzd_t *mzd_addmul(mzd_t *C, mzd_t *A, mzd_t *B, int cutoff) {
  if(A->ncols != B->nrows)
    m4ri_die("mzd_addmul: A ncols (%d) need to match B nrows (%d).\n", A->ncols, B->nrows);
  
  if (cutoff < 0)
    m4ri_die("mzd_addmul: cutoff must be >= 0.\n");

  if(cutoff == 0) {
    cutoff = STRASSEN_MUL_CUTOFF;
  }
  
  cutoff = cutoff/RADIX * RADIX;
  if (cutoff < RADIX) {
    cutoff = RADIX;
  };

  if (C == NULL) {
    C = mzd_init(A->nrows, B->ncols);
  } else if (C->nrows != A->nrows || C->ncols != B->ncols){
    m4ri_die("mzd_addmul: C (%d x %d) has wrong dimensions, expected (%d x %d)\n",
	     C->nrows, C->ncols, A->nrows, B->ncols);
  }
  C = _mzd_addmul(C, A, B, cutoff);
  return C;
}


