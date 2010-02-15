#include "Solver.h"


void Solver::checkConsistency(void)
{
    assert(occur_mode == occ_Permanent);    // (we only worry about this mode for now)

    // Every constraint is present in the appropriate occurs tables:
    for (int i = 0; i < constrs.size(); i++){
        Clause c = constrs[i]; if (c.null()) continue;
        for (int j = 0; j < c.size(); j++)
            if (occur[index(c[j])].size() > 0)
                find(occur[index(c[j])], c);
    }

    // Every occur list points to clauses containing the appropriate literals:
    for (int i = 0; i < nVars()*2; i++){
        vec<Clause>& cs = occur[i];
        Lit          p  = toLit(i);
        for (int j = 0; j < cs.size(); j++)
            find(cs[j], p);
    }

    // DEBUG
    if (0){
    printf("-----------------------------------------------------------------------------\n");
    vec<bool>   freed(constrs.size(), false);
    for (int i = 0; i < constrs_free.size(); i++){
        printf("MARKED FREE: %d\n", constrs_free[i]);
        freed[constrs_free[i]] = true; }
    for (int i = 0; i < constrs.size(); i++)
        if (constrs[i].null()){
            printf("FREE: %d\n", i);
            if (!freed[i]) printf("**MISSING**\n"), exit(1);
        }
    }

    // Null-clauses should be in the free lists:
    vec<bool>   freed(constrs.size(), false);
    for (int i = 0; i < constrs_free.size(); i++)
        assert(constrs[constrs_free[i]].null()),
        freed[constrs_free[i]] = true;

    for (int i = 0; i < constrs.size(); i++)
        assert(!constrs[i].null() || freed[i]);
}
