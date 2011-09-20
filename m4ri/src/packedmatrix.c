/******************************************************************************
*
*            M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2007 Gregory Bard <gregory.bard@ieee.org> 
*    Copyright (C) 2009,2010 Martin Albrecht <martinralbrecht@googlemail.com>
*
*  Distributed under the terms of the GNU General Public License (GEL)
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

#include "misc.h"

#include <stdlib.h>
#include <string.h>
#include "packedmatrix.h"
#include "parity.h"

#define SAFECHAR (int)(RADIX+RADIX/3)

void mzd_copy_row_weird_to_even(mzd_t* B, size_t i, const mzd_t* A, size_t j);

mzd_t *mzd_init(size_t r, size_t c) {
  mzd_t *A;
  size_t i,j;

  A=(mzd_t *)m4ri_mmc_malloc(sizeof(mzd_t));

  A->width=DIV_CEIL(c,RADIX);

#ifdef HAVE_SSE2
  int incw = 0;
  /* make sure each row is 16-byte aligned */
  if (A->width & 1) {
    A->width++;
    incw = 1;
  }
#endif

  A->ncols=c;
  A->nrows=r;
  A->offset = 0;

  A->rows=(word **)m4ri_mmc_calloc( sizeof(word*), r+1); //we're overcomitting here

  if(r && c) {
    /* we allow more than one malloc call so he have to be a bit clever
       here */
    
    const size_t bytes_per_row = A->width*sizeof(word);
    const size_t max_rows_per_block = MM_MAX_MALLOC/bytes_per_row;
    assert(max_rows_per_block);
    size_t rest = r % max_rows_per_block;
    
    size_t nblocks = (rest == 0) ? r / max_rows_per_block : r / max_rows_per_block + 1;
    A->blocks = (mmb_t*)m4ri_mmc_calloc(nblocks + 1, sizeof(mmb_t));
    for(i=0; i<nblocks-1; i++) {
      A->blocks[i].size = MM_MAX_MALLOC;
      A->blocks[i].data = m4ri_mmc_calloc(MM_MAX_MALLOC,1);
      for(j=0; j<max_rows_per_block; j++) 
        A->rows[max_rows_per_block*i + j] = ((word*)A->blocks[i].data) + j*A->width;
    }
    if(rest==0)
      rest = max_rows_per_block;

    A->blocks[nblocks-1].size = rest * bytes_per_row;
    A->blocks[nblocks-1].data = m4ri_mmc_calloc(rest, bytes_per_row);
    for(j=0; j<rest; j++) {
      A->rows[max_rows_per_block*(nblocks-1) + j] = (word*)(A->blocks[nblocks-1].data) + j*A->width;
    }
  } else {
    A->blocks = NULL;
  }

#ifdef HAVE_SSE2
  if (incw) {
    A->width--;
  }
#endif

  return A;
}

mzd_t *mzd_init_window (const mzd_t *m, size_t lowr, size_t lowc, size_t highr, size_t highc) {
  size_t nrows, ncols, i, offset; 
  mzd_t *window;
  window = (mzd_t *)m4ri_mmc_malloc(sizeof(mzd_t));

  nrows = MIN(highr - lowr, m->nrows - lowr);
  ncols = highc - lowc;
  
  window->ncols = ncols;
  window->nrows = nrows;

  window->offset = (m->offset + lowc) % RADIX;
  offset = (m->offset + lowc) / RADIX;
  
  window->width = (window->offset + ncols) / RADIX;
  if ((window->offset + ncols) % RADIX)
    window->width++;
  window->blocks = NULL;

  if(nrows)
    window->rows = (word **)m4ri_mmc_calloc(sizeof(word*), nrows+1);
  else
    window->rows = NULL;

  for(i=0; i<nrows; i++) {
    window->rows[i] = m->rows[lowr + i] + offset;
  }
  
  return window;
}


void mzd_free( mzd_t *A) {
  if(A->rows)
    m4ri_mmc_free(A->rows, (A->nrows+1) * sizeof(word*));
  if(A->blocks) {
    size_t i;
    for(i=0; A->blocks[i].size; i++) {
      m4ri_mmc_free(A->blocks[i].data, A->blocks[i].size);
    }
    m4ri_mmc_free(A->blocks, (i+1) * sizeof(mmb_t));
  }
  m4ri_mmc_free(A, sizeof(mzd_t));
}

void mzd_print( const mzd_t *M ) {
  size_t i, j, wide;
  char temp[SAFECHAR];
  word *row;


  for (i=0; i< M->nrows; i++ ) {
    printf("[");
    row = M->rows[i];
    if(M->offset == 0) {
      for (j=0; j< M->width-1; j++) {
        m4ri_word_to_str(temp, row[j], 1);
        printf("%s ", temp);
      }
      row = row + M->width - 1;
      if(M->ncols%RADIX)
        wide = (size_t)M->ncols%RADIX;
      else
        wide = RADIX;
      for (j=0; j< wide; j++) {
        if (GET_BIT(*row, j)) 
          printf("1");
        else
          printf(" ");
        if (((j % 4)==3) && (j!=RADIX-1))
          printf(":");
      }
    } else {
      for (j=0; j< M->ncols; j++) {
        if(mzd_read_bit(M, i, j))
          printf("1");
        else
          printf(" ");
        if (((j % 4)==3) && (j!=RADIX-1))
          printf(":");
      }
    }
    printf("]\n");
  }
}

