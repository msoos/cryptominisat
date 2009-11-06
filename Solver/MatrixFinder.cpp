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
#include "gaussian.h"
#include "gaussianconfig.h"

#include <list>
#include <set>
using std::list;
using std::set;

MatrixFinder::MatrixFinder(Solver *_S) :
    S(_S)
{
    if (S->xorclauses.size() == 0) {
        numMatrix = 0;
        return;
    }
    
    typedef list<set<Var> > myset;
    myset sets;
    
    for (XorClause** c = S->xorclauses.getData(), **end = c + S->xorclauses.size(); c != end; c++) {
        map<uint, myset::iterator> tomerge;
        for (Lit *l = &(**c)[0], *end2 = l + (**c).size(); l != end2; l++) {
            uint i = 0;
            for (myset::iterator it = sets.begin(), end3 = sets.end(); it != end3; it++, i++) {
                set<uint>::iterator finder = it->find(l->var());
                if (finder != it->end()) {
                    tomerge[i] = it;
                    break;
                }
            }
        }
        if (tomerge.empty()) {
            set<Var> tmp;
            for (Lit *l = &(**c)[0], *end3 = l + (**c).size(); l != end3; l++)
                tmp.insert(l->var());
            sets.push_back(tmp);
        } else {
            set<Var>& goodset = *(tomerge.begin()->second);
            for (map<uint, myset::iterator>::iterator it2 = ++tomerge.begin(), end2 = tomerge.end(); it2 != end2; it2++) {
                for (set<Var>::iterator it3 = (it2->second)->begin(), end3 = (it2->second)->end(); it3 != end3; it3++) {
                    goodset.insert(*it3);
                }
                sets.erase(it2->second);
            }
            for (Lit *l = &(**c)[0], *end2 = l + (**c).size(); l != end2; l++)
                goodset.insert(l->var());
        }
    }
    
    #ifdef VERBOSE_DEBUG
    for (myset::iterator it = sets.begin(), end = sets.end(); it != end; it++) {
        cout << "-- set begin --" << endl;
        for (set<Var>::iterator it2 = it->begin(), end2 = it->end(); it2 != end2; it2++) {
            cout << *it2 << ", ";
        }
        cout << "-------" << endl;
    }
    #endif
    
    map<Var, uint> varToSet;
    uint matrix = 0;
    for (myset::iterator it = sets.begin(), end = sets.end(); it != end; it++, matrix++) {
        for (set<Var>::iterator it2 = it->begin(), end2 = it->end(); it2 != end2; it2++) {
            varToSet[*it2] = matrix;
        }
    }
    assert(matrix < (1 << 12));
    
    vector<uint> numXorInMatrix(matrix, 0);
    
    for (XorClause** c = S->xorclauses.getData(), **end = c + S->xorclauses.size(); c != end; c++) {
        XorClause& x = **c;
        numXorInMatrix[varToSet[x[0].var()]]++;
    }
    
    map<uint, uint> remapMatrixes;
    for (uint i = 0; i < matrix; i++) {
        remapMatrixes[i] = i;
    }
    
    uint newNumMatrixes = matrix;
    for (uint i = 0; i < numXorInMatrix.size(); i++) {
        if (numXorInMatrix[i] < 20) {
            remapMatrixes[i] = UINT_MAX;
            for (uint i2 = i+1; i2 < matrix; i2++) {
                remapMatrixes[i2]--;
            }
            newNumMatrixes--;
        }
    }
    
    for (XorClause** c = S->xorclauses.getData(), **end = c + S->xorclauses.size(); c != end; c++) {
        XorClause& x = **c;
        const uint toSet = remapMatrixes[varToSet[x[0].var()]];
        if (toSet != UINT_MAX)
            x.setMatrix(toSet);
        else
            x.setMatrix((1 << 12)-1);
    }
    
    for (uint i = 0; i < newNumMatrixes; i++)
        S->gauss_matrixes.push_back(new Gaussian(*S, i, S->gaussconfig));
    
    numMatrix = newNumMatrixes;
}
