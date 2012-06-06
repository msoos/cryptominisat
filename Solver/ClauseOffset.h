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

#ifndef CLAUSEOFFSET_H
#define CLAUSEOFFSET_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

namespace CMSat {

typedef uint32_t ClauseOffset;

}

#endif //CLAUSEOFFSET_H