void mzd_print_tight( const mzd_t *M ) {
  assert(M->offset == 0);

  size_t i, j;
  char temp[SAFECHAR];
  word *row;

  for (i=0; i< M->nrows; i++ ) {
    printf("[");
    row = M->rows[i];
    for (j=0; j< M->ncols/RADIX; j++) {
      m4ri_word_to_str(temp, row[j], 0);
      printf("%s", temp);
    }
    row = row + M->width - 1;
    for (j=0; j< (int)(M->ncols%RADIX); j++) {
      printf("%d", (int)GET_BIT(*row, j));
    }
    printf("]\n");
  }
}

void mzd_row_add( mzd_t *m, size_t sourcerow, size_t destrow) {
  mzd_row_add_offset(m, destrow, sourcerow, 0);
}

int mzd_gauss_delayed(mzd_t *M, size_t startcol, int full) {
  assert(M->offset == 0);
  size_t i,j;
  size_t start; 

  size_t startrow = startcol;
  size_t ii;
  size_t pivots = 0;
  for (i=startcol ; i<M->ncols ; i++) {

    for(j=startrow ; j < M->nrows; j++) {
      if (mzd_read_bit(M,j,i)) {
	mzd_row_swap(M,startrow,j);
	pivots++;

	if (full==TRUE) 
          start=0; 
        else 
          start=startrow+1;

	for(ii=start ;  ii < M->nrows ; ii++) {
	  if (ii != startrow) {
	    if (mzd_read_bit(M, ii, i)) {
	      mzd_row_add_offset(M, ii, startrow, i);
	    }
	  }
	}
	startrow = startrow + 1;
	break;
      }
    }
  }

  return pivots;
}

int mzd_echelonize_naive(mzd_t *m, int full) { 
  return mzd_gauss_delayed(m, 0, full); 
}

/**
 * Transpose the 128 x 128-bit matrix inp and write the result in DST.
 */

static inline mzd_t *_mzd_transpose_direct_128(mzd_t *DST, const mzd_t *SRC) {
  assert(DST->offset==0);
  assert(SRC->offset==0);
  int j, k;
  word m, t[4];

  /* we do one recursion level 
   * [AB] -> [AC]
   * [CD]    [BD]
   */
  for(j=0; j<64; j++)  {
    DST->rows[   j][0] = SRC->rows[   j][0]; //A
    DST->rows[64+j][0] = SRC->rows[   j][1]; //B
    DST->rows[   j][1] = SRC->rows[64+j][0]; //C
    DST->rows[64+j][1] = SRC->rows[64+j][1]; //D
  }

  /* now transpose each block A,B,C,D separately, cf. Hacker's Delight */
  m = 0x00000000FFFFFFFFULL;
  for (j = 32; j != 0; j = j >> 1, m = m ^ (m << j)) {
    for (k = 0; k < 64; k = (k + j + 1) & ~j) {
      t[0] = (DST->rows[k][0] ^ (DST->rows[k+j][0] >> j)) & m;
      t[1] = (DST->rows[k][1] ^ (DST->rows[k+j][1] >> j)) & m;
      t[2] = (DST->rows[64+k][0] ^ (DST->rows[64+k+j][0] >> j)) & m;
      t[3] = (DST->rows[64+k][1] ^ (DST->rows[64+k+j][1] >> j)) & m;

      DST->rows[k][0] = DST->rows[k][0] ^ t[0];
      DST->rows[k][1] = DST->rows[k][1] ^ t[1];

      DST->rows[k+j][0] = DST->rows[k+j][0] ^ (t[0] << j);
      DST->rows[k+j][1] = DST->rows[k+j][1] ^ (t[1] << j);

      DST->rows[64+k][0] = DST->rows[64+k][0] ^ t[2];
      DST->rows[64+k][1] = DST->rows[64+k][1] ^ t[3];

      DST->rows[64+k+j][0] = DST->rows[64+k+j][0] ^ (t[2] << j);
      DST->rows[64+k+j][1] = DST->rows[64+k+j][1] ^ (t[3] << j);
    }
  }
  return DST;
}


static inline mzd_t *_mzd_transpose_direct(mzd_t *DST, const mzd_t *A) {
  size_t i,j,k, eol;
  word *temp;

  if(A->offset || DST->offset) {
    for(i=0; i<A->nrows; i++) {
      for(j=0; j<A->ncols; j++) {
        mzd_write_bit(DST, j, i, mzd_read_bit(A,i,j));
      }
    }
    return DST;
  }

  if (A->nrows == 128 && A->ncols == 128 && RADIX == 64) {
    _mzd_transpose_direct_128(DST, A);
    return DST;
  }

  if(DST->ncols%RADIX) {
    eol = RADIX*(DST->width-1);
  } else {
    eol = RADIX*(DST->width);
  }

  for (i=0; i<DST->nrows; i++) {
    temp = DST->rows[i];
    for (j=0; j < eol; j+=RADIX) {
      for (k=0; k<RADIX; k++) {
        *temp |= ((word)mzd_read_bit(A, j+k, i+A->offset))<<(RADIX-1-k);
      }
      temp++;
    }
    j = A->nrows - (A->nrows%RADIX);
    for (k=0; k<(size_t)(A->nrows%RADIX); k++) {
      *temp |= ((word)mzd_read_bit(A, j+k, i+A->offset))<<(RADIX-1-k);
    }
  }
  return DST;
}

