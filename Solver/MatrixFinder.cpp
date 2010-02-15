/***********************************************************************************
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
**************************************************************************************************/

#include "MatrixFinder.h"

#include "Solver.h"
#include "Gaussian.h"
#include "GaussianConfig.h"
#include "ClauseCleaner.h"

#include <set>
#include <map>
#include <iomanip>
#include <math.h>
using std::set;
using std::map;

//#define VERBOSE_DEBUG

using std::cout;
using std::endl;

//#define PART_FINDING

MatrixFinder::MatrixFinder(Solver& _solver) :
    solver(_solver)
{
}

inline const Var MatrixFinder::fingerprint(const XorClause& c) const
{
    Var fingerprint = 0;
    
    for (const Lit* a = &c[0], *end = a + c.size(); a != end; a++)
        fingerprint |= a->var();
    
    return fingerprint;
}

inline const bool MatrixFinder::firstPartOfSecond(const XorClause& c1, const XorClause& c2) const
{
    uint i1, i2;
    for (i1 = 0, i2 = 0; i1 < c1.size() && i2 < c2.size();) {
        if (c1[i1].var() != c2[i2].var())
            i2++;
        else {
            i1++;
            i2++;
        }
    }
    
    return (i1 == c1.size());
}

const uint MatrixFinder::findMatrixes()
{
    table.clear();
    table.resize(solver.nVars(), var_Undef);
    reverseTable.clear();
    matrix_no = 0;
    
    if (solver.xorclauses.size() == 0)
        return 0;
    
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    //TODO check for solver.ok == false
    
    for (XorClause** c = solver.xorclauses.getData(), **end = c + solver.xorclauses.size(); c != end; c++) {
        set<uint> tomerge;
        vector<Var> newSet;
        for (Lit *l = &(**c)[0], *end2 = l + (**c).size(); l != end2; l++) {
            if (table[l->var()] != var_Undef)
                tomerge.insert(table[l->var()]);
            else
                newSet.push_back(l->var());
        }
        if (tomerge.size() == 1) {
            const uint into = *tomerge.begin();
            map<uint, vector<Var> >::iterator intoReverse = reverseTable.find(into);
            for (uint i = 0; i < newSet.size(); i++) {
                intoReverse->second.push_back(newSet[i]);
                table[newSet[i]] = into;
            }
            continue;
        }
        
        for (set<uint>::iterator it = tomerge.begin(); it != tomerge.end(); it++) {
            newSet.insert(newSet.end(), reverseTable[*it].begin(), reverseTable[*it].end());
            reverseTable.erase(*it);
        }
        for (uint i = 0; i < newSet.size(); i++)
            table[newSet[i]] = matrix_no;
        reverseTable[matrix_no] = newSet;
        matrix_no++;
    }
    
    #ifdef VERBOSE_DEBUG
    for (map<uint, vector<Var> >::iterator it = reverseTable.begin(), end = reverseTable.end(); it != end; it++) {
        cout << "-- set begin --" << endl;
        for (vector<Var>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
            cout << *it2 << ", ";
        }
        cout << "-------" << endl;
    }
    #endif
    
    return setMatrixes();
}

const uint MatrixFinder::setMatrixes()
{
    vector<pair<uint, uint> > numXorInMatrix;
    for (uint i = 0; i < matrix_no; i++)
        numXorInMatrix.push_back(std::make_pair(i, 0));
    
    vector<uint> sumXorSizeInMatrix(matrix_no, 0);
    vector<vector<uint> > xorSizesInMatrix(matrix_no);
    vector<vector<XorClause*> > xorsInMatrix(matrix_no);
    
    #ifdef PART_FINDING
    vector<vector<Var> > xorFingerprintInMatrix(matrix_no);
    #endif
    
    for (XorClause** c = solver.xorclauses.getData(), **end = c + solver.xorclauses.size(); c != end; c++) {
        XorClause& x = **c;
        const uint matrix = table[x[0].var()];
        assert(matrix < matrix_no);
        
        //for stats
        numXorInMatrix[matrix].second++;
        sumXorSizeInMatrix[matrix] += x.size();
        xorSizesInMatrix[matrix].push_back(x.size());
        xorsInMatrix[matrix].push_back(&x);
        
        #ifdef PART_FINDING
        xorFingerprintInMatrix[matrix].push_back(fingerprint(x));
        #endif //PART_FINDING
    }
    
    std::sort(numXorInMatrix.begin(), numXorInMatrix.end(), mysorter());
    
    #ifdef PART_FINDING
    for (uint i = 0; i < matrix_no; i++)
        findParts(xorFingerprintInMatrix[i], xorsInMatrix[i]);
    #endif //PART_FINDING
    
    uint realMatrixNum = 0;
    for (int a = matrix_no-1; a != -1; a--) {
        uint i = numXorInMatrix[a].first;
        
        if (numXorInMatrix[a].second < 3)
            continue;
        
        const uint totalSize = reverseTable[i].size()*numXorInMatrix[a].second;
        const double density = (double)sumXorSizeInMatrix[i]/(double)totalSize*100.0;
        double avg = (double)sumXorSizeInMatrix[i]/(double)numXorInMatrix[a].second;
        double variance = 0.0;
        for (uint i2 = 0; i2 < xorSizesInMatrix[i].size(); i2++)
            variance += pow((double)xorSizesInMatrix[i][i2]-avg, 2);
        variance /= (double)xorSizesInMatrix.size();
        const double stdDeviation = sqrt(variance);
        
        if (numXorInMatrix[a].second >= 20
            && numXorInMatrix[a].second <= 1000
            && realMatrixNum < 3)
        {
            if (solver.verbosity >=1)
                cout << "c |  Matrix no " << std::setw(4) << realMatrixNum;
            solver.gauss_matrixes.push_back(new Gaussian(solver, solver.gaussconfig, realMatrixNum, xorsInMatrix[i]));
            realMatrixNum++;
            
        } else {
            if (solver.verbosity >=1  /*&& numXorInMatrix[a].second >= 20*/)
                cout << "c |  Unused Matrix ";
        }
        if (solver.verbosity >=1 /*&& numXorInMatrix[a].second >= 20*/) {
            cout << std::setw(5) << numXorInMatrix[a].second << " x" << std::setw(5) << reverseTable[i].size();
            cout << "  density:" << std::setw(5) << std::fixed << std::setprecision(1) << density << "%";
            cout << "  xorlen avg:" << std::setw(5) << std::fixed << std::setprecision(2)  << avg;
            cout << " stdev:" << std::setw(6) << std::fixed << std::setprecision(2) << stdDeviation << "  |" << endl;
        }
    }
    
    return realMatrixNum;
}

void MatrixFinder::findParts(vector<Var>& xorFingerprintInMatrix, vector<XorClause*>& xorsInMatrix)
{
    uint ai = 0;
    for (XorClause **a = &xorsInMatrix[0], **end = a + xorsInMatrix.size(); a != end; a++, ai++) {
        const Var fingerprint = xorFingerprintInMatrix[ai];
        uint ai2 = 0;
        for (XorClause **a2 = &xorsInMatrix[0]; a2 != end; a2++, ai2++) {
            if (ai == ai2) continue;
            const Var fingerprint2 = xorFingerprintInMatrix[ai2];
            if (((fingerprint & fingerprint2) == fingerprint) && firstPartOfSecond(**a, **a2)) {
                cout << "First part of second:" << endl;
                (*a)->plainPrint();
                (*a2)->plainPrint();
                cout << "END" << endl;
            }
        }
    }
}
