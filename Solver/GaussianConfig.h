/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************************/

#ifndef GAUSSIANCONFIG_H
#define GAUSSIANCONFIG_H

#include <sys/types.h>
#include "PackedRow.h"

class GaussianConfig
{
    public:
    
    GaussianConfig() :
        only_nth_gauss_save(2)
        , decision_until(0)
        , starts_from(2)
    {
    }
        
    //tuneable gauss parameters
    uint only_nth_gauss_save;  //save only every n-th gauss matrix
    uint decision_until; //do Gauss until this level
    uint starts_from; //Gauss elimination starts from this restart number
};

#endif //GAUSSIANCONFIG_H
