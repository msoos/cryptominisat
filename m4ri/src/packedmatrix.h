/**
 * \file packedmatrix.h
 * \brief Dense matrices over GF(2) represented as a bit field.
 *
 * \author Gregory Bard <bard@fordham.edu>
 * \author Martin Albrecht <M.R.Albrecht@rhul.ac.uk>
 */

#ifndef PACKEDMATRIX_H
#define PACKEDMATRIX_H
/*******************************************************************
*
*                M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2007, 2008 Gregory Bard <bard@fordham.edu>
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

#include <math.h>
#include <assert.h>
#include <stdio.h>

#ifdef HAVE_SSE2
#include <emmintrin.h>
#endif


#ifdef HAVE_SSE2
/**
 * \brief SSE2 curoff in words.
 *
 * Cutoff in words after which row length SSE2 instructions should be
 * used.
 */

#define SSE2_CUTOFF 20
#endif

/**
 * \brief Matrix multiplication block-ing dimension.
 * 
 * Defines the number of rows of the matrix A that are
 * processed as one block during the execution of a multiplication
 * algorithm.
 */

#define MZD_MUL_BLOCKSIZE MIN(((int)sqrt((double)(4*CPU_L2_CACHE)))/2,2048)


/**
 * \brief Dense matrices over GF(2). 
 * 
 * The most fundamental data type in this library.
 */

typedef struct {
  /**
   * Contains pointers to the actual blocks of memory containing the
   * values packed into words of size RADIX.
   */

  mmb_t *blocks;

  /**
   * Number of rows.
   */

  size_t nrows;

  /**
   * Number of columns.
   */

  size_t ncols;

  /**
   * width = ceil(ncols/RADIX)
   */
  size_t width; 

  /**
   * column offset of the first column.
   */

  size_t offset;
  
  /**
   * Address of first word in each row, so the first word of row i is
   * is m->rows[i]
   */

  word **rows;

} mzd_t;

/**
 * \brief Create a new matrix of dimension r x c.
 *
 * Use mzd_free to kill it.
 *
 * \param r Number of rows
 * \param c Number of columns
 *
 */

mzd_t *mzd_init(const size_t r, const size_t c);

/**
 * \brief Free a matrix created with mzd_init.
 * 
 * \param A Matrix
 */

void mzd_free(mzd_t *A);


/**
 * \brief Create a window/view into the matrix M.
 *
 * A matrix window for M is a meta structure on the matrix M. It is
 * setup to point into the matrix so M \em must \em not be freed while the
 * matrix window is used.
 *
 * This function puts the restriction on the provided parameters that
 * all parameters must be within range for M which is not enforced
 * currently .
 *
 * Use mzd_free_window to free the window.
 *
 * \param M Matrix
 * \param lowr Starting row (inclusive)
 * \param lowc Starting column (inclusive)
 * \param highr End row (exclusive)
 * \param highc End column (exclusive)
 *
 */

mzd_t *mzd_init_window(const mzd_t *M, const size_t lowr, const size_t lowc, const size_t highr, const size_t highc);

/**
 * \brief Free a matrix window created with mzd_init_window.
 * 
 * \param A Matrix
 */

#define mzd_free_window mzd_free

/**
 * \brief Swap the two rows rowa and rowb.
 * 
 * \param M Matrix
 * \param rowa Row index.
 * \param rowb Row index.
 */
 
static inline void mzd_row_swap(mzd_t *M, const size_t rowa, const size_t rowb) {
  if(rowa == rowb)
    return;
  size_t i;
  size_t width = M->width - 1;
  word *a = M->rows[rowa];
  word *b = M->rows[rowb];
  word tmp; 
  word mask_begin = RIGHT_BITMASK(RADIX - M->offset);
  word mask_end = LEFT_BITMASK( (M->offset + M->ncols)%RADIX );

  if (width != 0) {
    tmp = a[0];
    a[0] = (a[0] & ~mask_begin) | (b[0] & mask_begin);
    b[0] = (b[0] & ~mask_begin) | (tmp & mask_begin);
    
    for(i = 1; i<width; i++) {
      tmp = a[i];
      a[i] = b[i];
      b[i] = tmp;
    }
    tmp = a[width];
    a[width] = (a[width] & ~mask_end) | (b[width] & mask_end);
    b[width] = (b[width] & ~mask_end) | (tmp & mask_end);
    
  } else {
    tmp = a[0];
    a[0] = (a[0] & ~mask_begin) | (b[0] & mask_begin & mask_end) | (a[0] & ~mask_end);
    b[0] = (b[0] & ~mask_begin) | (tmp & mask_begin & mask_end) | (b[0] & ~mask_end);
  }
}

