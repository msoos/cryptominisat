/****************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "RestartTypeChooser.h"

#include <utility>
#include "Solver.h"
using std::pair;

//#define VERBOSE_DEBUG
//#define PRINT_VARS

RestartTypeChooser::RestartTypeChooser(const Solver& s) :
    solver(s)
    , topXPerc(0.01)
    , limitPerc(0.0018)
{
}

/**
@brief Adds info at the end of a restart to the internal datastructures

It is called a number of times after a full restart has been done, to
accumulate data. Finally, choose() is called to choose the restart type
*/
void RestartTypeChooser::addInfo()
{
    if (firstVarsOld.empty()) {
        double size = std::max(solver.order_heap.size(), 10000U);
        topX = (uint32_t)(topXPerc*size) + 1;
        limit = (uint32_t)(limitPerc*size) + 1;
        #ifdef VERBOSE_DEBUG
        std::cout << "c topX for chooser: " << topX << std::endl;
        std::cout << "c limit for chooser: " << limit << std::endl;
        #endif
    }
    assert(topX != 0);
    assert(limit != 0);

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
        std::cout << "c Same vars in first&second: " << sameIn << std::endl;
        #endif
        sameIns.push_back(sameIn);
    }

    #ifdef VERBOSE_DEBUG
    std::cout << "c Avg same vars in first&second: " << avg() << " standard Deviation:" << stdDeviation(sameIns) <<std::endl;
    #endif
}

/**
@brief After accumulation of data, this function finally decides which type to choose
*/
const RestartType RestartTypeChooser::choose()
{
    #ifdef VERBOSE_DEBUG
    std::cout << "c restart choosing time. Avg: " << avg() <<
    " , stdDeviation : " << stdDeviation(sameIns) << std::endl;
    std::cout << "c topX for chooser: " << topX << std::endl;
    std::cout << "c limit for chooser: " << limit << std::endl;
    #endif
    //pair<double, double> mypair = countVarsDegreeStDev();
    if ((avg() > (double)limit
        || ((avg() > (double)(limit*0.9) && stdDeviation(sameIns) < 5)))
        || ((double)solver.xorclauses.size() > (double)solver.nClauses()*0.1)
        || (solver.order_heap.size() < 10000)
       ) {
        #ifdef VERBOSE_DEBUG
        std::cout << "c restartTypeChooser chose STATIC restarts" << std::endl;
        #endif
        return static_restart;
    } else {
        #ifdef VERBOSE_DEBUG
        std::cout << "c restartTypeChooser chose DYNAMIC restarts" << std::endl;
        #endif
        return dynamic_restart;
    }
}

/**
@brief Calculates average for topx variable activity changes
*/
const double RestartTypeChooser::avg() const
{
    double sum = 0.0;
    for (uint32_t i = 0; i != sameIns.size(); i++)
        sum += sameIns[i];
    return (sum/(double)sameIns.size());
}

/**
@brief Calculates standard deviation for topx variable activity changes
*/
const double RestartTypeChooser::stdDeviation(vector<uint32_t>& measure) const
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

/*const std::pair<double, double> RestartTypeChooser::countVarsDegreeStDev() const
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
    for (const vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec2<Watched>& ws = *it;
        for (vec2<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                degrees[lit.var()]++;
                degrees[it2->getOtherLit().var()]++;
            }
        }
    }
}*/

