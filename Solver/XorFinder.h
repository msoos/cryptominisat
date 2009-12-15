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

#ifndef XORFINDER_H
#define XORFINDER_H

#include "Clause.h"
#include <sys/types.h>
#include "VarReplacer.h"
#include "ClauseCleaner.h"

class Solver;

using std::pair;

class XorFinder
{
    public:
        
        XorFinder(Solver* S, vec<Clause*>& cls, ClauseCleaner::ClauseSetType _type);
        uint doNoPart(const uint minSize, const uint maxSize);
        
    private:
        typedef vector<pair<Clause*, uint> > ClauseTable;
        
        uint findXors(uint& sumLengths);
        bool getNextXor(ClauseTable::iterator& begin, ClauseTable::iterator& end, bool& impair);
        
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
        
        struct clause_sorter_primary {
            bool operator()(const pair<Clause*, uint>& c11, const pair<Clause*, uint>& c22)
            {
                if (c11.first->size() != c22.first->size())
                    return (c11.first->size() < c22.first->size());

                for (a = c11.first->getData(), b = c22.first->getData(), end = a + c11.first->size(); a != end; a++, b++) {
                    if (a->var() != b->var())
                        return (a->var() < b->var());
                }

                return false;
            }
            
            Lit const *a;
            Lit const *b;
            Lit const *end;
        };
        
        struct clause_sorter_secondary {
            bool operator()(const pair<Clause*, uint>& c11, const pair<Clause*, uint>& c22) const
            {
                const Clause& c1 = *(c11.first);
                const Clause& c2 = *(c22.first);

                for (uint i = 0, size = c1.size(); i < size; i++) {
                    if (c1[i].sign() !=  c2[i].sign())
                        return c2[i].sign();
                }
                
                return false;
            }
        };
         
        bool clause_vareq(const Clause* c1, const Clause* c2) const
        {
            if (c1->size() != c2->size())
                return false;

            for (uint i = 0, size = c1->size(); i < size; i++)
                if ((*c1)[i].var() != (*c2)[i].var())
                    return false;

            return true;
        }
        
        ClauseTable table;
        vector<bool> toRemove;
        void clearToRemove();
        
        vec<Clause*>& cls;
        ClauseCleaner::ClauseSetType type;
        
        bool clauseEqual(const Clause& c1, const Clause& c2) const;
        bool impairSigns(const Clause& c) const;
        void countImpairs(const ClauseTable::iterator& begin, const ClauseTable::iterator& end, uint& numImpair, uint& numPair) const;
        bool isXor(const ClauseTable::iterator& begin, const ClauseTable::iterator& end, bool& impair);
        
        Solver* S;
};

#endif //XORFINDER_H
