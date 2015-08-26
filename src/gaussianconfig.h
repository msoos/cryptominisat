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

#include "packedrow.h"
#include "constants.h"

namespace CMSat
{

class GaussConf
{
    public:

    GaussConf() :
        only_nth_gauss_save(2)
        , decision_until(0)
        , dontDisable(false)
        , noMatrixFind(false)
        , orderCols(true)
        , iterativeReduce(true)
        , maxMatrixRows(1000)
        , minMatrixRows(20)
        , maxNumMatrixes(3)
    {
    }

    //tuneable gauss parameters
    uint32_t only_nth_gauss_save;  //save only every n-th gauss matrix
    uint32_t decision_until; //do Gauss until this level
    bool dontDisable; //If activated, gauss elimination is never disabled
    bool noMatrixFind; //Put all xor-s into one matrix, don't find matrixes
    bool orderCols; //Order columns according to activity
    bool iterativeReduce; //Don't minimise matrix work
    uint32_t maxMatrixRows; //The maximum matrix size -- no. of rows
    uint32_t minMatrixRows; //The minimum matrix size -- no. of rows
    uint32_t maxNumMatrixes; //Maximum number of matrixes
};

}

#endif //GAUSSIANCONFIG_H