/**
 * \brief copy row j from A to row i from B.
 *
 * The offsets of A and B must match and the number of columns of A
 * must be less than or equal to the number of columns of B.
 *
 * \param B Target matrix.
 * \param i Target row index.
 * \param A Source matrix.
 * \param j Source row index.
 */

void mzd_copy_row(mzd_t* B, size_t i, const mzd_t* A, size_t j);

/**
 * \brief Swap the two columns cola and colb.
 * 
 * \param M Matrix.
 * \param cola Column index.
 * \param colb Column index.
 */
 
void mzd_col_swap(mzd_t *M, const size_t cola, const size_t colb);

/**
 * \brief Swap the two columns cola and colb but only between start_row and stop_row.
 * 
 * \param M Matrix.
 * \param cola Column index.
 * \param colb Column index.
 * \param start_row Row index.
 * \param stop_row Row index (exclusive).
 */
 
static inline void mzd_col_swap_in_rows(mzd_t *M, const size_t cola, const size_t colb, const size_t start_row, const size_t stop_row) {
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
    for (i=start_row; i<stop_row; i++) {
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
    for (i=start_row; i<stop_row; i++) {
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
    for (i=start_row; i<stop_row; i++) {
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

/**
 * \brief Read the bit at position M[row,col].
 *
 * \param M Matrix
 * \param row Row index
 * \param col Column index
 *
 * \note No bounds checks whatsoever are performed.
 *
 */

static inline BIT mzd_read_bit(const mzd_t *M, const size_t row, const size_t col ) {
  return GET_BIT(M->rows[row][(col+M->offset)/RADIX], (col+M->offset) % RADIX);
}

/**
 * \brief Write the bit value to position M[row,col]
 * 
 * \param M Matrix
 * \param row Row index
 * \param col Column index
 * \param value Either 0 or 1 
 *
 * \note No bounds checks whatsoever are performed.
 *
 */

static inline void mzd_write_bit(mzd_t *M, const size_t row, const size_t col, const BIT value) {
  if (value==1)
    SET_BIT(M->rows[row][(col+M->offset)/RADIX], (col+M->offset) % RADIX);
  else
    CLR_BIT(M->rows[row][(col+M->offset)/RADIX], (col+M->offset) % RADIX);
}

/**
 * \brief Print a matrix to stdout. 
 *
 * The output will contain colons between every 4-th column.
 *
 * \param M Matrix
 */

void mzd_print(const mzd_t *M);

/**
 * \brief Print the matrix to stdout.
 *
 * \param M Matrix
 */

void mzd_print_tight(const mzd_t *M);

/**
 * \brief Add the rows sourcerow and destrow and stores the total in the row
 * destrow, but only begins at the column coloffset.
 *
 * \param M Matrix
 * \param dstrow Index of target row
 * \param srcrow Index of source row
 * \param coloffset Column offset
 */

/*void mzd_row_add_offset(mzd_t *M, size_t dstrow, size_t srcrow, size_t coloffset); */
static inline void mzd_row_add_offset(mzd_t *M, size_t dstrow, size_t srcrow, size_t coloffset) {
  coloffset += M->offset;
  const size_t startblock= coloffset/RADIX;
  size_t wide = M->width - startblock;
  word *src = M->rows[srcrow] + startblock;
  word *dst = M->rows[dstrow] + startblock;

  if(!wide)
    return;

  word temp = *src++;
  if (coloffset%RADIX)
    temp = RIGHTMOST_BITS(temp, (RADIX-(coloffset%RADIX)-1));
  *dst++ ^= temp;
  wide--;

#ifdef HAVE_SSE2 
  if (ALIGNMENT(src,16)==8 && wide) {
    *dst++ ^= *src++;
    wide--;
  }
  __m128i *__src = (__m128i*)src;
  __m128i *__dst = (__m128i*)dst;
  const __m128i *eof = (__m128i*)((unsigned long)(src + wide) & ~0xF);
  __m128i xmm1;
  
  while(__src < eof) {
    xmm1 = _mm_xor_si128(*__dst, *__src++);
    *__dst++ = xmm1;
  }
  src  = (word*)__src;
  dst = (word*)__dst;
  wide = ((sizeof(word)*wide)%16)/sizeof(word);
#endif
  size_t i;
  for(i=0; i<wide; i++) {
    dst[i] ^= src[i];
  }
}

/**
 * \brief Add the rows sourcerow and destrow and stores the total in
 * the row destrow.
 *
 * \param M Matrix
 * \param sourcerow Index of source row
 * \param destrow Index of target row
 *
 * \note this can be done much faster with mzd_combine.
 */

void mzd_row_add(mzd_t *M, const size_t sourcerow, const size_t destrow);

/**
 * \brief Transpose a matrix.
 *
 * This function uses the fact that:
\verbatim
   [ A B ]T    [AT CT]
   [ C D ]  =  [BT DT] 
 \endverbatim 
 * and thus rearranges the blocks recursively. 
 *
 * \param DST Preallocated return matrix, may be NULL for automatic creation.
 * \param A Matrix
 */

mzd_t *mzd_transpose(mzd_t *DST, const mzd_t *A );

/**
 * \brief Naive cubic matrix multiplication.
 *
 * That is, compute C such that C == AB.
 *
 * \param C Preallocated product matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \note Normally, if you will multiply several times by b, it is
 * smarter to calculate bT yourself, and keep it, and then use the
 * function called _mzd_mul_naive
 *
 */
mzd_t *mzd_mul_naive(mzd_t *C, const mzd_t *A, const mzd_t *B);

/**
 * \brief Naive cubic matrix multiplication and addition
 *
 * That is, compute C such that C == C + AB.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \note Normally, if you will multiply several times by b, it is
 * smarter to calculate bT yourself, and keep it, and then use the
 * function called _mzd_mul_naive
 */

mzd_t *mzd_addmul_naive(mzd_t *C, const mzd_t *A, const mzd_t *B);

/**
 * \brief Naive cubic matrix multiplication with the pre-transposed B.
 *
 * That is, compute C such that C == AB^t.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Pre-transposed input matrix B.
 * \param clear Whether to clear C before accumulating AB
 */

mzd_t *_mzd_mul_naive(mzd_t *C, const mzd_t *A, const mzd_t *B, const int clear);

/**
 * \brief Matrix multiplication optimized for v*A where v is a vector.
 *
 * \param C Preallocated product matrix.
 * \param v Input matrix v.
 * \param A Input matrix A.
 * \param clear If set clear C first, otherwise add result to C.
 *
 */
mzd_t *_mzd_mul_va(mzd_t *C, const mzd_t *v, const mzd_t *A, const int clear);

/**
 * \brief Fill matrix M with uniformly distributed bits.
 *
 * \param M Matrix
 *
 * \todo Allow the user to provide a RNG callback.
 *
 * \wordoffset
 */

void mzd_randomize(mzd_t *M);

/**
 * \brief Set the matrix M to the value equivalent to the integer
 * value provided.
 *
 * Specifically, this function does nothing if value%2 == 0 and
 * returns the identity matrix if value%2 == 1.
 *
 * If the matrix is not square then the largest possible square
 * submatrix is set to the identity matrix.
 *
 * \param M Matrix
 * \param value Either 0 or 1
 */

void mzd_set_ui(mzd_t *M, const unsigned value);

/**
 * \brief Gaussian elimination.
 * 
 * This will do Gaussian elimination on the matrix m but will start
 * not at column 0 necc but at column startcol. If full=FALSE, then it
 * will do triangular style elimination, and if full=TRUE, it will do
 * Gauss-Jordan style, or full elimination.
 * 
 * \param M Matrix
 * \param startcol First column to consider for reduction.
 * \param full Gauss-Jordan style or upper triangular form only.
 *
 * \wordoffset
 */

int mzd_gauss_delayed(mzd_t *M, const size_t startcol, const int full);

/**
 * \brief Gaussian elimination.
 * 
 * This will do Gaussian elimination on the matrix m.  If full=FALSE,
 *  then it will do triangular style elimination, and if full=TRUE,
 *  it will do Gauss-Jordan style, or full elimination.
 *
 * \param M Matrix
 * \param full Gauss-Jordan style or upper triangular form only.
 *
 * \wordoffset
 * 
 * \sa mzd_echelonize_m4ri(), mzd_echelonize_pluq()
 */

int mzd_echelonize_naive(mzd_t *M, const int full);

/**
 * \brief Return TRUE if A == B.
 *
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 */

BIT mzd_equal(const mzd_t *A, const mzd_t *B );

/**
 * \brief Return -1,0,1 if if A < B, A == B or A > B respectively.
 *
 * \param A Matrix.
 * \param B Matrix.
 *
 * \note This comparison is not well defined mathematically and
 * relatively arbitrary since elements of GF(2) don't have an
 * ordering.
 *
 * \wordoffset
 */

int mzd_cmp(const mzd_t *A, const mzd_t *B);

/**
 * \brief Copy matrix  A to DST.
 *
 * \param DST May be NULL for automatic creation.
 * \param A Source matrix.
 *
 * \wordoffset
 */

mzd_t *mzd_copy(mzd_t *DST, const mzd_t *A);

/**
 * \brief Concatenate B to A and write the result to C.
 * 
 * That is,
 *
 \verbatim
 [ A ], [ B ] -> [ A  B ] = C
 \endverbatim
 *
 * The inputs are not modified but a new matrix is created.
 *
 * \param C Matrix, may be NULL for automatic creation
 * \param A Matrix
 * \param B Matrix
 *
 * \note This is sometimes called augment.
 *
 * \wordoffset
 */

mzd_t *mzd_concat(mzd_t *C, const mzd_t *A, const mzd_t *B);

/**
 * \brief Stack A on top of B and write the result to C.
 *
 * That is, 
 *
 \verbatim
 [ A ], [ B ] -> [ A ] = C
                 [ B ]
 \endverbatim
 *
 * The inputs are not modified but a new matrix is created.
 *
 * \param C Matrix, may be NULL for automatic creation
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 */

mzd_t *mzd_stack(mzd_t *C, const mzd_t *A, const mzd_t *B);

/**
 * \brief Copy a submatrix.
 * 
 * Note that the upper bounds are not included.
 *
 * \param S Preallocated space for submatrix, may be NULL for automatic creation.
 * \param M Matrix
 * \param lowr start rows
 * \param lowc start column
 * \param highr stop row (this row is \em not included)
 * \param highc stop column (this column is \em not included)
 */
mzd_t *mzd_submatrix(mzd_t *S, const mzd_t *M, const size_t lowr, const size_t lowc, const size_t highr, const size_t highc);

/**
 * \brief Invert the matrix target using Gaussian elimination. 
 *
 * To avoid recomputing the identity matrix over and over again, I may
 * be passed in as identity parameter.
 *
 * \param INV Preallocated space for inversion matrix, may be NULL for automatic creation.
 * \param A Matrix to be reduced.
 * \param I Identity matrix.
 *
 * \wordoffset
 */

mzd_t *mzd_invert_naive(mzd_t *INV, mzd_t *A, const mzd_t *I);

/**
 * \brief Set C = A+B.
 *
 * C is also returned. If C is NULL then a new matrix is created which
 * must be freed by mzd_free.
 *
 * \param C Preallocated sum matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 */

mzd_t *mzd_add(mzd_t *C, const mzd_t *A, const mzd_t *B);

/**
 * \brief Same as mzd_add but without any checks on the input.
 *
 * \param C Preallocated sum matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 */

mzd_t *_mzd_add(mzd_t *C, const mzd_t *A, const mzd_t *B);

/**
 * \brief Same as mzd_add.
 *
 * \param C Preallocated difference matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 */

#define mzd_sub mzd_add

/**
 * \brief Same as mzd_sub but without any checks on the input.
 *
 * \param C Preallocated difference matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 */

#define _mzd_sub _mzd_add

/**
 * \brief row3[col3:] = row1[col1:] + row2[col2:]
 * 
 * Adds row1 of SC1, starting with startblock1 to the end, to
 * row2 of SC2, starting with startblock2 to the end. This gets stored
 * in DST, in row3, starting with startblock3.
 *
 * \param DST destination matrix
 * \param row3 destination row for matrix dst
 * \param startblock3 starting block to work on in matrix dst
 * \param SC1 source matrix
 * \param row1 source row for matrix sc1
 * \param startblock1 starting block to work on in matrix sc1
 * \param SC2 source matrix
 * \param startblock2 starting block to work on in matrix sc2
 * \param row2 source row for matrix sc2
 *
 * \wordoffset
 */

void mzd_combine(mzd_t * DST, const size_t row3, const size_t startblock3,
		 const mzd_t * SC1, const size_t row1, const size_t startblock1, 
		 const mzd_t * SC2, const size_t row2, const size_t startblock2);

/**
 * Get n bits starting a position (x,y) from the matrix M.
 *
 * \param M Source matrix.
 * \param x Starting row.
 * \param y Starting column.
 * \param n Number of bits (<= RADIX);
 */ 

static inline word mzd_read_bits(const mzd_t *M, const size_t x, const size_t y, const int n) {
  word temp;

  /* there are two possible situations. Either all bits are in one
   * word or they are spread across two words. */

  if ( ((y+M->offset)%RADIX + n - 1) < RADIX ) {
    /* everything happens in one word here */
    temp =  M->rows[x][(y+M->offset) / RADIX]; /* get the value */
    temp <<= (y+M->offset)%RADIX; /* clear upper bits */
    temp >>= RADIX - n; /* clear lower bits and move to correct position.*/
    return temp;

  } else {
    /* two words are affected */
    const size_t block = (y+M->offset) / RADIX; /* correct block */
    const size_t spot  = (y+M->offset+n) % RADIX; /* correct offset */
    /* make room by shifting spot times to the right, and add stuff from the second word */
    temp = (M->rows[x][block] << spot) | ( M->rows[x][block + 1] >> (RADIX - spot) ); 
    return (temp << (RADIX-n)) >> (RADIX-n); /* clear upper bits and return */
   }
}


/**
 * \brief XOR n bits from values to M starting a position (x,y).
 *
 * \param M Source matrix.
 * \param x Starting row.
 * \param y Starting column.
 * \param n Number of bits (<= RADIX);
 * \param values Word with values;
 */

static inline void mzd_xor_bits(const mzd_t *M, const size_t x, const size_t y, const int n, word values) {
  word *temp;

  /* there are two possible situations. Either all bits are in one
   * word or they are spread across two words. */

  if ( ((y+M->offset)%RADIX + n - 1) < RADIX ) {
    /* everything happens in one word here */
    temp =  M->rows[x] + (y+M->offset) / RADIX;
    *temp ^= values<<(RADIX-((y+M->offset)%RADIX)-n);

  } else {
    /* two words are affected */
    const size_t block = (y+M->offset) / RADIX; /* correct block */
    const size_t spot  = (y+M->offset+n) % RADIX; /* correct offset */
    M->rows[x][block] ^= values >> (spot);
    M->rows[x][block + 1] ^= values<<(RADIX-spot);
  }
}

/**
 * \brief AND n bits from values to M starting a position (x,y).
 *
 * \param M Source matrix.
 * \param x Starting row.
 * \param y Starting column.
 * \param n Number of bits (<= RADIX);
 * \param values Word with values;
 */

static inline void mzd_and_bits(const mzd_t *M, const size_t x, const size_t y, const int n, word values) {
  word *temp;

  /* there are two possible situations. Either all bits are in one
   * word or they are spread across two words. */

  if ( ((y+M->offset)%RADIX + n - 1) < RADIX ) {
    /* everything happens in one word here */
    temp =  M->rows[x] + (y+M->offset) / RADIX;
    *temp &= values<<(RADIX-((y+M->offset)%RADIX)-n);

  } else {
    /* two words are affected */
    const size_t block = (y+M->offset) / RADIX; /* correct block */
    const size_t spot  = (y+M->offset+n) % RADIX; /* correct offset */
    M->rows[x][block] &= values >> (spot);
    M->rows[x][block + 1] &= values<<(RADIX-spot);
  }
}

/**
 * \brief Clear n bits in M starting a position (x,y).
 *
 * \param M Source matrix.
 * \param x Starting row.
 * \param y Starting column.
 * \param n Number of bits (<= RADIX);
 */

static inline void mzd_clear_bits(const mzd_t *M, const size_t x, const size_t y, const int n) {
  word temp;

  /* there are two possible situations. Either all bits are in one
   * word or they are spread across two words. */

  if ( ((y+M->offset)%RADIX + n - 1) < RADIX ) {
    /* everything happens in one word here */
    temp =  M->rows[x][(y+M->offset) / RADIX];
    temp <<= (y+M->offset)%RADIX; /* clear upper bits */
    temp >>= RADIX-n; /* clear lower bits and move to correct position.*/
    temp <<= RADIX-n - (y+M->offset)%RADIX;
    M->rows[x][(y+M->offset) / RADIX] ^= temp;
  } else {
    /* two words are affected */
    const size_t block = (y+M->offset) / RADIX; /* correct block */
    const size_t spot  = (y+M->offset+n) % RADIX; /* correct offset */
    M->rows[x][block]   ^=  M->rows[x][block] & ((ONE<<(n-spot))-1);
    M->rows[x][block+1] ^= (M->rows[x][block+1]>>(RADIX-spot))<<(RADIX-spot);
  }
}

/**
 * \brief Zero test for matrix.
 *
 * \param A Input matrix.
 *
 */
int mzd_is_zero(mzd_t *A);

/**
 * \brief Clear the given row, but only begins at the column coloffset.
 *
 * \param M Matrix
 * \param row Index of row
 * \param coloffset Column offset
 */

void mzd_row_clear_offset(mzd_t *M, const size_t row, const size_t coloffset);

/**
 * \brief Find the next nonzero entry in M starting at start_row and start_col. 
 *
 * This function walks down rows in the inner loop and columns in the
 * outer loop. If a nonzero entry is found this function returns 1 and
 * zero otherwise.
 *
 * If and only if a nonzero entry is found r and c are updated.
 *
 * \param M Matrix
 * \param start_row Index of row where to start search
 * \param start_col Index of column where to start search
 * \param r Row index updated if pivot is found
 * \param c Column index updated if pivot is found
 */

int mzd_find_pivot(mzd_t *M, size_t start_row, size_t start_col, size_t *r, size_t *c);


/**
 * \brief Return the number of nonzero entries divided by nrows *
 * ncols
 *
 * If res = 0 then 100 samples per row are made, if res > 0 the
 * function takes res sized steps within each row (res = 1 uses every
 * word).
 *
 * \param A Matrix
 * \param res Resolution of sampling
 */

double mzd_density(mzd_t *A, int res);

/**
 * \brief Return the number of nonzero entries divided by nrows *
 * ncols considering only the submatrix starting at (r,c).
 *
 * If res = 0 then 100 samples per row are made, if res > 0 the
 * function takes res sized steps within each row (res = 1 uses every
 * word).
 *
 * \param A Matrix
 * \param res Resolution of sampling
 * \param r Row to start counting
 * \param c Column to start counting
 */

double _mzd_density(mzd_t *A, int res, size_t r, size_t c);


/**
 * \brief Return the first row with all zero entries.
 *
 * If no such row can be found returns nrows.
 *
 * \param A Matrix
 */

size_t mzd_first_zero_row(mzd_t *A);

#endif //PACKEDMATRIX_H