static inline mzd_t *_mzd_transpose(mzd_t *DST, const mzd_t *X) {
  assert(X->offset == 0);

  const size_t nr = X->nrows;
  const size_t nc = X->ncols;
  const size_t cutoff = 128; // must be >= 128.

  if(nr <= cutoff || nc <= cutoff) {
    mzd_t *x = mzd_copy(NULL, X);
    _mzd_transpose_direct(DST, x);
    mzd_free(x);
    return DST;
  }

  /* we cut at multiples of 128 if possible, otherwise at multiples of 64 */
  size_t nr2 = (X->nrows > 256) ? 2*RADIX*(X->nrows/(4*RADIX)) : RADIX*(X->nrows/(2*RADIX));
  size_t nc2 = (X->ncols > 256) ? 2*RADIX*(X->ncols/(4*RADIX)) : RADIX*(X->ncols/(2*RADIX));

  mzd_t *A = mzd_init_window(X,    0,   0, nr2, nc2);
  mzd_t *B = mzd_init_window(X,    0, nc2, nr2,  nc);
  mzd_t *C = mzd_init_window(X,  nr2,   0,  nr, nc2);
  mzd_t *D = mzd_init_window(X,  nr2, nc2,  nr,  nc);

  mzd_t *AT = mzd_init_window(DST,   0,   0, nc2, nr2);
  mzd_t *CT = mzd_init_window(DST,   0, nr2, nc2,  nr);
  mzd_t *BT = mzd_init_window(DST, nc2,   0,  nc, nr2);
  mzd_t *DT = mzd_init_window(DST, nc2, nr2,  nc,  nr);

  _mzd_transpose(AT, A);
  _mzd_transpose(BT, B);
  _mzd_transpose(CT, C);
  _mzd_transpose(DT, D);

  mzd_free_window(A); mzd_free_window(B);
  mzd_free_window(C); mzd_free_window(D);

  mzd_free_window(AT); mzd_free_window(CT);
  mzd_free_window(BT); mzd_free_window(DT);
  
  return DST;
}

mzd_t *mzd_transpose(mzd_t *DST, const mzd_t *A) {
  if (DST == NULL) {
    DST = mzd_init( A->ncols, A->nrows );
  } else {
    if (DST->nrows != A->ncols || DST->ncols != A->nrows) {
      m4ri_die("mzd_transpose: Wrong size for return matrix.\n");
    }
  }
  if(A->offset || DST->offset)
    return _mzd_transpose_direct(DST, A);
  else
    return _mzd_transpose(DST, A);
}

mzd_t *mzd_mul_naive(mzd_t *C, const mzd_t *A, const mzd_t *B) {
  if (C==NULL) {
    C=mzd_init(A->nrows, B->ncols);
  } else {
    if (C->nrows != A->nrows || C->ncols != B->ncols) {
      m4ri_die("mzd_mul_naive: Provided return matrix has wrong dimensions.\n");
    }
  }
  if(B->ncols < RADIX-10) { /* this cutoff is rather arbitrary */
    mzd_t *BT = mzd_transpose(NULL, B);
    _mzd_mul_naive(C, A, BT, 1);
    mzd_free (BT);
  } else {
    _mzd_mul_va(C, A, B, 1);
  }
  return C;
}

mzd_t *mzd_addmul_naive(mzd_t *C, const mzd_t *A, const mzd_t *B) {
  if (C->nrows != A->nrows || C->ncols != B->ncols) {
    m4ri_die("mzd_mul_naive: Provided return matrix has wrong dimensions.\n");
  }

  if(B->ncols < RADIX-10) { /* this cutoff is rather arbitrary */
    mzd_t *BT = mzd_transpose(NULL, B);
    _mzd_mul_naive(C, A, BT, 0);
    mzd_free (BT);
  } else {
    _mzd_mul_va(C, A, B, 0);
  }
  return C;
}

mzd_t *_mzd_mul_naive(mzd_t *C, const mzd_t *A, const mzd_t *B, const int clear) {
  assert(A->offset == 0);
  assert(B->offset == 0);
  assert(C->offset == 0);
  size_t i, j, k, ii, eol;
  word *a, *b, *c;

  if (clear) {
    for (i=0; i<C->nrows; i++) {
      for (j=0; j<C->width-1; j++) {
  	C->rows[i][j] = 0;
      }
      C->rows[i][j] &= ~LEFT_BITMASK(C->ncols);
    }
  }

  if(C->ncols%RADIX) {
    eol = (C->width-1);
  } else {
    eol = (C->width);
  }

  word parity[64];
  for (i=0; i<64; i++) {
    parity[i] = 0;
  }
  const size_t wide = A->width;
  const size_t blocksize = MZD_MUL_BLOCKSIZE;
  size_t start;
  for (start = 0; start + blocksize <= C->nrows; start += blocksize) {
    for (i=start; i<start+blocksize; i++) {
      a = A->rows[i];
      c = C->rows[i];
      for (j=0; j<RADIX*eol; j+=RADIX) {
	for (k=0; k<RADIX; k++) {
          b = B->rows[j+k];
          parity[k] = a[0] & b[0];
          for (ii=wide-1; ii>=1; ii--)
	    parity[k] ^= a[ii] & b[ii];
        }
        c[j/RADIX] ^= parity64(parity);
      }
      
      if (eol != C->width) {
        for (k=0; k<(int)(C->ncols%RADIX); k++) {
          b = B->rows[RADIX*eol+k];
          parity[k] = a[0] & b[0];
          for (ii=1; ii<A->width; ii++)
            parity[k] ^= a[ii] & b[ii];
        }
        c[eol] ^= parity64(parity) & LEFT_BITMASK(C->ncols);
      }
    }
  }

  for (i=C->nrows - (C->nrows%blocksize); i<C->nrows; i++) {
    a = A->rows[i];
    c = C->rows[i];
    for (j=0; j<RADIX*eol; j+=RADIX) {
      for (k=0; k<RADIX; k++) {
        b = B->rows[j+k];
        parity[k] = a[0] & b[0];
        for (ii=wide-1; ii>=1; ii--)
          parity[k] ^= a[ii] & b[ii];
      }
      c[j/RADIX] ^= parity64(parity);
      }
    
    if (eol != C->width) {
      for (k=0; k<(int)(C->ncols%RADIX); k++) {
        b = B->rows[RADIX*eol+k];
        parity[k] = a[0] & b[0];
        for (ii=1; ii<A->width; ii++)
          parity[k] ^= a[ii] & b[ii];
      }
      c[eol] ^= parity64(parity) & LEFT_BITMASK(C->ncols);
    }
  }

  return C;
}

