/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

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

#ifndef GAUSSIANCONFIG_H
#define GAUSSIANCONFIG_H

#include "cmsat/PackedRow.h"
#include "cmsat/constants.h"

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
