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

#include "cmsat/RestartTypeChooser.h"

#include <utility>
#include "cmsat/Solver.h"

//#define VERBOSE_DEBUG
//#define PRINT_VARS

using namespace CMSat;

using std::pair;

RestartTypeChooser::RestartTypeChooser(const Solver& s) :
    solver(s)
    , topX(100)
    , limit(40)
{
}

/**
@brief Adds info at the end of a restart to the internal datastructures

It is called a number of times after a full restart has been done, to
accumulate data. Finally, choose() is called to choose the restart type
*/
void RestartTypeChooser::addInfo()
{
    firstVarsOld = firstVars;
    calcHeap();
    uint32_t sameIn = 0;
    if (!firstVarsOld.empty()) {
        uint32_t thisTopX = std::min(firstVarsOld.size(), (size_t)topX);
        for (uint32_t i = 0; i != thisTopX; i++) {
            if (std::find(firstVars.begin(), firstVars.end(), firstVarsOld[i]) != firstVars.end())
                sameIn++;
        }
        #ifdef VERBOSE_DEBUG
        std::cout << "    Same vars in first&second first 100: " << sameIn << std::endl;
        #endif
        sameIns.push_back(sameIn);
    }

    #ifdef VERBOSE_DEBUG
    std::cout << "Avg same vars in first&second first 100: " << avg() << " standard Deviation:" << stdDeviation(sameIns) <<std::endl;
    #endif
}

/**
@brief After accumulation of data, this function finally decides which type to choose
*/
RestartType RestartTypeChooser::choose()
{
    pair<double, double> mypair = countVarsDegreeStDev();
    if ((mypair.second  < 80 &&
        (avg() > (double)limit || ((avg() > (double)(limit*0.9) && stdDeviation(sameIns) < 5))))
        ||
        (mypair.second  < 80 && (double)solver.xorclauses.size() > (double)solver.nClauses()*0.1))
        return static_restart;
    else
        return dynamic_restart;
}

/**
@brief Calculates average for topx variable activity changes
*/
double RestartTypeChooser::avg() const
{
    double sum = 0.0;
    for (uint32_t i = 0; i != sameIns.size(); i++)
        sum += sameIns[i];
    return (sum/(double)sameIns.size());
}

/**
@brief Calculates standard deviation for topx variable activity changes
*/
double RestartTypeChooser::stdDeviation(vector<uint32_t>& measure) const
{
    double average = avg();
    double variance = 0.0;
    for (uint32_t i = 0; i != measure.size(); i++)
        variance += pow((double)measure[i]-average, 2);
    variance /= (double)measure.size();

    return sqrt(variance);
}

void RestartTypeChooser::calcHeap()
{
    firstVars.clear();
    firstVars.reserve(topX);
    #ifdef PRINT_VARS
    std::cout << "First vars:" << std::endl;
    #endif
    Heap<Solver::VarOrderLt> tmp(solver.order_heap);
    uint32_t thisTopX = std::min(tmp.size(), topX);
    for (uint32_t i = 0; i != thisTopX; i++) {
        #ifdef PRINT_VARS
        std::cout << tmp.removeMin()+1 << ", ";
        #endif
        firstVars.push_back(tmp.removeMin());
    }
    #ifdef PRINT_VARS
    std::cout << std::endl;
    #endif
}

std::pair<double, double> RestartTypeChooser::countVarsDegreeStDev() const
{
    vector<uint32_t> degrees;
    degrees.resize(solver.nVars(), 0);
    addDegrees(solver.clauses, degrees);
    addDegreesBin(degrees);
    addDegrees(solver.xorclauses, degrees);
    uint32_t sum = 0;
    uint32_t *i = &degrees[0], *j = i;
    for (uint32_t *end = i + degrees.size(); i != end; i++) {
        if (*i != 0) {
            sum += *i;
            *j++ = *i;
        }
    }
    degrees.resize(degrees.size() - (i-j));

    double avg = (double)sum/(double)degrees.size();
    double stdDev = stdDeviation(degrees);

    #ifdef VERBOSE_DEBUG
    std::cout << "varsDegree avg:" << avg << " stdDev:" << stdDev << std::endl;
    #endif

    return std::make_pair(avg, stdDev);
}

template<class T>
void RestartTypeChooser::addDegrees(const vec<T*>& cs, vector<uint32_t>& degrees) const
{
    for (T * const*c = cs.getData(), * const*end = c + cs.size(); c != end; c++) {
        T& cl = **c;
        if (cl.learnt()) continue;

        for (const Lit *l = cl.getData(), *end2 = l + cl.size(); l != end2; l++) {
            degrees[l->var()]++;
        }
    }
}

template void RestartTypeChooser::addDegrees(const vec<Clause*>& cs, vector<uint32_t>& degrees) const;
template void RestartTypeChooser::addDegrees(const vec<XorClause*>& cs, vector<uint32_t>& degrees) const;

void RestartTypeChooser::addDegreesBin(vector<uint32_t>& degrees) const
{
    uint32_t wsLit = 0;
    for (const vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                degrees[lit.var()]++;
                degrees[it2->getOtherLit().var()]++;
            }
        }
    }
}
