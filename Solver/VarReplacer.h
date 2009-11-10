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
        void replace(const map<Var, Lit>& toReplace);
        uint getReplaced() const;
    
    private:
        void replace_set(const map<Var, Lit>& toReplace, vec<Clause*>& set);
        void replace_set(const map<Var, Lit>& toReplace, vec<XorClause*>& cs, const bool need_reattach);
        
        uint replaced;
        Solver* S;
};

#endif
