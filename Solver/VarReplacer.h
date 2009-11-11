#ifndef __REPLACE_H__
#define __REPLACE_H__

#include "SolverTypes.h"
#include "clause.h"
#include "Vec.h"

#include <sys/types.h>
#include <map>
using std::map;

class Solver;

class VarReplacer
{
    public:
        VarReplacer(Solver* S);
        void replace(const Var var, Lit lit);
        void extendModel() const;
        void performReplace();
        uint getNumReplaced() const;
    
    private:
        void replace_set(vec<Clause*>& set);
        void replace_set(vec<XorClause*>& cs, const bool need_reattach);
        
        map<Var, Lit> table;
        uint replaced;
        Solver* S;
};

#endif
