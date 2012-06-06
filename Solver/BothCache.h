/*
This file is part of CryptoMiniSat 2.

Foobar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Foobar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BOTHCACHE_H
#define BOTHCACHE_H

#include "Solver.h"

namespace CMSat {

class BothCache
{
    public:
        BothCache(Solver& solver);
        bool tryBoth();

    private:
        Solver& solver;
};

}

#endif //BOTHCACHE_H
