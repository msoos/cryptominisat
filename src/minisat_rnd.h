/*******************************************************************************************[Rnd.h]
Copyright (c) 2012, Niklas Sorensson
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef __MINISAT_RND__
#define __MINISAT_RND__

#include <limits>
#include <cstdint>

namespace CMSat {

class MiniSatRnd
{
public:
    MiniSatRnd () {}
    MiniSatRnd (const uint32_t _inter_seed) :
        inter_seed(_inter_seed)
    {
        if (inter_seed == 0) {
            inter_seed = 91648253;
        }
    }

    // Generate a random double
    inline double randDblExc()
    {
        inter_seed *= 1389796;
        int q = (int)(inter_seed / 2147483647);
        inter_seed -= (double)q * 2147483647;
        return inter_seed / 2147483647;
    }

    // Generate a random integer:
    inline uint32_t randInt(const uint32_t size = std::numeric_limits<uint32_t>::max())
    {
        return (uint32_t)(randDblExc() * size);
    }

    inline void seed(const uint32_t _inter_seed)
    {
        inter_seed = _inter_seed;
        if (inter_seed == 0) {
            inter_seed = 91648253;
        }
    }

private:
    double inter_seed = 91648253;
};

}

#endif //__MINISAT_RND__