mzd_t *_mzd_mul_va(mzd_t *C, const mzd_t *v, const mzd_t *A, const int clear) {
  assert(C->offset == 0);
  assert(A->offset == 0);
  assert(v->offset == 0);

  if(clear)
    mzd_set_ui(C,0);

  size_t i,j;
  const size_t m=v->nrows;
  const size_t n=v->ncols;
  
  for(i=0; i<m; i++)
    for(j=0;j<n;j++)
      if (mzd_read_bit(v,i,j))
        mzd_combine(C,i,0,C,i,0,A,j,0);
  return C;
}

void mzd_randomize(mzd_t *A) {
  size_t i, j;
  assert(A->offset == 0);

  for (i=0; i < A->nrows; i++) {
    for (j=0; j < A->ncols; j++) {
      mzd_write_bit(A, i, j, m4ri_coin_flip() );
    }
  }
}

void mzd_set_ui( mzd_t *A, unsigned int value) {
  size_t i,j;
  size_t stop = MIN(A->nrows, A->ncols);

  word mask_begin = RIGHT_BITMASK(RADIX - A->offset);
  word mask_end = LEFT_BITMASK((A->offset + A->ncols)%RADIX);
  
  if(A->width==1) {
    for (i=0; i<A->nrows; i++) {
      for(j=0 ; j<A->ncols; j++)
        mzd_write_bit(A,i,j, 0);
    }
  } else {
    for (i=0; i<A->nrows; i++) {
      word *row = A->rows[i];
      row[0] &= ~mask_begin;
      for(j=1 ; j<A->width-1; j++)
        row[j] = 0;
      row[A->width - 1] &= ~mask_end;
    }
  }

  if(value%2 == 0)
    return;

  for (i=0; i<stop; i++) {
    mzd_write_bit(A, i, i, 1);
  }
}

BIT mzd_equal(const mzd_t *A, const mzd_t *B) {
  assert(A->offset == 0);
  assert(B->offset == 0);

  size_t i, j;

  if (A->nrows != B->nrows) return FALSE;
  if (A->ncols != B->ncols) return FALSE;

  for (i=0; i< A->nrows; i++) {
    for (j=0; j< A->width; j++) {
      if (A->rows[i][j] != B->rows[i][j])
	return FALSE;
    }
  }
  return TRUE;
}

int mzd_cmp(const mzd_t *A, const mzd_t *B) {
  assert(A->offset == 0);
  assert(B->offset == 0);

  size_t i,j;

  if(A->nrows < B->nrows) return -1;
  if(B->nrows < A->nrows) return 1;
  if(A->ncols < B->ncols) return -1;
  if(B->ncols < A->ncols) return 1;

  for(i=0; i < A->nrows ; i++) {
    for(j=0 ; j< A->width ; j++) {
      if ( A->rows[i][j] < B->rows[i][j] )
	return -1;
      else if( A->rows[i][j] > B->rows[i][j] )
	return 1;
    }
  }
  return 0;
}

mzd_t *mzd_copy(mzd_t *N, const mzd_t *P) {
  if (N == P)
    return N;

  if (!P->offset){
    if (N == NULL) {
      N = mzd_init(P->nrows, P->ncols);
    } else {
      if (N->nrows < P->nrows || N->ncols < P->ncols)
	m4ri_die("mzd_copy: Target matrix is too small.");
    }
    size_t i, j;
    word *p_truerow, *n_truerow;
    const size_t wide = P->width-1; 
    word mask = LEFT_BITMASK(P->ncols);
    for (i=0; i<P->nrows; i++) {
      p_truerow = P->rows[i];
      n_truerow = N->rows[i];
      for (j=0; j<wide; j++) {
       n_truerow[j] = p_truerow[j];
      }
      n_truerow[wide] = (n_truerow[wide] & ~mask) | (p_truerow[wide] & mask);
    }
  } else { // P->offset > 0
    if (N == NULL) {
      N = mzd_init(P->nrows, P->ncols+ P->offset);
      N->ncols -= P->offset;
      N->offset = P->offset;
      N->width=P->width;
    } else {
      if (N->nrows < P->nrows || N->ncols < P->ncols)
	m4ri_die("mzd_copy: Target matrix is too small.");
    }
    if(N->offset == P->offset) {
      for(size_t i=0; i<P->nrows; i++) {
        mzd_copy_row(N, i, P, i);
      }
    } else if(N->offset == 0) {
      for(size_t i=0; i<P->nrows; i++) {
        mzd_copy_row_weird_to_even(N, i, P, i);
      }
    } else {
      m4ri_die("mzd_copy: completely unaligned copy not implemented yet.");
    }
  }
/*     size_t i, j, p_truerow, n_truerow; */
/*     /\** */
/*      * \todo This is wrong  */
/*      *\/ */
/*     int trailingdim =  RADIX - P->ncols - P->offset; */

/*     if (trailingdim >= 0) { */
/*       // All columns fit in one word */
/*       word mask = ((ONE << P->ncols) - 1) << trailingdim; */
/*       for (i=0; i<P->nrows; i++) { */
/* 	p_truerow = P->rowswap[i]; */
/* 	n_truerow = N->rowswap[i]; */
/* 	N->values[n_truerow] = (N->values[n_truerow] & ~mask) | (P->values[p_truerow] & mask); */
/*       } */
/*     } else { */
/*       int r = (P->ncols + P->offset) % RADIX; */
/*       word mask_begin = RIGHT_BITMASK(RADIX - P->offset);  */
/*       word mask_end = LEFT_BITMASK(r); */
/*       for (i=0; i<P->nrows; i++) { */
/* 	p_truerow = P->rowswap[i]; */
/* 	n_truerow = N->rowswap[i]; */
/* 	N->values[n_truerow] = (N->values[n_truerow] & ~mask_begin) | (P->values[p_truerow] & mask_begin); */
/* 	for (j=1; j<P->width-1; j++) { */
/* 	  N->values[n_truerow + j] = P->values[p_truerow + j]; */
/* 	} */
/* 	N->values[n_truerow + j] = (N->values[n_truerow + j] & ~mask_end) | (P->values[p_truerow + j] & mask_end); */
/*       } */
/*     } */
  return N;
}

