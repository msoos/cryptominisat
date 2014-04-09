/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

#ifndef __AVGCALC_H__
#define __AVGCALC_H__

#include "constants.h"
#include <limits>
#include "assert.h"
#include <vector>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace CMSat {
using std::vector;

template <class T, class T2 = uint64_t>
class AvgCalc {
    T2      sum;
    double  sumSqare;
    size_t  num;

    //min, max
    T       min;
    T       max;

public:
    AvgCalc(void) :
        sum(0)
        , sumSqare(0)
        , num(0)


        //min, max
        , min(std::numeric_limits<T>::max())
        , max(std::numeric_limits<T>::min())
    {}

    void push(const T x) {
        sum += x;
        sumSqare += (double)x*(double)x;
        num++;

        max = std::max(max, x);
        min = std::min(min, x);
    }

    T getMin() const
    {
        return min;
    }

    T getMax() const
    {
        return max;
    }

    double avg() const
    {
        if (num == 0)
            return 0;

        return (double)sum/(double)num;
    }

    double var() const
    {
        if (num == 0)
            return 0;

        const double calcAvg = avg();
        return
            (((double)sumSqare
                - 2.0*calcAvg*(double)sum
            ))/(double)num
             + calcAvg*calcAvg;
    }

    std::string avgPrint(size_t prec, size_t w) const
    {
        std::stringstream ss;
        if (num > 0) {
            ss << std::fixed << std::setprecision(prec) << std::setw(w) << std::left
            << avg();
        } else {
            ss << std::setw(5) << "?";
        }

        return ss.str();
    }

    void clear()
    {
        AvgCalc<T, T2> tmp;
        *this = tmp;
    }

    void addData(const AvgCalc& other)
    {
        sum += other.sum;
        sumSqare += other.sumSqare;
        num += other.num;

        //min, max
        min = std::min(min, other.min);
        max = std::max(max, other.max);
    }
};

} //end namespace

#endif //__AVGCALC_H__
