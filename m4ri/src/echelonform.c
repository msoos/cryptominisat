/*******************************************************************
*
*                 M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2010 Martin Albrecht <M.R.Albrecht@rhul.ac.uk>
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

#include "misc.h"

#include "echelonform.h"
#include "brilliantrussian.h"
#include "pls.h"
#include "trsm.h"

size_t mzd_echelonize(mzd_t *A, int full) {
  return _mzd_echelonize_m4ri(A, full, 0, 1, ECHELONFORM_CROSSOVER_DENSITY);
}

size_t mzd_echelonize_m4ri(mzd_t *A, int full, int k) {
  return _mzd_echelonize_m4ri(A, full, k, 0, 1.0);
}

size_t mzd_echelonize_pluq(mzd_t *A, int full) {
  mzp_t *P = mzp_init(A->nrows);
  mzp_t *Q = mzp_init(A->ncols);

  size_t r = mzd_pluq(A, P, Q, 0);

  if(full) {
    mzd_t *U = mzd_init_window(A, 0, 0, r, r);
    mzd_t *B = mzd_init_window(A, 0, r, r, A->ncols);
    if(r!=A->ncols) 
      mzd_trsm_upper_left(U, B, 0);
    if(r!=0) 
      mzd_set_ui(U, 0);
    for(size_t i=0; i<r; i++)
      mzd_write_bit(A, i, i, 1);
    mzd_free_window(U);
    mzd_free_window(B);

  } else {
    for(size_t i=0; i<r; i++) {
      for(size_t j=0; j< i; j+=RADIX) {
        const size_t length = MIN(RADIX, i-j);
        mzd_clear_bits(A, i, j, length);
      }
    }
  }

  if(r!=0) {
    mzd_t *A0 = mzd_init_window(A, 0, 0, r, A->ncols);
    mzd_apply_p_right(A0, Q);
    mzd_free_window(A0);
  } else {
    mzd_apply_p_right(A, Q);
  }

  if(r!=A->nrows) {
    mzd_t *R = mzd_init_window(A, r, 0, A->nrows, A->ncols);
    mzd_set_ui(R, 0);
    mzd_free_window(R);
  }
  
  mzp_free(P);
  mzp_free(Q);
  return r;
}