/* This is sometimes called augment */
mzd_t *mzd_concat(mzd_t *C, const mzd_t *A, const mzd_t *B) {
  assert(A->offset == 0);
  assert(B->offset == 0);
  size_t i, j;
  word *src_truerow, *dst_truerow;
  
  if (A->nrows != B->nrows) {
    m4ri_die("mzd_concat: Bad arguments to concat!\n");
  }

  if (C == NULL) {
    C = mzd_init(A->nrows, A->ncols + B->ncols);
  } else if (C->nrows != A->nrows || C->ncols != (A->ncols + B->ncols)) {
    m4ri_die("mzd_concat: C has wrong dimension!\n");
  }

  for (i=0; i<A->nrows; i++) {
    dst_truerow = C->rows[i];
    src_truerow = A->rows[i];
    for (j=0; j <A->width; j++) {
      dst_truerow[j] = src_truerow[j];
    }
  }

  for (i=0; i<B->nrows; i++) {
    for (j=0; j<B->ncols; j++) {
      mzd_write_bit(C, i, j+(A->ncols), mzd_read_bit(B, i, j) );
    }
  }

  return C;
}

mzd_t *mzd_stack(mzd_t *C, const mzd_t *A, const mzd_t *B) {
  assert(A->offset == 0);
  assert(B->offset == 0);
  size_t i, j;
  word *src_truerow, *dst_truerow;

  if (A->ncols != B->ncols) {
    m4ri_die("mzd_stack: A->ncols (%d) != B->ncols (%d)!\n",A->ncols, B->ncols);
  }

  if (C == NULL) {
    C = mzd_init(A->nrows + B->nrows, A->ncols);
  } else if (C->nrows != (A->nrows + B->nrows) || C->ncols != A->ncols) {
    m4ri_die("mzd_stack: C has wrong dimension!\n");
  }
  
  for(i=0; i<A->nrows; i++) {
    src_truerow = A->rows[i];
    dst_truerow = C->rows[i];
    for (j=0; j<A->width; j++) {
      dst_truerow[j] = src_truerow[j]; 
    }
  }

  for(i=0; i<B->nrows; i++) {
    dst_truerow = C->rows[A->nrows + i];
    src_truerow = B->rows[i];
    for (j=0; j<B->width; j++) {
      dst_truerow[j] = src_truerow[j]; 
    }
  }
  return C;
}

mzd_t *mzd_invert_naive(mzd_t *INV, mzd_t *A, const mzd_t *I) {
  assert(A->offset == 0);
  mzd_t *H;
  int x;

  H = mzd_concat(NULL, A, I);

  x = mzd_echelonize_naive(H, TRUE);

  if (x == FALSE) { 
    mzd_free(H); 
    return NULL; 
  }
  
  INV = mzd_submatrix(INV, H, 0, A->ncols, A->nrows, A->ncols*2);

  mzd_free(H);
  return INV;
}

mzd_t *mzd_add(mzd_t *ret, const mzd_t *left, const mzd_t *right) {
  if (left->nrows != right->nrows || left->ncols != right->ncols) {
    m4ri_die("mzd_add: rows and columns must match.\n");
  }
  if (ret == NULL) {
    ret = mzd_init(left->nrows, left->ncols);
  } else if (ret != left) {
    if (ret->nrows != left->nrows || ret->ncols != left->ncols) {
      m4ri_die("mzd_add: rows and columns of returned matrix must match.\n");
    }
  }
  return _mzd_add(ret, left, right);
}

mzd_t *_mzd_add(mzd_t *C, const mzd_t *A, const mzd_t *B) {
  size_t i;
  size_t nrows = MIN(MIN(A->nrows, B->nrows), C->nrows);
  const mzd_t *tmp;

  if (C == B) { //swap
    tmp = A;
    A = B;
    B = tmp;
  }
  
  for(i=0; i<nrows; i++) {
    mzd_combine(C,i,0, A,i,0, B,i,0);
  }
  return C;
}

