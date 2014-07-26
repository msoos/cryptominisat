#include "searcher.h"

using namespace CMS;

//seen[x.var()] = 2  -> cannot be removed
//seen[x.var()] = 2 or 4 or 8 -> DFS has been done


void Searcher::prune_removable(vector<Lit>& out_learnt)
{
    int j = 1;
    for (int i = 1, sz = out_learnt.size(); i < sz; i++) {
        if ((seen[out_learnt[i].var()] & (1|2)) == (1|2)) {
            assert((seen[out_learnt[i].var()] & (4|8)) == 0);
            out_learnt[j++] = out_learnt[i];
        }
    }
    out_learnt.resize(j);
}

void Searcher::find_removable(const vector<Lit>& out_learnt, const uint32_t abstract_level)
{
    bool found_some = false;
    trace_lits_minim.clear();
    for (int i = 1, sz = out_learnt.size(); i < sz; i++) {
        const Lit curLit = out_learnt[i];
        assert(varData[curLit.var()].level > 0);

        if ((seen[curLit.var()] & (2|4|8)) == 0) {
            found_some |= (bool)dfs_removable(curLit, abstract_level);
        }
    }

    if (found_some)
       res_removable();
}

int Searcher::quick_keeper(Lit p, uint32_t abstract_level, const bool maykeep)
{
    // See if I can kill myself right away.
    // maykeep == 1 if I am in the original conflict clause.
    if (varData[p.var()].reason.isNULL()) {
        return (maykeep ? 2 : 8);
    } else if ((abstractLevel(p.var()) & abstract_level) == 0) {
        assert(maykeep == 0);
        return 8;
    } else {
        return 0;
    }
}

int Searcher::dfs_removable(Lit p, uint32_t abstract_level)
{
    int pseen = seen[p.var()];
    assert((pseen & (2|4|8)) == 0);

    bool maykeep = pseen & (1);
    int pstatus = quick_keeper(p, abstract_level, maykeep);
    if (pstatus) {
        seen[p.var()] |= (char) pstatus;
        if (pseen == 0) toClear.push_back(p);
        return 0;
    }

    int found_some = 0;
    pstatus = 4;

    // rp[0] is p.  The rest of rp are predecessors of p.
    const PropBy rp = varData[p.var()].reason;
    switch (rp.getType()) {
        case tertiary_t : {
            const Lit q = rp.lit3();
            if (varData[q.var()].level > 0) {
                if ((seen[q.var()] & (2|4|8)) == 0) {
                    found_some |= dfs_removable(q, abstract_level);
                }
                int qseen = seen[q.var()];
                if (qseen & (8)) {
                    pstatus = (maykeep ? 2 : 8);
                    break;
                 }
                 assert((qseen & (2|4)));
            }
             //NO BREAK
        }

        case binary_t: {
            const Lit q = rp.lit2();
            if (varData[q.var()].level > 0) {
                if ((seen[q.var()] & (2|4|8)) == 0) {
                    found_some |= dfs_removable(q, abstract_level);
                }
                int qseen = seen[q.var()];
                if (qseen & (8)) {
                    pstatus = (maykeep ? 2 : 8);
                    break;
                 }
                 assert((qseen & (2|4)));
            }
            break;
        }

        case clause_t : {
            const Clause& cl = *cl_alloc.ptr(rp.getClause());
            for (int i = 0, sz = cl.size(); i < sz; i++) {
                if (i == 0)
                    continue;

                const Lit q = cl[i];
                if (varData[q.var()].level > 0) {
                    if ((seen[q.var()] & (2|4|8)) == 0) {
                        found_some |= dfs_removable(q, abstract_level);
                    }
                    int qseen = seen[q.var()];
                    if (qseen & (8)) {
                        pstatus = (maykeep ? 2 : 8);
                        break;
                     }
                     assert((qseen & (2|4)));
                }
            }
            break;
        }

        case null_clause_t :
        default:
            assert(false);
            break;
    }


    if (pstatus == 4) {
        // We might want to resolve p out.  See res_removable().
        trace_lits_minim.push_back(p);
    }
    seen[p.var()] |= (char) pstatus;
    if (pseen == 0) toClear.push_back(p);
    found_some |= (int)maykeep;

    return found_some;
}

void Searcher::mark_needed_removable(const Lit p)
{
    const PropBy rp = varData[p.var()].reason;
    switch (rp.getType()) {
        case tertiary_t : {
            const Lit q = rp.lit3();
            if (varData[q.var()].level > 0) {
                const int qseen = seen[q.var()];
                if ((qseen & (1)) == 0 && !varData[q.var()].reason.isNULL()) {
                    seen[q.var()] |= 1;
                    if (qseen == 0) toClear.push_back(q);
                }
            }
            //NO BREAK
        }

        case binary_t : {
            const Lit q  = rp.lit2();
            if (varData[q.var()].level > 0) {
                const int qseen = seen[q.var()];
                if ((qseen & (1)) == 0 && !varData[q.var()].reason.isNULL()) {
                    seen[q.var()] |= 1;
                    if (qseen == 0) toClear.push_back(q);
                }
            }
            break;
        }

        case clause_t : {
            const Clause& cl = *cl_alloc.ptr(rp.getClause());
            for (int i = 0; i < cl.size(); i++){
                if (i == 0)
                    continue;

                const Lit q  = cl[i];
                if (varData[q.var()].level > 0) {
                    const int qseen = seen[q.var()];
                    if ((qseen & (1)) == 0 && !varData[q.var()].reason.isNULL()) {
                        seen[q.var()] |= 1;
                        if (qseen == 0) toClear.push_back(q);
                    }
                }
            }
            break;
        }

        case null_clause_t :
        default:
            assert(false);
            break;
    }

    return;
}


int  Searcher::res_removable()
{
    int minim_res_ctr = 0;
    while (trace_lits_minim.size() > 0){
        const Lit p = trace_lits_minim.back();
        trace_lits_minim.pop_back();
        assert(!varData[p.var()].reason.isNULL());

        int pseen = seen[p.var()];
        if (pseen & (1)) {
            minim_res_ctr ++;
            trace_reasons.push_back(varData[p.var()].reason);
            mark_needed_removable(p);
        }
    }
    return minim_res_ctr;
}