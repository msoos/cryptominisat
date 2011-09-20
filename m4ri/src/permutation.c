/******************************************************************************
*
*                 M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2008 Martin Albrecht <malb@informatik.uni-bremen.de> 
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
******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "permutation.h"
#include "packedmatrix.h"

mzp_t *mzp_init(size_t length) {
  size_t i;
  mzp_t *P = (mzp_t*)m4ri_mm_malloc(sizeof(mzp_t));
  P->values = (size_t*)m4ri_mm_malloc(sizeof(size_t)*length);
  P->length = length;
  for (i=0; i<length; i++) {
    P->values[i] = i;
  }
  return P;
}

void mzp_free(mzp_t *P) {
  m4ri_mm_free(P->values);
  m4ri_mm_free(P);
}

mzp_t *mzp_init_window(mzp_t* P, size_t begin, size_t end){
  mzp_t *window = (mzp_t *)m4ri_mm_malloc(sizeof(mzp_t));
  window->values = P->values + begin;
  window->length = end-begin;
  return window;
}

void mzp_free_window(mzp_t* condemned){
  m4ri_mm_free(condemned);
}

void mzp_set_ui(mzp_t *P, unsigned int value) {
  size_t i;
  for (i=0; i<P->length; i++) {
    P->values[i] = i;
  }
}

void mzd_apply_p_left(mzd_t *A, mzp_t *P) {
  size_t i;
  if(A->ncols == 0)
    return;
  const size_t length = MIN(P->length, A->nrows);
  for (i=0; i<length; i++) {
    assert(P->values[i] >= i);
    mzd_row_swap(A, i, P->values[i]);
  }
}

void mzd_apply_p_left_trans(mzd_t *A, mzp_t *P) {
  long i;
  if(A->ncols == 0)
    return;
  const size_t length = MIN(P->length, A->nrows);
  for (i=length-1; i>=0; i--) {
    assert(P->values[i] >= (size_t)i);
    mzd_row_swap(A, i, P->values[i]);
  }
}

/* optimised column swap operations */