mzd_t *mzd_submatrix(mzd_t *S, const mzd_t *M, const size_t startrow, const size_t startcol, const size_t endrow, const size_t endcol) {
  size_t nrows, ncols, i, colword, x, y, block, spot, startword;
  word *truerow;
  word temp  = 0;
  
  nrows = endrow - startrow;
  ncols = endcol - startcol;

  if (S == NULL) {
    S = mzd_init(nrows, ncols);
  } else if(S->nrows < nrows || S->ncols < ncols) {
    m4ri_die("mzd_submatrix: got S with dimension %d x %d but expected %d x %d\n",S->nrows,S->ncols,nrows,ncols);
  }
  assert(M->offset == S->offset);

  startword = (M->offset + startcol) / RADIX;

  /* we start at the beginning of a word */
  if ((M->offset + startcol)%RADIX == 0) {
    if(ncols/RADIX) {
      for(x = startrow, i=0; i<nrows; i++, x++) {
        memcpy(S->rows[i], M->rows[x] + startword, 8*(ncols/RADIX));
      }
    }
    if (ncols%RADIX) {
      for(x = startrow, i=0; i<nrows; i++, x++) {
        /* process remaining bits */
	temp = M->rows[x][startword + ncols/RADIX] & LEFT_BITMASK(ncols);
	S->rows[i][ncols/RADIX] = temp;
      } 
    }
    /* startcol is not the beginning of a word */
  } else { 
    spot = (M->offset + startcol) % RADIX;
    for(x = startrow, i=0; i<nrows; i++, x+=1) {
      truerow = M->rows[x];

      /* process full words first */
      for(colword=0; colword<(int)(ncols/RADIX); colword++) {
	block = colword + startword;
	temp = (truerow[block] << (spot)) | (truerow[block + 1] >> (RADIX-spot) ); 
	S->rows[i][colword] = temp;
      }
      /* process remaining bits (lazy)*/
      colword = ncols/RADIX;
      for (y=0; y < (int)(ncols%RADIX); y++) {
	temp = mzd_read_bit(M, x, startcol + colword*RADIX + y);
	mzd_write_bit(S, i, colword*RADIX + y, (BIT)temp);
      }
    }
  }
  return S;
}

void mzd_combine( mzd_t * C, const size_t c_row, const size_t c_startblock,
		  const mzd_t * A, const size_t a_row, const size_t a_startblock, 
		  const mzd_t * B, const size_t b_row, const size_t b_startblock) {

  size_t i;
  if(C->offset || A->offset || B->offset) {
    /**
     * \todo this code is slow if offset!=0 
     */
    for(i=0; i+RADIX<=A->ncols; i+=RADIX) {
      const word tmp = mzd_read_bits(A, a_row, i, RADIX) ^ mzd_read_bits(B, b_row, i, RADIX);
      for(size_t j=0; j<RADIX; j++) {
        mzd_write_bit(C, c_row, i*RADIX+j, GET_BIT(tmp, j));
      }
    }
    for( ; i<A->ncols; i++) {
      mzd_write_bit(C, c_row, i, mzd_read_bit(A, a_row, i) ^ mzd_read_bit(B, b_row, i));
    }
    return;
  }

  size_t wide = A->width - a_startblock;

  word *a = a_startblock + A->rows[a_row];
  word *b = b_startblock + B->rows[b_row];
  
  if( C == A && a_row == c_row && a_startblock == c_startblock) {
#ifdef HAVE_SSE2
    if(wide > SSE2_CUTOFF) {
      /** check alignments **/
      if (ALIGNMENT(a,16)) {
        *a++ ^= *b++;
        wide--;
      }

      if (ALIGNMENT(a,16)==0 && ALIGNMENT(b,16)==0) {
	__m128i *a128 = (__m128i*)a;
	__m128i *b128 = (__m128i*)b;
	const __m128i *eof = (__m128i*)((unsigned long)(a + wide) & ~0xF);

	do {
	  *a128 = _mm_xor_si128(*a128, *b128);
	  ++b128;
	  ++a128;
	} while(a128 < eof);
	
	a = (word*)a128;
	b = (word*)b128;
	wide = ((sizeof(word)*wide)%16)/sizeof(word);
      }
    }
#endif //HAVE_SSE2
    for(i=0; i < wide; i++)
      a[i] ^= b[i];
    return;
    
  } else { /* C != A */
    word *c = c_startblock + C->rows[c_row];

    /* this is a corner case triggered by Strassen multiplication
       which assumes certain (virtual) matrix sizes */
    if (a_row >= A->nrows) {
      for(i = 0; i<wide; i++) {
        c[i] = b[i];
      }
    } else {
#ifdef HAVE_SSE2
    if(wide > SSE2_CUTOFF) {
      /** check alignments **/
      if (ALIGNMENT(a,16)) {
        *c++ = *b++ ^ *a++;
        wide--;
      }

      if ((ALIGNMENT(b,16)==0) && (ALIGNMENT(c,16)==0)) {
	__m128i *a128 = (__m128i*)a;
	__m128i *b128 = (__m128i*)b;
	__m128i *c128 = (__m128i*)c;
	const __m128i *eof = (__m128i*)((unsigned long)(a + wide) & ~0xF);
	
	do {
          *c128 = _mm_xor_si128(*a128, *b128);
	  ++c128;
	  ++b128;
	  ++a128;
	} while(a128 < eof);
	
	a = (word*)a128;
	b = (word*)b128;
	c = (word*)c128;
	wide = ((sizeof(word)*wide)%16)/sizeof(word);
      }
    }
#endif //HAVE_SSE2
    for(i = 0; i<wide; i++) {
      c[i] = a[i] ^ b[i];
    }
    return;
    }
  }
}


