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

#include <set>
#include <map>
#include <iomanip>
#include <math.h>
using std::set;
using std::map;

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif

//#define PART_FINDING

MatrixFinder::MatrixFinder(Solver *_S) :
    unAssigned(_S->nVars() + 1)
    , S(_S)
{
    table.resize(S->nVars(), unAssigned);
    matrix_no = 0;
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
    if (S->xorclauses.size() == 0)
        return 0;
    
    for (XorClause** c = S->xorclauses.getData(), **end = c + S->xorclauses.size(); c != end; c++) {
        set<uint> tomerge;
        vector<Var> newSet;
        for (Lit *l = &(**c)[0], *end2 = l + (**c).size(); l != end2; l++) {
            if (table[l->var()] != unAssigned)
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
    vector<uint> numXorInMatrix(matrix_no, 0);
    vector<uint> sumXorSizeInMatrix(matrix_no, 0);
    vector<vector<uint> > xorSizesInMatrix(matrix_no);
    vector<vector<XorClause*> > xorsInMatrix(matrix_no);
    
    #ifdef PART_FINDING
    vector<vector<Var> > xorFingerprintInMatrix(matrix_no);
    #endif
    
    for (XorClause** c = S->xorclauses.getData(), **end = c + S->xorclauses.size(); c != end; c++) {
        XorClause& x = **c;
        const uint matrix = table[x[0].var()];
        
        //for stats
        numXorInMatrix[matrix]++;
        sumXorSizeInMatrix[matrix] += x.size();
        xorSizesInMatrix[matrix].push_back(x.size());
        xorsInMatrix[matrix].push_back(&x);
        
        #ifdef PART_FINDING
        xorFingerprintInMatrix[matrix].push_back(fingerprint(x));
        #endif //PART_FINDING
    }
    
    #ifdef PART_FINDING
    for (uint i = 0; i < matrix_no; i++)
        findParts(xorFingerprintInMatrix[i], xorsInMatrix[i]);
    #endif //PART_FINDING
    
    uint realMatrixNum = 0;
    for (uint i = 0; i < matrix_no; i++) {
        if (numXorInMatrix[i] < 3)
            continue;
        
        const uint totalSize = reverseTable[i].size()*numXorInMatrix[i];
        const double density = (double)sumXorSizeInMatrix[i]/(double)totalSize*100.0;
        double avg = (double)sumXorSizeInMatrix[i]/(double)numXorInMatrix[i];
        double variance = 0.0;
        for (uint i2 = 0; i2 < xorSizesInMatrix[i].size(); i2++)
            variance += pow((double)xorSizesInMatrix[i][i2]-avg, 2);
        variance /= xorSizesInMatrix.size();
        const double stdDeviation = sqrt(variance);
        
        if (numXorInMatrix[i] >= 20
            && numXorInMatrix[i] <= 1000
            && realMatrixNum < (1 << 12))
        {
            cout << "|  Matrix no " << std::setw(4) << realMatrixNum;
            S->gauss_matrixes.push_back(new Gaussian(*S, S->gaussconfig, realMatrixNum, xorsInMatrix[i]));
            realMatrixNum++;
            
        } else {
            cout << "|  Unused Matrix ";
        }
        cout << std::setw(5) << numXorInMatrix[i] << " x" << std::setw(5) << reverseTable[i].size();
        cout << "  density:" << std::setw(5) << std::fixed << std::setprecision(1) << density << "%";
        cout << "  xorlen avg:" << std::setw(5) << std::fixed << std::setprecision(2)  << avg;
        cout << " stdev:" << std::setw(6) << std::fixed << std::setprecision(2) << stdDeviation << "  |" << endl;
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
                (*a)->plain_print();
                (*a2)->plain_print();
                cout << "END" << endl;
            }
        }
    }
}
