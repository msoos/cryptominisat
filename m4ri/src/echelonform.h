/**
 * \file echelonform.h
 * \brief Row echelon forms
 *
 * \author Martin Albrecht <M.R.Albrecht@rhul.ac.uk>
 */

#ifndef ECHELONFORM_H
#define ECHELONFORM_H

/*******************************************************************
*
*                M4RI: Linear Algebra over GF(2)
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

#include "packedmatrix.h"

/**
 * Density at which we switch to PLS decomposition.
 */

#define ECHELONFORM_CROSSOVER_DENSITY 0.15

/**
 * \brief (Reduced) row echelon form.
 *
 * This function will 
 *
 * \param A Matrix.
 * \param full Return the reduced row echelon form, not only upper triangular form.
 *
 * \wordoffset
 *
 * \return Rank of A.
 */

size_t mzd_echelonize(mzd_t *A, int full);

/**
 * \brief (Reduced) row echelon form using PLUQ factorisation.
 *
 * \param A Matrix.
 * \param full Return the reduced row echelon form, not only upper triangular form.
 *
 * \wordoffset
 *
 * \sa mzd_pluq()
 *
 * \return Rank of A.
 */

size_t mzd_echelonize_pluq(mzd_t *A, int full);

/**
 * \brief Matrix elimination using the 'Method of the Four Russians' (M4RI).
 *
 * This is a wrapper function for _mzd_echelonize_m4ri()
 * 
 * \param A Matrix to be reduced.
 * \param full Return the reduced row echelon form, not only upper triangular form.
 * \param k M4RI parameter, may be 0 for auto-choose.
 *
 * \wordoffset
 *
 * \sa _mzd_echelonize_m4ri()
 * 
 * \return Rank of A.
 */

size_t mzd_echelonize_m4ri(mzd_t *A, int full, int k);

#endif //ECHELONFORM_H