static inline void mzd_write_col_to_rows_blockd(mzd_t *A, mzd_t *B, size_t *permutation, word *write_mask, const size_t start_row, const size_t stop_row, size_t length) {
  assert(A->offset == 0);
  for(size_t i=0; i<length; i+=RADIX) {
    /* optimisation for identity permutations */
    if (write_mask[i/RADIX] == FFFF)
      continue;
    const size_t todo = MIN(RADIX,length-i);
    const size_t a_word = (A->offset+i)/RADIX;
    size_t words[RADIX];
    size_t bits[RADIX];
    word bitmasks[RADIX];

    /* we pre-compute bit access in advance */
    for(size_t k=0;k<todo; k++) {
        const size_t colb = permutation[i+k] + B->offset;
        words[k] = colb/RADIX;
        bits[k] = colb%RADIX;
        bitmasks[k] = (ONE<<(RADIX - (bits[k]) - 1));
    }

    for (size_t r=start_row; r<stop_row; r++) {
      word *Brow = B->rows[r-start_row];
      word *Arow = A->rows[r];
      register word value = 0;

      /* we gather the bits in a register word */
      switch(todo-1) {
      case 63: value |= ((Brow[words[63]] & bitmasks[63]) << bits[63]) >> 63;
      case 62: value |= ((Brow[words[62]] & bitmasks[62]) << bits[62]) >> 62;
      case 61: value |= ((Brow[words[61]] & bitmasks[61]) << bits[61]) >> 61;
      case 60: value |= ((Brow[words[60]] & bitmasks[60]) << bits[60]) >> 60;
      case 59: value |= ((Brow[words[59]] & bitmasks[59]) << bits[59]) >> 59;
      case 58: value |= ((Brow[words[58]] & bitmasks[58]) << bits[58]) >> 58;
      case 57: value |= ((Brow[words[57]] & bitmasks[57]) << bits[57]) >> 57;
      case 56: value |= ((Brow[words[56]] & bitmasks[56]) << bits[56]) >> 56;
      case 55: value |= ((Brow[words[55]] & bitmasks[55]) << bits[55]) >> 55;
      case 54: value |= ((Brow[words[54]] & bitmasks[54]) << bits[54]) >> 54;
      case 53: value |= ((Brow[words[53]] & bitmasks[53]) << bits[53]) >> 53;
      case 52: value |= ((Brow[words[52]] & bitmasks[52]) << bits[52]) >> 52;
      case 51: value |= ((Brow[words[51]] & bitmasks[51]) << bits[51]) >> 51;
      case 50: value |= ((Brow[words[50]] & bitmasks[50]) << bits[50]) >> 50;
      case 49: value |= ((Brow[words[49]] & bitmasks[49]) << bits[49]) >> 49;
      case 48: value |= ((Brow[words[48]] & bitmasks[48]) << bits[48]) >> 48;
      case 47: value |= ((Brow[words[47]] & bitmasks[47]) << bits[47]) >> 47;
      case 46: value |= ((Brow[words[46]] & bitmasks[46]) << bits[46]) >> 46;
      case 45: value |= ((Brow[words[45]] & bitmasks[45]) << bits[45]) >> 45;
      case 44: value |= ((Brow[words[44]] & bitmasks[44]) << bits[44]) >> 44;
      case 43: value |= ((Brow[words[43]] & bitmasks[43]) << bits[43]) >> 43;
      case 42: value |= ((Brow[words[42]] & bitmasks[42]) << bits[42]) >> 42;
      case 41: value |= ((Brow[words[41]] & bitmasks[41]) << bits[41]) >> 41;
      case 40: value |= ((Brow[words[40]] & bitmasks[40]) << bits[40]) >> 40;
      case 39: value |= ((Brow[words[39]] & bitmasks[39]) << bits[39]) >> 39;
      case 38: value |= ((Brow[words[38]] & bitmasks[38]) << bits[38]) >> 38;
      case 37: value |= ((Brow[words[37]] & bitmasks[37]) << bits[37]) >> 37;
      case 36: value |= ((Brow[words[36]] & bitmasks[36]) << bits[36]) >> 36;
      case 35: value |= ((Brow[words[35]] & bitmasks[35]) << bits[35]) >> 35;
      case 34: value |= ((Brow[words[34]] & bitmasks[34]) << bits[34]) >> 34;
      case 33: value |= ((Brow[words[33]] & bitmasks[33]) << bits[33]) >> 33;
      case 32: value |= ((Brow[words[32]] & bitmasks[32]) << bits[32]) >> 32;
      case 31: value |= ((Brow[words[31]] & bitmasks[31]) << bits[31]) >> 31;
      case 30: value |= ((Brow[words[30]] & bitmasks[30]) << bits[30]) >> 30;
      case 29: value |= ((Brow[words[29]] & bitmasks[29]) << bits[29]) >> 29;
      case 28: value |= ((Brow[words[28]] & bitmasks[28]) << bits[28]) >> 28;
      case 27: value |= ((Brow[words[27]] & bitmasks[27]) << bits[27]) >> 27;
      case 26: value |= ((Brow[words[26]] & bitmasks[26]) << bits[26]) >> 26;
      case 25: value |= ((Brow[words[25]] & bitmasks[25]) << bits[25]) >> 25;
      case 24: value |= ((Brow[words[24]] & bitmasks[24]) << bits[24]) >> 24;
      case 23: value |= ((Brow[words[23]] & bitmasks[23]) << bits[23]) >> 23;
      case 22: value |= ((Brow[words[22]] & bitmasks[22]) << bits[22]) >> 22;
      case 21: value |= ((Brow[words[21]] & bitmasks[21]) << bits[21]) >> 21;
      case 20: value |= ((Brow[words[20]] & bitmasks[20]) << bits[20]) >> 20;
      case 19: value |= ((Brow[words[19]] & bitmasks[19]) << bits[19]) >> 19;
      case 18: value |= ((Brow[words[18]] & bitmasks[18]) << bits[18]) >> 18;
      case 17: value |= ((Brow[words[17]] & bitmasks[17]) << bits[17]) >> 17;
      case 16: value |= ((Brow[words[16]] & bitmasks[16]) << bits[16]) >> 16;
      case 15: value |= ((Brow[words[15]] & bitmasks[15]) << bits[15]) >> 15;
      case 14: value |= ((Brow[words[14]] & bitmasks[14]) << bits[14]) >> 14;
      case 13: value |= ((Brow[words[13]] & bitmasks[13]) << bits[13]) >> 13;
      case 12: value |= ((Brow[words[12]] & bitmasks[12]) << bits[12]) >> 12;
      case 11: value |= ((Brow[words[11]] & bitmasks[11]) << bits[11]) >> 11;
      case 10: value |= ((Brow[words[10]] & bitmasks[10]) << bits[10]) >> 10;
      case  9: value |= ((Brow[words[ 9]] & bitmasks[ 9]) << bits[ 9]) >>  9;
      case  8: value |= ((Brow[words[ 8]] & bitmasks[ 8]) << bits[ 8]) >>  8;
      case  7: value |= ((Brow[words[ 7]] & bitmasks[ 7]) << bits[ 7]) >>  7;
      case  6: value |= ((Brow[words[ 6]] & bitmasks[ 6]) << bits[ 6]) >>  6;
      case  5: value |= ((Brow[words[ 5]] & bitmasks[ 5]) << bits[ 5]) >>  5;
      case  4: value |= ((Brow[words[ 4]] & bitmasks[ 4]) << bits[ 4]) >>  4;
      case  3: value |= ((Brow[words[ 3]] & bitmasks[ 3]) << bits[ 3]) >>  3;
      case  2: value |= ((Brow[words[ 2]] & bitmasks[ 2]) << bits[ 2]) >>  2;
      case  1: value |= ((Brow[words[ 1]] & bitmasks[ 1]) << bits[ 1]) >>  1;
      case  0: value |= ((Brow[words[ 0]] & bitmasks[ 0]) << bits[ 0]) >>  0;
      default:
        break;
      }
/*       for(register size_t k=0; k<todo; k++) { */
/*         value |= ((Brow[words[k]] & bitmasks[k]) << bits[k]) >> k; */
/*       } */
      /* and write the word once */
      Arow[a_word] |= value;
    }
  }
}
/**
 * Implements both apply_p_right and apply_p_right_trans.
 */
