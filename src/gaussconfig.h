/******************************************
Copyright (c) 2016, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

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
        , decision_until(700)
        , autodisable(true)
        , iterativeReduce(true)
        , max_matrix_rows(800)
        , min_matrix_rows(15)
        , max_num_matrixes(2)
    {
    }

    uint32_t only_nth_gauss_save;  //save only every n-th gauss matrix
    uint32_t decision_until; //do Gauss until this level
    bool autodisable; //If activated, gauss elimination is never disabled
    bool iterativeReduce; //Minimise matrix work
    uint32_t max_matrix_rows; //The maximum matrix size -- no. of rows
    uint32_t min_matrix_rows; //The minimum matrix size -- no. of rows
    uint32_t max_num_matrixes; //Maximum number of matrixes

    //Matrix extraction config
    bool doMatrixFind = true;
    uint32_t min_gauss_xor_clauses = 3;
    uint32_t max_gauss_xor_clauses = 500000;
};

}

#endif //GAUSSIANCONFIG_H
