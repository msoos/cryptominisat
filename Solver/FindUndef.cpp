#include "FindUndef.h"

#include "Solver.h"

FindUndef::FindUndef(Solver& _S) :
    S(_S)
{
    fixNeed.clear();
    fixNeed.resize(S.nVars(), false);
    
    for (uint i = 0; i < S.trail_lim[0]; i++)
        fixNeed[S.trail[i].var()] = true;
    
    for (XorClause** it = S.xorclauses.getData(), **end = it + S.xorclauses.size(); it != end; it++) {
        XorClause& c = **it;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            fixNeed[l->var()] = true;
            assert(!S.value(*l).isUndef());
        }
    }
    
    dontLookAtClause.resize(S.clauses.size(), false);
}

const uint FindUndef::unRoll()
{
    updateFixNeed();
    
    int trail = S.decisionLevel()-1;
    uint unbounded = 0;
    
    while(trail > 0) {
        assert(trail < S.trail_lim.size());
        uint at = S.trail_lim[trail];
        
        assert(at > 0);
        Var v = S.trail[at].var();
        if (!fixNeed[v]) {
            S.assigns[v] = l_Undef;
            unbounded++;
            
            updateFixNeed();
        }
        
        trail--;
    }
    
    return unbounded;
}

void FindUndef::updateFixNeed()
{
    uint i = 0;
    for (Clause** it = S.clauses.getData(), **end = it + S.clauses.size(); it != end; it++, i++) {
        if (dontLookAtClause[i])
            continue;
        
        Clause& c = **it;
        uint numTrue = 0;
        Var v;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (S.value(*l) == l_True) {
                numTrue ++;
                v = l->var();
            }
        }
        assert(c.size() > 0);
        assert(numTrue > 0);
        
        if (numTrue == 1) {
            fixNeed[v] = true;
            dontLookAtClause[i] = true;
        }
    }
}

