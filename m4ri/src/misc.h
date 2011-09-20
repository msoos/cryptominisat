
/**
 * \file misc.h
 * \brief Helper functions.
 *
 * \author Gregory Bard <bard@fordham.edu>
 * \author Martin Albrecht <M.R.Albrecht@rhul.ac.uk>
 */

#ifndef MISC_H
#define MISC_H
/*******************************************************************
*
*                 M4RI: Linear Algebra over GF(2)
*
*    Copyright (C) 2007, 2008 Gregory Bard <bard@fordham.edu>
*    Copyright (C) 2008 Martin Albrecht <M.R.Albrecht@rhul.ac.uk>
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

#ifndef HAVE_SSE2
#undef HAVE_MM_MALLOC
#endif

#ifdef HAVE_MM_MALLOC
#include <mm_malloc.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*
 * These define entirely the word width used in the library.
 */

/**
 * A word is the typical packed data structure to represent packed
 * bits.
 */

typedef unsigned long long word;

/**
 * \brief The number of bits in a word.
 */

#define RADIX (sizeof(word)<<3)

/**
 * \brief The number one as a word.
 */

#define ONE ((word)1)


/**
 * \brief The number 2^64-1 as a word.
 */

#define FFFF ((word)0xffffffffffffffffull)

/**
 * \brief Return the maximal element of x and y
 *
 * \param x Word
 * \param y Word
 */

#ifndef MAX
#define MAX(x,y) (((x) > (y))?(x):(y))
#endif

/**
 * \brief Return the minimal element of x and y
 *
 * \param x Word
 * \param y Word
 */

#ifndef MIN
#define MIN(x,y) (((x) < (y))?(x):(y))
#endif

/**
 * \brief Return r such that x elements fit into r blocks of length y.
 *
 * \param x Number of elements
 * \param y Block size
 */

#define DIV_CEIL(x,y) (((x)%(y))?(x)/(y)+1:(x)/(y))

/**
 *\brief Pretty for 1.
 */ 

#define TRUE 1

/**
 *\brief Pretty for 0.
 */ 

#define FALSE 0

/**
 * \brief $2^i$
 *
 * \param i Integer.
 */ 

#define TWOPOW(i) (ONE<<(i))

/**
 * \brief Pretty for unsigned char.
 */

typedef unsigned char BIT;

/**
 * \brief Clear the bit spot (counting from the left) in the word w
 * 
 * \param w Word
 * \param spot Integer with 0 <= spot < RADIX
 */

#define CLR_BIT(w, spot) ((w) &= ~(ONE<<(RADIX - (spot) - 1)))

/**
 * \brief Set the bit spot (counting from the left) in the word w
 * 
 * \param w Word
 * \param spot Integer with 0 <= spot < RADIX
 */

#define SET_BIT(w, spot) ((w) |= (ONE<<(RADIX - (spot) - 1)))

/**
 * \brief Get the bit spot (counting from the left) in the word w
 * 
 * \param w Word
 * \param spot Integer with 0 <= spot < RADIX
 */

#define GET_BIT(w, spot) (((w) & (ONE<<(RADIX - (spot) - 1))) >> (RADIX - (spot) - 1))

/**
 * \brief Write the value to the bit spot in the word w
 * 
 * \param w Word.
 * \param spot Integer with 0 <= spot < RADIX.
 * \param value Either 0 or 1.
 */

#define WRITE_BIT(w, spot, value) ((w) = (((w) &~(ONE<<(RADIX - (spot) - 1))) | (((word)(value))<<(RADIX - (spot) - 1))))

/**
 * \brief Flip the spot in the word w
 *
 * \param w Word.
 * \param spot Integer with 0 <= spot < RADIX.
 */

#define FLIP_BIT(w, spot) ((w) ^= (ONE<<(RADIX - (spot) - 1)))

/**
* \brief Return the n leftmost bits of the word w.
*
* \param w Word
* \param n Integer with 0 <= spot < RADIX
*/

#define LEFTMOST_BITS(w, n)  ((w) & ~((ONE<<(RADIX-(n)))-1))>>(RADIX-(n))

/**
* \brief Return the n rightmost bits of the word w.
*
* \param w Word
* \param n Integer with 0 <= spot < RADIX
*/

#define RIGHTMOST_BITS(w, n) (((w)<<(RADIX-(n)-1))>>(RADIX-(n)-1))

/**
* \brief creat a bit mask to zero out all but the n%RADIX leftmost
* bits.
*
* \param n Integer
*/

#define LEFT_BITMASK(n) (~((ONE << ((RADIX - (n % RADIX))%RADIX) ) - 1))

/**
* \brief creat a bit mask to zero out all but the n%RADIX rightmost
* bits.
*
* \param n Integer
*
* \warning Does not handle multiples of RADIX correctly
*/

#define RIGHT_BITMASK(n) (FFFF>>( (RADIX - (n%RADIX))%RADIX ))

/**
* \brief creat a bit mask to zero out all but the n%RADIX bit.
*
* \param n Integer
*
*/

