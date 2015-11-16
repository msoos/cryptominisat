/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef GAUSSIANCONFIG_H
#define GAUSSIANCONFIG_H

#include "constants.h"

namespace CMSat
{

class GaussConf
{
    public:

    GaussConf() :
        only_nth_gauss_save(2)
        , decision_until(10000)
        , dontDisable(true)
        , orderCols(true)
        , iterativeReduce(true)
        , max_matrix_rows(1000)
        , min_matrix_rows(20)
        , max_num_matrixes(3)
    {
    }

    //tuneable gauss parameters
    uint32_t only_nth_gauss_save;  //save only every n-th gauss matrix
    uint32_t decision_until; //do Gauss until this level
    bool dontDisable; //If activated, gauss elimination is never disabled
    bool orderCols; //Order columns according to activity
    bool iterativeReduce; //Don't minimise matrix work
    uint32_t max_matrix_rows; //The maximum matrix size -- no. of rows
    uint32_t min_matrix_rows; //The minimum matrix size -- no. of rows
    uint32_t max_num_matrixes; //Maximum number of matrixes

    uint32_t min_gauss_xor_clauses = 3;
    uint32_t max_gauss_xor_clauses = 50000;
    bool doMatrixFind = true;
};

}

#endif //GAUSSIANCONFIG_H
