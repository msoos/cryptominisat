#ifndef VARREPLACER_H
#define VARREPLACER_H

#include "SolverTypes.h"
#include "Clause.h"
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
        void replace(vec<Lit>& ps, const bool xor_clause_inverted, const uint group);
        void extendModel() const;
        void performReplace();
        const uint getNumReplacedLits() const;
        const uint getNumReplacedVars() const;
        const vector<Var> getReplacingVars() const;
        const vector<Lit>& getReplaceTable() const;
        void newClause();
        void newVar();
    
    private:
        void replace_set(vec<Clause*>& set);
        void replace_set(vec<XorClause*>& cs, const bool need_reattach);
        bool handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2);
        void addBinaryXorClause(vec<Lit>& ps, const bool xor_clause_inverted, const uint group, const bool internal = false);
        
        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        
        vector<Lit> table;
        map<Var, vector<Var> > reverseTable;
        vec<Clause*> toRemove;
        
        uint replacedLits;
        uint replacedVars;
        bool addedNewClause;
        Solver* S;
};

#endif //VARREPLACER_H