#define BITMASK(n) (ONE<<(RADIX-((n)%RADIX)-1))


/**
 * \brief Return alignment of addr w.r.t. n. For example the address
 * 17 would be 1 aligned w.r.t. 16.
 *
 * \param addr
 * \param n
 */

#define ALIGNMENT(addr, n) (((unsigned long)(addr))%(n))

/**
 * Return the index of the leftmost bit in a for a nonzero.
 *
 * \param a Word
 */

static inline int leftmost_bit(word a) {
  int r = 0;
  if(a>>32)
    r+=32, a>>=32;
  if(a>>16)
    r+=16, a>>=16;
  if(a>>8)
    r+=8, a>>=8;
  if(a>>4)
    r+=4, a>>=4;
  if(a>>2)
    r+=2, a>>=2;
  if(a>>1)
    r+=1, a>>=1;
  if(a)
    r+=1, a>>=1;
  return r;
}



/**** Error Handling *****/

/**
 * \brief Print error message and abort(). 
 * 
 * The function accepts additional
 * parameters like printf, so e.g. m4ri_die("foo %d bar %f\n",1 ,2.0)
 * is valid and will print the string "foo 1 bar 2.0" before dying.
 *
 * \param errormessage a string to be printed.
 *
 * \todo Allow user to register callback which is called on
 * m4ri_die().
 *
 * \warning The provided string is not free'd.
 */

void m4ri_die(const char *errormessage, ...);

/**** IO *****/

/**
 * \brief Write a sting representing the word data to destination. 
 * 
 * \param destination Address of buffer of length at least RADIX*1.3
 * \param data Source word
 * \param colon Insert a Colon after every 4-th bit. 
 * \warning Assumes destination has RADIX*1.3 bytes available 
 */
void m4ri_word_to_str( char *destination, word data, int colon);

/**
 * \brief Return 1 or 0 uniformly randomly distributed.
 *
 * \todo Allow user to provide her own random() function.
 */

//BIT m4ri_coin_flip(void);
static inline BIT m4ri_coin_flip() {
  if (rand() < RAND_MAX/2) {
    return 0;
  }  else {
    return 1;
  }
}

/**
 * \brief Return uniformly randomly distributed random word.
 *
 * \todo Allow user to provide her own random() function.
 */

word m4ri_random_word();

/***** Initialization *****/

/**
 * \brief Initialize global data structures for the M4RI library.
 *
 * On Linux/Solaris this is called automatically when the shared
 * library is loaded, but it doesn't harm if it is called twice.
 */

#if defined(__GNUC__)
void __attribute__ ((constructor)) m4ri_init(void);
#else
void m4ri_init(void);
#endif

#ifdef __SUNPRO_C
#pragma init(m4ri_init)
#endif

/**
 * \brief De-initialize global data structures from the M4RI library. 
 *
 * On Linux/Solaris this is called automatically when the shared
 * library is unloaded, but it doesn't harm if it is called twice.
 */

#if defined(__GNUC__)
void __attribute__ ((destructor)) m4ri_fini(void);
#else
void m4ri_fini(void);
#endif

#ifdef __SUNPRO_C
#pragma fini(m4ri_fini)
#endif

/***** Memory Management *****/

#if CPU_L2_CACHE == 0
/**
 * Fix some standard value for L2 cache size if it couldn't be
 * determined by configure.
 */
#define CPU_L2_CACHE 524288
#endif //CPU_L2_CACHE

#if CPU_L1_CACHE == 0
/**
 * Fix some standard value for L1 cache size if it couldn't be
 * determined by configure.
 */
#define CPU_L1_CACHE 16384
#endif //CPU_L1_CACHE

/**
 * \brief Calloc wrapper.
 *
 * \param count Number of elements.
 * \param size Size of each element.
 *
 * \todo Allow user to register calloc function.
 */

/* void *m4ri_mm_calloc( int count, int size ); */
static inline void *m4ri_mm_calloc( int count, int size ) {
  void *newthing;
#ifdef HAVE_OPENMP
#pragma omp critical
{
#endif

#ifdef HAVE_MM_MALLOC
  newthing = _mm_malloc(count*size, 16);
#else
  newthing = calloc(count, size);
#endif

#ifdef HAVE_OPENMP
 }
#endif

  if (newthing==NULL) {
    m4ri_die("m4ri_mm_calloc: calloc returned NULL\n");
    return NULL; /* unreachable. */
  }
#ifdef HAVE_MM_MALLOC
  char *b = (char*)newthing;
  memset(b, 0, count*size);
#endif
  return newthing;
}

/**
 * \brief Malloc wrapper.
 *
 * \param size Size in bytes.
 *
 * \todo Allow user to register malloc function.
 */

/* void *m4ri_mm_malloc( int size ); */
static inline void *m4ri_mm_malloc( int size ) {
  void *newthing;
#ifdef HAVE_OPENMP
#pragma omp critical
{
#endif

#ifdef HAVE_MM_MALLOC
  newthing = _mm_malloc(size, 16);
#else
  newthing = malloc( size );
#endif  
#ifdef HAVE_OPENMP
 }
#endif
  if (newthing==NULL && (size>0)) {
    m4ri_die("m4ri_mm_malloc: malloc returned NULL\n");
    return NULL; /* unreachable */
  }
  else return newthing;
}


