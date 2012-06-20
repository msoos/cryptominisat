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

#ifndef RESTARTTYPECHOOSER_H
#define RESTARTTYPECHOOSER_H

#include "cmsat/Solver.h"
#include <vector>
#include "cmsat/constants.h"
#include "cmsat/SolverTypes.h"

namespace CMSat {

using std::vector;

class Solver;

/**
@brief Chooses between MiniSat and GLUCOSE restart types&learnt clause evaluation

MiniSat style restart is geometric, and glucose-type is dynamic. MiniSat-type
learnt clause staistic is activity-based, glucose-type is glue-based. This
class takes as input a number of MiniSat restart's end results, computes some
statistics on them, and at the end, tells if we should use one type or the
other. Basically, it masures variable activity stability, number of xors
in the problem, and variable degrees.
*/
class RestartTypeChooser
{
    public:
        RestartTypeChooser(const Solver& s);
        void addInfo();
        RestartType choose();
        void reset();

    private:
        void calcHeap();
        double avg() const;
        std::pair<double, double> countVarsDegreeStDev() const;
        double stdDeviation(vector<uint32_t>& measure) const;

        template<class T>
        void addDegrees(const vec<T*>& cs, vector<uint32_t>& degrees) const;
        void addDegreesBin(vector<uint32_t>& degrees) const;

        const Solver& solver;
        uint32_t topX; ///<The how many is the top X? 100 is default
        uint32_t limit; ///<If top x contains on average this many common varables, we select MiniSat-type
        vector<Var> sameIns;

        vector<Var> firstVars; ///<The top x variables (in terms of var activity)
        vector<Var> firstVarsOld; ///<The previous top x variables (in terms of var activity)
};

inline void RestartTypeChooser::reset()
{
    sameIns.clear();
}

}

#endif //RESTARTTYPECHOOSER_H
