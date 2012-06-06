/*
This file is part of CryptoMiniSat2.

CryptoMiniSat2 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CryptoMiniSat2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CryptoMiniSat2.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "Vec.h"
#include "SolverTypes.h"

#include <vector>

namespace CMSat {

class SharedData
{
    public:
        vec<lbool> value;
        std::vector<std::vector<Lit> > bins;
};

}

#endif //SHARED_DATA_H