/**
 * \brief Free wrapper.
 *
 * \param condemned Pointer.
 *
 * \todo Allow user to register free function.
 */

/* void m4ri_mm_free(void *condemned, ...); */
static inline void m4ri_mm_free(void *condemned, ...) { 
#ifdef HAVE_MM_MALLOC
  _mm_free(condemned); 
#else
  free(condemned);
#endif
}

/**
 * \brief Maximum number of bytes allocated in one malloc() call.
 */

#define MM_MAX_MALLOC ((1ULL)<<30)

/**
 * \brief Enable memory block cache (default: disabled)
 */
#define ENABLE_MMC


/**
 * \brief Number of blocks that are cached.
 */

#define M4RI_MMC_NBLOCKS 16

/**
 * \brief Maximal size of blocks stored in cache.
 */

#define M4RI_MMC_THRESHOLD CPU_L2_CACHE

/**
 * The mmc memory management functions check a cache for re-usable
 * unused memory before asking the system for it.
 */

typedef struct _mm_block {
  /**
   * Size in bytes of the data.
   */
  size_t size;

  /**
   * Pointer to buffer of data.
   */
  void *data;

} mmb_t;

/**
 * The actual memory block cache.
 */

extern mmb_t m4ri_mmc_cache[M4RI_MMC_NBLOCKS];

/**
 * \brief Return handle for locale memory management cache.
 *
 * \attention Not thread safe.
 */

static inline mmb_t *m4ri_mmc_handle(void) {
  return m4ri_mmc_cache;
}

/**
 * \brief Allocate size bytes.
 *
 * \param size Number of bytes.
 */

static inline void *m4ri_mmc_malloc(size_t size) {

#ifdef ENABLE_MMC
  void *ret = NULL;
#endif

#ifdef HAVE_OPENMP
#pragma omp critical
{
#endif

#ifdef ENABLE_MMC
  mmb_t *mm = m4ri_mmc_handle();
  if (size <= M4RI_MMC_THRESHOLD) {
    size_t i;
    for (i=0; i<M4RI_MMC_NBLOCKS; i++) {
      if(mm[i].size == size) {
        ret = mm[i].data;
        mm[i].data = NULL;
        mm[i].size = 0;
        break;
      }
    }
  }
#endif //ENABLE_MMC

#ifdef HAVE_OPENMP
 }
#endif

#ifdef ENABLE_MMC
 if (ret)
   return ret;
 else
   return m4ri_mm_malloc(size);
#else 
 return m4ri_mm_malloc(size);
#endif
}

/**
 * \brief Allocate size times count zeroed bytes.
 *
 * \param size Number of bytes per block.
 * \param count Number of blocks.
 *
 * \warning Not thread safe.
 */

static inline void *m4ri_mmc_calloc(size_t size, size_t count) {
  void *ret = m4ri_mmc_malloc(size*count);
  memset((char*)ret, 0, count*size);
  return ret;
}

/**
 * \brief Free the data pointed to by condemned of the given size.
 *
 * \param condemned Pointer to memory.
 * \param size Number of bytes.
 *
 * \warning Not thread safe.
 */

static inline void m4ri_mmc_free(void *condemned, size_t size) {
#ifdef HAVE_OPENMP
#pragma omp critical
{
#endif
#ifdef ENABLE_MMC  
  static size_t j = 0;
  mmb_t *mm = m4ri_mmc_handle();
  if (size < M4RI_MMC_THRESHOLD) {
    size_t i;
    for(i=0; i<M4RI_MMC_NBLOCKS; i++) {
      if(mm[i].size == 0) {
        mm[i].size = size;
        mm[i].data = condemned;
        break;
      }
    }
    if (i == M4RI_MMC_NBLOCKS) {
      m4ri_mm_free(mm[j].data);
      mm[j].size = size;
      mm[j].data = condemned;
      j = (j+1) % M4RI_MMC_NBLOCKS;
    }
  } else {
    m4ri_mm_free(condemned);
  }
#else
  m4ri_mm_free(condemned);
#endif //ENABLE_MMC
#ifdef HAVE_OPENMP
 }
#endif
}

/**
 * \brief Cleans up the cache.
 *
 * This function is called automatically when the shared library is
 * loaded.
 *
 * \warning Not thread safe.
 */

static inline void m4ri_mmc_cleanup(void) {
#ifdef HAVE_OPENMP
#pragma omp critical
{
#endif
  mmb_t *mm = m4ri_mmc_handle();
  size_t i;
  for(i=0; i < M4RI_MMC_NBLOCKS; i++) {
    if (mm[i].size)
      m4ri_mm_free(mm[i].data);
    mm[i].size = 0;
  }
#ifdef HAVE_OPENMP
 }
#endif
}

#endif //MISC_H
