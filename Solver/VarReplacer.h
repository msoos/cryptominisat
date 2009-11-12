#ifndef __REPLACE_H__
#define __REPLACE_H__

#include "SolverTypes.h"
#include "clause.h"
#include "Vec.h"

#include <sys/types.h>
#include <map>
#include <vector>
using std::map;
using std::vector;

class Solver;

class VarReplacer
{
    public:
        VarReplacer(Solver* S);
        void replace(const Var var, Lit lit);
        void extendModel() const;
        void performReplace();
        uint getNumReplaced() const;
        void newVar();
    
    private:
        void replace_set(vec<Clause*>& set);
        void replace_set(vec<XorClause*>& cs, const bool need_reattach);
        
        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        
        vector<Lit> table;
        
        uint replaced;
        Solver* S;
};

#endif
