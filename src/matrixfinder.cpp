/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#include "src/matrixfinder.h"
#include "src/solver.h"
#include "src/gaussian.h"
#include "src/clausecleaner.h"
#include "src/time_mem.h"

#include <set>
#include <map>
#include <iomanip>
#include <cmath>

//#define VERBOSE_DEBUG
//#define PART_FINDING

using namespace CMSat;

using std::set;
using std::map;

MatrixFinder::MatrixFinder(Solver* _solver) :
    solver(_solver)
{
}

inline uint32_t MatrixFinder::fingerprint(const Xor& x) const
{
    uint32_t fingerprint = 0;

    for (uint32_t v: x)
        fingerprint |= v;

    return fingerprint;
}

inline bool MatrixFinder::firstPartOfSecond(const Xor& c1, const Xor& c2) const
{
    uint32_t i1, i2;
    for (i1 = 0, i2 = 0; i1 < c1.size() && i2 < c2.size();) {
        if (c1[i1] != c2[i2])
            i2++;
        else {
            i1++;
            i2++;
        }
    }

    return (i1 == c1.size());
}

bool MatrixFinder::findMatrixes()
{
    assert(solver->decisionLevel() == 0);
    assert(solver->ok);

    table.clear();
    table.resize(solver->nVars(), var_Undef);
    reverseTable.clear();
    matrix_no = 0;
    double myTime = cpuTime();

    if (solver->xorclauses.size() < solver->conf.gaussconf.min_gauss_xor_clauses
        || solver->conf.gaussconf.decision_until <= 0
        || solver->xorclauses.size() > solver->conf.gaussconf.max_gauss_xor_clauses
    ) {
        return true;
    }

    bool ret = solver->clauseCleaner->clean_xor_clauses(solver->xorclauses);
    if (!ret)
        return false;

    if (!solver->conf.gaussconf.doMatrixFind) {
        if (solver->conf.verbosity >=1) {
            cout << "c Matrix finding disabled through switch. Putting all xors into matrix." << endl;
        }
        solver->gauss_matrixes.push_back(new Gaussian(solver, solver->xorclauses, 0));
        return true;
    }

    vector<uint32_t> newSet;
    set<uint32_t> tomerge;
    for (const Xor& x : solver->xorclauses) {
        tomerge.clear();
        newSet.clear();
        for (uint32_t v : x) {
            if (table[v] != var_Undef)
                tomerge.insert(table[v]);
            else
                newSet.push_back(v);
        }
        if (tomerge.size() == 1) {
            const uint32_t into = *tomerge.begin();
            map<uint32_t, vector<uint32_t> >::iterator intoReverse = reverseTable.find(into);
            for (uint32_t i = 0; i < newSet.size(); i++) {
                intoReverse->second.push_back(newSet[i]);
                table[newSet[i]] = into;
            }
            continue;
        }

        for (uint32_t v: tomerge) {
            newSet.insert(newSet.end(), reverseTable[v].begin(), reverseTable[v].end());
            reverseTable.erase(v);
        }
        for (uint32_t i = 0; i < newSet.size(); i++)
            table[newSet[i]] = matrix_no;
        reverseTable[matrix_no] = newSet;
        matrix_no++;
    }

    #ifdef VERBOSE_DEBUG
    for (map<uint32_t, vector<uint32_t> >::iterator it = reverseTable.begin()
        , end = reverseTable.end()
        ; it != end
        ; ++it
    ) {
        cout << "-- set: " << endl;
        for (vector<uint32_t>::iterator it2 = it->second.begin(), end2 = it->second.end()
            ; it2 != end2
            ; it2++
        ) {
            cout << *it2 << ", ";
        }
        cout << "-------" << endl;
    }
    #endif

    uint32_t numMatrixes = setMatrixes();

    if (solver->conf.verbosity >=1)
        cout << "c Finding matrixes :    " << cpuTime() - myTime
        << " s (found  " << numMatrixes << ")"
        << endl;

    for (Gaussian* g: solver->gauss_matrixes) {
        if (!g->init_until_fixedpoint())
            return false;
    }

    return true;
}

uint32_t MatrixFinder::setMatrixes()
{
    vector<pair<uint32_t, uint32_t> > numXorInMatrix;
    for (uint32_t i = 0; i < matrix_no; i++)
        numXorInMatrix.push_back(std::make_pair(i, 0));

    vector<uint32_t> sumXorSizeInMatrix(matrix_no, 0);
    vector<vector<uint32_t> > xorSizesInMatrix(matrix_no);
    vector<vector<Xor> > xorsInMatrix(matrix_no);

    for (const Xor& x : solver->xorclauses) {
        //take 1st variable to check which matrix it's in.
        const uint32_t matrix = table[x[0]];
        assert(matrix < matrix_no);

        //for stats
        numXorInMatrix[matrix].second++;
        sumXorSizeInMatrix[matrix] += x.size();
        xorSizesInMatrix[matrix].push_back(x.size());
        xorsInMatrix[matrix].push_back(x);
    }

    std::sort(numXorInMatrix.begin(), numXorInMatrix.end(), mysorter());

    uint32_t realMatrixNum = 0;
    for (int a = matrix_no-1; a >= 0; a--) {
        uint32_t i = numXorInMatrix[a].first;
        if (numXorInMatrix[a].second == 0) {
            continue;
        }

        if (numXorInMatrix[a].second < solver->conf.gaussconf.min_matrix_rows
            || numXorInMatrix[a].second > solver->conf.gaussconf.max_matrix_rows
        ) {
            //cout << "Too small or too large:" << numXorInMatrix[a].second << endl;
            continue;
        }

        const uint32_t totalSize = reverseTable[i].size()*numXorInMatrix[a].second;
        const double density = (double)sumXorSizeInMatrix[i]/(double)totalSize*100.0;
        double avg = (double)sumXorSizeInMatrix[i]/(double)numXorInMatrix[a].second;
        double variance = 0.0;
        for (uint32_t i2 = 0; i2 < xorSizesInMatrix[i].size(); i2++)
            variance += std::pow((double)xorSizesInMatrix[i][i2]-avg, 2);
        variance /= (double)xorSizesInMatrix.size();
        const double stdDeviation = std::sqrt(variance);

        if (realMatrixNum <= solver->conf.gaussconf.max_num_matrixes)
        {
            if (solver->conf.verbosity >=1) {
                cout << "c Matrix no " << std::setw(2) << realMatrixNum;
            }
            solver->gauss_matrixes.push_back(
                new Gaussian(solver, xorsInMatrix[i], realMatrixNum)
            );
            realMatrixNum++;
        } else {
            if (solver->conf.verbosity >=1) {
                cout << "c Unused Matrix ";
            }
        }
        if (solver->conf.verbosity >=1) {
            cout << std::setw(7) << numXorInMatrix[a].second << " x" << std::setw(5) << reverseTable[i].size();
            cout << "  density:" << std::setw(5) << std::fixed << std::setprecision(1) << density << "%";
            cout << "  xorlen avg:" << std::setw(5) << std::fixed << std::setprecision(2)  << avg;
            cout << " stdev:" << std::setw(6) << std::fixed << std::setprecision(2) << stdDeviation << endl;
        }
    }

    return realMatrixNum;
}