void mzd_col_swap(mzd_t *M, const size_t cola, const size_t colb) {
  if (cola == colb)
    return;

  const size_t _cola = cola + M->offset;
  const size_t _colb = colb + M->offset;

  const size_t a_word = _cola/RADIX;
  const size_t b_word = _colb/RADIX;
  const size_t a_bit = _cola%RADIX;
  const size_t b_bit = _colb%RADIX;
  
  word a, b, *base;

  size_t i;
  
  if(a_word == b_word) {
    const word ai = RADIX - a_bit - 1;
    const word bi = RADIX - b_bit - 1;
    for (i=0; i<M->nrows; i++) {
      base = (M->rows[i] + a_word);
      register word b = *base;
      register word x = ((b >> ai) ^ (b >> bi)) & 1; // XOR temporary
      *base = b ^ ((x << ai) | (x << bi));
    }
    return;
  }

  const word a_bm = (ONE<<(RADIX - (a_bit) - 1));
  const word b_bm = (ONE<<(RADIX - (b_bit) - 1));

  if(a_bit > b_bit) {
    const size_t offset = a_bit - b_bit;

    for (i=0; i<M->nrows; i++) {
      base = M->rows[i];
      a = *(base + a_word);
      b = *(base + b_word);

      a ^= (b & b_bm) >> offset;
      b ^= (a & a_bm) << offset;
      a ^= (b & b_bm) >> offset;

      *(base + a_word) = a;
      *(base + b_word) = b;
    }
  } else {
    const size_t offset = b_bit - a_bit;
    for (i=0; i<M->nrows; i++) {
      base = M->rows[i];

      a = *(base + a_word);
      b = *(base + b_word);

      a ^= (b & b_bm) << offset;
      b ^= (a & a_bm) >> offset;
      a ^= (b & b_bm) << offset;
      *(base + a_word) = a;
      *(base + b_word) = b;
    }
  }

}


int mzd_is_zero(mzd_t *A) {
  /* Could be improved: stopping as the first non zero value is found (status!=0)*/
  size_t mb = A->nrows;
  size_t nb = A->ncols;
  size_t Aoffset = A->offset;
  size_t nbrest = (nb + Aoffset) % RADIX;
  word status=0;
  if (nb + Aoffset >= RADIX) {
    // Large A
    word mask_begin = RIGHT_BITMASK(RADIX-Aoffset);
    if (Aoffset == 0)
      mask_begin = ~mask_begin;
    word mask_end = LEFT_BITMASK(nbrest);
    size_t i;
    for (i=0; i<mb; ++i) {
        status |= A->rows[i][0] & mask_begin;
        size_t j;
        for ( j = 1; j < A->width-1; ++j)
            status |= A->rows[i][j];
        status |= A->rows[i][A->width - 1] & mask_end;
    }
  } else {
    // Small A
    word mask = LEFT_BITMASK(nb);
    size_t i;
    for (i=0; i < mb; ++i) {
      status |= A->rows[i][0] & mask;
    }
  }
  
  return (int)(!status);
}

void mzd_copy_row_weird_to_even(mzd_t* B, size_t i, const mzd_t* A, size_t j) {
  assert(B->offset == 0);
  assert(B->ncols >= A->ncols);

  word *b = B->rows[j];
  size_t c;

  size_t rest = A->ncols%RADIX;

  for(c = 0; c+RADIX <= A->ncols; c+=RADIX) {
    b[c/RADIX] = mzd_read_bits(A, i, c, RADIX);
  }
  if (rest) {
    const word temp = mzd_read_bits(A, i, c, rest);
    b[c/RADIX] &= LEFT_BITMASK(RADIX-rest);
    b[c/RADIX] |= temp<<(RADIX-rest);
  }
}

void mzd_copy_row(mzd_t* B, size_t i, const mzd_t* A, size_t j) {
  assert(B->offset == A->offset);
  assert(B->ncols >= A->ncols);
  size_t k;
  const size_t width= MIN(B->width, A->width) - 1;

  word* a = A->rows[j];
  word* b = B->rows[i];
 
  word mask_begin = RIGHT_BITMASK(RADIX - A->offset);
  word mask_end = LEFT_BITMASK( (A->offset + A->ncols)%RADIX );

  if (width != 0) {
    b[0] = (b[0] & ~mask_begin) | (a[0] & mask_begin);
    for(k = 1; k<width; k++)
      b[k] = a[k];
    b[width] = (b[width] & ~mask_end) | (a[width] & mask_end);
    
  } else {
    b[0] = (b[0] & ~mask_begin) | (a[0] & mask_begin & mask_end) | (b[0] & ~mask_end);
  }
}


void mzd_row_clear_offset(mzd_t *M, size_t row, size_t coloffset) {
  coloffset += M->offset;
  size_t startblock= coloffset/RADIX;
  size_t i;
  word temp;

  /* make sure to start clearing at coloffset */
  if (coloffset%RADIX) {
    temp = M->rows[row][startblock];
    temp &= RIGHT_BITMASK(RADIX - coloffset);
  } else {
    temp = 0;
  }
  M->rows[row][startblock] = temp;
  for ( i=startblock+1; i < M->width; i++ ) {
    M->rows[row][i] = 0ULL;
  }
}


