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

#ifndef __XORFINDER_H__
#define __XORFINDER_H__

#include "clause.h"
#include <sys/types.h>
#include <hash_map>
#include <map>

class Solver;

using __gnu_cxx::hash_map;
using std::map;
using std::pair;

class XorFinder
{
    public:
        
        XorFinder(Solver* S, vec<Clause*>& cls, vec<XorClause*>& xorcls);
        uint doByPart(uint& sumLengths, const uint minSize, const uint maxSize);
        
    private:
        
        uint findXors(uint& sumLengths);
        const vector<pair<Clause*, uint> >* getNextXor(bool& impair);
        
        struct clause_hasher {
            size_t operator()(const Clause* c) const
            {
                size_t hash = 5381;
                hash = ((hash << 5) + hash) ^ c->size();
                for (uint i = 0, size = c->size(); i < size; i++)
                    hash = ((hash << 5) + hash) ^ (*c)[i].var();
                
                return hash;
            }
        };
        
        struct clause_sorter {
            bool operator()(const pair<Clause*, uint>& c11, const pair<Clause*, uint>& c22) const
            {
                const Clause* c1 = c11.first;
                const Clause* c2 = c22.first;
                for (uint i = 0, size = c1->size(); i < size; i++) {
                    if ((*c1)[i].sign() !=  (*c2)[i].sign()) return (*c2)[i].sign();
                }
                
                return false;
            }
        };
        
        struct clause_vareq {
            uint operator()(const Clause* c1, const Clause* c2) const
            {
                if (c1->size() != c2->size())
                    return false;
                
                for (uint i = 0, size = c1->size(); i < size; i++)
                    if ((*c1)[i].var() != (*c2)[i].var())
                        return false;
                    
                return true;
            }
        };
        
        typedef hash_map<Clause*, vector<pair<Clause*, uint> >, clause_hasher, clause_vareq> ClauseTable;
        
        ClauseTable table;
        ClauseTable::iterator nextXor;
        
        map<Var, Lit> toReplace;
        
        vec<Clause*>& cls;
        vec<XorClause*>& xorcls;
        
        bool clauseEqual(const Clause& c1, const Clause& c2) const;
        bool impairSigns(const Clause& c) const;
        void countImpairs(const vector<pair<Clause*, uint> >& clauses, uint& numImpair, uint& numPair) const;
        void cleanNotRightImPair(vector<pair<Clause*, uint> >& clauses, const bool impair) const;
        bool isXor(vector<pair<Clause*, uint> >& clauses, bool& impair);
        
        Solver* S;
};

#endif //__XORFINDER_H__
