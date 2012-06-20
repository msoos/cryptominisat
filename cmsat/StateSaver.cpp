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

#include "cmsat/StateSaver.h"

using namespace CMSat;

StateSaver::StateSaver(Solver& _solver) :
    solver(_solver)
    , backup_order_heap(Solver::VarOrderLt(solver.activity))
{
    //Saving Solver state
    backup_var_inc = solver.var_inc;
    backup_activity.growTo(solver.activity.size());
    std::copy(solver.activity.getData(), solver.activity.getDataEnd(), backup_activity.getData());
    backup_order_heap = solver.order_heap;
    backup_polarities = solver.polarity;
    backup_restartType = solver.restartType;
    backup_propagations = solver.propagations;
    backup_random_var_freq = solver.conf.random_var_freq;
}

void StateSaver::restore()
{
    //Restore Solver state
    solver.var_inc = backup_var_inc;
    std::copy(backup_activity.getData(), backup_activity.getDataEnd(), solver.activity.getData());
    solver.order_heap = backup_order_heap;
    solver.polarity = backup_polarities;
    solver.restartType = backup_restartType;
    solver.propagations = backup_propagations;
    solver.conf.random_var_freq = backup_random_var_freq;

    //Finally, clear the order_heap from variables set/non-decisionned
    solver.order_heap.filter(Solver::VarFilter(solver));
}
