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

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "PackedRow.h"

class GaussianConfig
{
    public:
    
    GaussianConfig() :
        only_nth_gauss_save(2)
        , decision_until(0)
        , dontDisable(false)
    {
    }
        
    //tuneable gauss parameters
    uint only_nth_gauss_save;  //save only every n-th gauss matrix
    uint decision_until; //do Gauss until this level
    bool dontDisable; //If activated, gauss elimination is never disabled
};

#endif //GAUSSIANCONFIG_H