void _mzd_apply_p_right_even(mzd_t *A, mzp_t *P, size_t start_row, size_t start_col, int notrans) {
  assert(A->offset == 0);
  if(A->nrows - start_row == 0)
    return;
  const size_t length = MIN(P->length,A->ncols);
  const size_t width = A->width;
  size_t step_size = MIN(A->nrows-start_row, MAX((CPU_L1_CACHE>>3)/A->width,1));

  /* our temporary where we store the columns we want to swap around */
  mzd_t *B = mzd_init(step_size, A->ncols);
  word *Arow;
  word *Brow;

  /* setup mathematical permutation */
  size_t *permutation = (size_t *)m4ri_mm_calloc(sizeof(size_t),A->ncols);
  for(size_t i=0; i<A->ncols; i++)
    permutation[i] = i;

  if (!notrans) {
    for(size_t i=start_col; i<length; i++) {
      size_t t = permutation[i];
      permutation[i] = permutation[P->values[i]];
      permutation[P->values[i]] = t;
    }
  } else {
    for(size_t i=start_col; i<length; i++) {
      size_t t = permutation[length-i-1];
      permutation[length-i-1] = permutation[P->values[length-i-1]];
      permutation[P->values[length-i-1]] = t;
    }
  }

  /* we have a bitmask to encode where to write to */
  word *write_mask = (word*)m4ri_mm_calloc(sizeof(word), width);
  for(size_t i=0; i<A->ncols; i+=RADIX) {
    const size_t todo = MIN(RADIX,A->ncols-i);
    for(size_t k=0; k<todo; k++) {
      if(permutation[i+k] == i+k) {
        write_mask[i/RADIX] |= ONE<<(RADIX - k - 1);
      }
    }
  }

  for(size_t i=start_row; i<A->nrows; i+=step_size) {
    step_size = MIN(step_size, A->nrows-i);
    for(size_t k=0; k<step_size; k++) {
      Arow = A->rows[i+k];
      Brow = B->rows[k];

      /*copy row & clear those values which will be overwritten */
      for(size_t j=0; j<width; j++) {
        Brow[j] = Arow[j];
        Arow[j] = Arow[j] & write_mask[j];
      }
    }
    /* here we actually write out the permutation */
    mzd_write_col_to_rows_blockd(A, B, permutation, write_mask, i, i+step_size, length);
  }
  m4ri_mm_free(permutation);
  m4ri_mm_free(write_mask);
  mzd_free(B);
}