int mzd_find_pivot(mzd_t *A, size_t start_row, size_t start_col, size_t *r, size_t *c) { 
  assert(A->offset == 0);
  register size_t i = start_row;
  register size_t j = start_col;
  const size_t nrows = A->nrows;
  const size_t ncols = A->ncols;
  size_t row_candidate = 0;
  word data = 0;
  if(A->ncols - start_col < RADIX) {
    for(j=start_col; j<A->ncols; j+=RADIX) {
      const size_t length = MIN(RADIX, ncols-j);
      for(i=start_row; i<nrows; i++) {
        const word curr_data = (word)mzd_read_bits(A, i, j, length);
        if (curr_data > data && leftmost_bit(curr_data) > leftmost_bit(data)) {
          row_candidate = i;
          data = curr_data;
          if(GET_BIT(data,RADIX-length-1))
            break;
        }
      }
      if(data) {
        i = row_candidate;
        data <<=(RADIX-length);
        for(size_t l=0; l<length; l++) {
          if(GET_BIT(data, l)) {
            j+=l;
            break;
          }
        }
        *r = i, *c = j;
        return 1;
      }
    }
  } else {
    /* we definitely have more than one word */
    /* handle first word */
    const size_t bit_offset = (start_col % RADIX);
    const size_t word_offset = start_col / RADIX;
    const word mask_begin = RIGHT_BITMASK(RADIX-bit_offset);
    for(i=start_row; i<nrows; i++) {
      const word curr_data = A->rows[i][word_offset] & mask_begin;
      if (curr_data > data && leftmost_bit(curr_data) > leftmost_bit(data)) {
        row_candidate = i;
        data = curr_data;
        if(GET_BIT(data,bit_offset)) {
          break;
        }
      }
    }
    if(data) {
      i = row_candidate;
      data <<=bit_offset;
      for(size_t l=0; l<(RADIX-bit_offset); l++) {
        if(GET_BIT(data, l)) {
          j+=l;
          break;
        }
      }
      *r = i, *c = j;
      return 1;
    }
    /* handle complete words */
    for(j=word_offset + 1; j<A->width - 1; j++) {
      for(i=start_row; i<nrows; i++) {
        const word curr_data = A->rows[i][j];
        if (curr_data > data && leftmost_bit(curr_data) > leftmost_bit(data)) {
          row_candidate = i;
          data = curr_data;
          if(GET_BIT(data, 0))
            break;
        }
      }
      if(data) {
        i = row_candidate;
        for(size_t l=0; l<RADIX; l++) {
          if(GET_BIT(data, l)) {
            j=j*RADIX + l;
            break;
          }
        }
        *r = i, *c = j;
        return 1;
      }
    }
    /* handle last word */
    const size_t end_offset = A->ncols % RADIX ? (A->ncols%RADIX) : RADIX;
    const word mask_end = LEFT_BITMASK(end_offset);
    j = A->width-1;
    for(i=start_row; i<nrows; i++) {
      const word curr_data = A->rows[i][j] & mask_end;
      if (curr_data > data && leftmost_bit(curr_data) > leftmost_bit(data)) {
        row_candidate = i;
        data = curr_data;
        if(GET_BIT(data,0))
          break;
      }
    }
    if(data) {
      i = row_candidate;
      for(size_t l=0; l<end_offset; l++) {
        if(GET_BIT(data, l)) {
          j=j*RADIX+l;
          break;
        }
      }
      *r = i, *c = j;
      return 1;
    }
  }
  return 0;
}


#define MASK(c)    (((word)(-1)) / (TWOPOW(TWOPOW(c)) + ONE))
#define COUNT(x,c) ((x) & MASK(c)) + (((x) >> (TWOPOW(c))) & MASK(c))

static inline int m4ri_bitcount(word n)  {
   n = COUNT(n, 0);
   n = COUNT(n, 1);
   n = COUNT(n, 2);
   n = COUNT(n, 3);
   n = COUNT(n, 4);
   n = COUNT(n, 5);
   return (int)n;
}


double _mzd_density(mzd_t *A, int res, size_t r, size_t c) {
  size_t count = 0;
  size_t total = 0;
  
  if(res == 0)
    res = (int)(A->width/100.0);
  if (res < 1)
    res = 1;

  if(A->width == 1) {
    for(size_t i=r; i<A->nrows; i++)
      for(size_t j=c; j<A->ncols; j++)
        if(mzd_read_bit(A, i, j))
          count++;
    return ((double)count)/(A->ncols * A->nrows);
  }

  for(size_t i=r; i<A->nrows; i++) {
    word *truerow = A->rows[i];
    for(size_t j = c; j< RADIX-A->offset; j++)
      if(mzd_read_bit(A, i, j))
        count++;
    total += RADIX - A->offset;

    for(size_t j=MAX(1,((A->offset+c)/RADIX)); j<A->width-1; j+=res) {
        count += m4ri_bitcount(truerow[j]);
        total += RADIX;
    }
    for(size_t j = 0; j < (A->offset + A->ncols)%RADIX; j++)
      if(mzd_read_bit(A, i, j+ RADIX*((A->offset + A->ncols)/RADIX)))
        count++;
    total += (A->offset + A->ncols)%RADIX;
  }

  return ((double)count)/(total);
}

double mzd_density(mzd_t *A, int res) {
  return _mzd_density(A, res, 0, 0);
}

size_t mzd_first_zero_row(mzd_t *A) {
  word mask_begin = RIGHT_BITMASK(RADIX-A->offset);
  word mask_end = LEFT_BITMASK((A->ncols + A->offset)%RADIX);
  const size_t end = A->width - 1;
  word *row;

  for(long i = A->nrows - 1; i>=0; i--) {
    word tmp = 0;
    row = A->rows[i];
    tmp |= row[0] & mask_begin;
    for (size_t j = 1; j < end; ++j)
      tmp |= row[j];
    tmp |= row[end] & mask_end;
    if(tmp)
      return i+1;
  }
  return 0;
}
