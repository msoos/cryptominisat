/**
 * \file solve.h
 *
 * \brief System solving with matrix routines.
 *
 * \author Jean-Guillaume Dumas <Jean-Guillaume.Dumas@imag.fr>
 *
 * \attention This file is currently broken.
 */
#ifndef SOLVE_H
#define SOLVE_H
 /*******************************************************************
 *
 *            M4RI: Linear Algebra over GF(2)
 *
 *       Copyright (C) 2008 Jean-Guillaume.Dumas@imag.fr
 *
 *  Distributed under the terms of the GNU General Public License (GPL)
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

#include <stdio.h>
#include "misc.h"
#include "permutation.h"
#include "packedmatrix.h"

/**
 * \brief Solves A X = B with A and B matrices. 
 *
 * The solution X is stored inplace on B.
 *
 * \param A Input matrix (overwritten).
 * \param B Input matrix, being overwritten by the solution matrix X
 * \param cutoff Minimal dimension for Strassen recursion (default: 0).
 * \param inconsistency_check decide wether or not to check for
 *        incosistency (faster without but output not defined if
 *        system is not consistent).
 *
 */
void mzd_solve_left(mzd_t *A, mzd_t *B, const int cutoff, 
                    const int inconsistency_check);

/**
 * \brief Solves (P L U Q) X = B
 * 
 * A is an input matrix supposed to store both:
 * \li  an upper right triangular matrix U
 * \li  a lower left unitary triangular matrix L.
 *
 * The solution X is stored inplace on B
 *
 * This version assumes that the matrices are at an even position on
 * the RADIX grid and that their dimension is a multiple of RADIX.
 *
 * \param A Input upper/lower triangular matrices.
 * \param rank is rank of A.
 * \param P Input row permutation matrix.
 * \param Q Input column permutation matrix.
 * \param B Input matrix, being overwritten by the solution matrix X.
 * \param cutoff Minimal dimension for Strassen recursion (default: 0).
 * \param inconsistency_check decide whether or not to check for
 *        incosistency (faster without but output not defined if
 *        system is not consistent).
 */
void mzd_pluq_solve_left (mzd_t *A, size_t rank, 
                          mzp_t *P, mzp_t *Q, 
                          mzd_t *B, const int cutoff, const int inconsistency_check);

/**
 * \brief  Solves (P L U Q) X = B
 *
 * A is an input matrix supposed to store both:
 * \li an upper right triangular matrix U
 * \li a lower left unitary triangular matrix L.

 * The solution X is stored inplace on B.
 *
 * This version assumes that the matrices are at an even position on
 * the RADIX grid and that their dimension is a multiple of RADIX.
 *
 * \param A Input upper/lower triangular matrices.
 * \param rank is rank of A.
 * \param P Input row permutation matrix.
 * \param Q Input column permutation matrix.
 * \param B Input matrix, being overwritten by the solution matrix X.
 * \param cutoff Minimal dimension for Strassen recursion (default: 0).
 * \param inconsistency_check decide whether or not to check for
 *        incosistency (faster without but output not defined if
 *        system is not consistent).
 *
 */
void _mzd_pluq_solve_left(mzd_t *A, size_t rank, 
                          mzp_t *P, mzp_t *Q, 
                          mzd_t *B, const int cutoff, const int inconsistency_check);

/**
 * \brief Solves A X = B with A and B matrices.
 *
 * The solution X is stored inplace on B.
 *
 * This version assumes that the matrices are at an even position on
 * the RADIX grid and that their dimension is a multiple of RADIX.
 *
 * \param A Input matrix.
 * \param B Input matrix, being overwritten by the solution matrix X.
 * \param cutoff Minimal dimension for Strassen recursion (default: 0).
 * \param inconsistency_check decide whether or not to check for
 *        incosistency (faster without but output not defined if
 *        system is not consistent).
 */
void _mzd_solve_left(mzd_t *A, mzd_t *B, const int cutoff, const int inconsistency_check);


/**
 * \brief Solve X for A X = 0.
 *
 * If r is the rank of the nr x nc matrix A, return the nc x (nc-r)
 * matrix X such that A*X == 0 and that the columns of X are linearly
 * independent.
 *
 * \param A Matrix.
 * \param cutoff Minimal dimension for Strassen recursion (default: 0).
 *
 * \wordoffset
 *
 * \sa mzd_pluq()
 *
 * \return X
 */

mzd_t *mzd_kernel_left_pluq(mzd_t *A, const int cutoff);

#endif // SOLVE_H