void _mzd_apply_p_right_trans(mzd_t *A, mzp_t *P) {
  size_t i;
  if(A->nrows == 0)
    return;
  const size_t length = MIN(P->length, A->ncols);
  const size_t step_size = MAX((CPU_L1_CACHE>>3)/A->width,1);
  for(size_t j=0; j<A->nrows; j+=step_size) {
    size_t stop_row = MIN(j+step_size, A->nrows);
    for (i=0; i<length; ++i) {
      assert(P->values[i] >= i);
      mzd_col_swap_in_rows(A, i, P->values[i], j, stop_row);
    }
  }
/*   for (i=0; i<P->length; i++) { */
/*     assert(P->values[i] >= i); */
/*     mzd_col_swap(A, i, P->values[i]); */
/*   } */
}

void _mzd_apply_p_right(mzd_t *A, mzp_t *P) {
  int i;
  if(A->nrows == 0)
    return;
  const size_t step_size = MAX((CPU_L1_CACHE>>3)/A->width,1);
  for(size_t j=0; j<A->nrows; j+=step_size) {
    size_t stop_row = MIN(j+step_size, A->nrows);
    for (i=P->length-1; i>=0; --i) {
      assert(P->values[i] >= (size_t)i);
      mzd_col_swap_in_rows(A, i, P->values[i], j, stop_row);
    }
  }
/*   long i; */
/*   for (i=P->length-1; i>=0; i--) { */
/*     assert(P->values[i] >= i); */
/*     mzd_col_swap(A, i, P->values[i]); */
/*   } */
}


void mzd_apply_p_right_trans(mzd_t *A, mzp_t *P) {
  if(!A->nrows)
    return;
  if(A->offset) {
    _mzd_apply_p_right_trans(A,P);
    return;
  }
  _mzd_apply_p_right_even(A, P, 0, 0, 0); 
}

void mzd_apply_p_right(mzd_t *A, mzp_t *P) {
  if(!A->nrows)
    return;
  if(A->offset) {
    _mzd_apply_p_right(A,P);
    return;
  }
  _mzd_apply_p_right_even(A, P, 0, 0, 1); 
}

void mzd_apply_p_right_trans_even_capped(mzd_t *A, mzp_t *P, size_t start_row, size_t start_col) {
  assert(A->offset == 0);
  if(!A->nrows)
    return;
  _mzd_apply_p_right_even(A, P, start_row, start_col, 0); 
}

void mzd_apply_p_right_even_capped(mzd_t *A, mzp_t *P, size_t start_row, size_t start_col) {
  assert(A->offset == 0);
  if(!A->nrows)
    return;
  _mzd_apply_p_right_even(A, P, start_row, start_col, 1); 
}

void mzp_print(mzp_t *P) {
  printf("[ ");
  for(size_t i=0; i<P->length; i++) {
    printf("%zu ",P->values[i]);
  }
  printf("]");
}

#if 0
void  mzd_apply_p_right_trans_tri (mzd_t * A, mzp_t * P, size_t rank){
  assert(P->length==A->ncols);
  for (size_t i =0 ; i<P->length; ++i) {
    assert(P->values[i] >= i);
    //if (P->values[i] > i) {
    mzd_col_swap_in_rows(A, i, P->values[i], 0, i);
    //}
  }
}
#else
void mzd_apply_p_right_trans_tri(mzd_t *A, mzp_t *P) {
  assert(P->length==A->ncols);
  const size_t step_size = MAX((CPU_L1_CACHE>>2)/A->width,1);

  for(size_t r=0; r<A->nrows; r+=step_size) {
    const size_t row_bound = MIN(r+step_size, A->nrows);
    for (size_t i =0 ; i<A->ncols; ++i) {
      assert(P->values[i] >= i);
      //if (P->values[i] > i) {
      mzd_col_swap_in_rows(A, i, P->values[i], r, MIN(row_bound,i));
      //}
    }
  }
}
#endif
