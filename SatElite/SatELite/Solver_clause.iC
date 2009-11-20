/**************************************************************************************************

Solver_clause.iC -- (C) Niklas Een, Niklas Sörensson, 2004

ADT for clauses, the only constraint supported by Satelite.

**************************************************************************************************/

#include "Solver.h"
#include "Sort.h"


//=================================================================================================
// Allocation:


#if 1
// HACKISH OPTIMIZATION OF MEMORY ALLOCATION:

#include "VecAlloc.h"
struct Size24 { char dummy[24]; };
struct Size28 { char dummy[28]; };
VecAlloc<Size24> mem24;
VecAlloc<Size28> mem28;

template <class T> macro void* ymalloc(int size);
template <> void* ymalloc<char>(int size)
{
    if      (size == 24) return (void*)mem24.alloc();
    else if (size == 28) return (void*)mem28.alloc();
    else                 return (void*)xmalloc<char>(size);
}


macro void yfree(void* ptr)
{
    Clause c((Clause_t*)ptr);
    assert(!c.dynamic());
    int size = sizeof(Clause_t) + sizeof(uint)*(c.size() + (int)c.learnt());
    if      (size == 24) mem24.free((Size24*)ptr);
    else if (size == 28) mem28.free((Size28*)ptr);
    else                 xfree(ptr);
}

#else
#define ymalloc xmalloc
#define yfree   xfree
#endif

//=================================================================================================
// Solver methods operating on clauses:


// Will allocate space in 'constrs' or 'learnts' and return the position to put new clause at.
int Solver::allocClauseId(bool learnt)
{
    int     id;
    if (learnt){
        if (learnts_free.size() > 0)
            id = learnts_free.last(),
            learnts_free.pop();
        else
            id = learnts.size(),
            learnts.push();
    }else{
        if (constrs_free.size() > 0)
            id = constrs_free.last(),
            constrs_free.pop();
        else
            id = constrs.size(),
            constrs.push();
    }
    return id;
}


void Solver::freeClauseId(int id, bool learnt)
{
    if (learnt){
        learnts[id] = Clause_NULL;
        learnts_free.push(id);
    }else{
        constrs[id] = Clause_NULL;
        constrs_free.push(id);
    }
}


Clause Solver::allocClause(const vec<Lit>& ps, bool learnt, Clause overwrite)
{
    assert(sizeof(Lit)   == sizeof(uint));
    assert(sizeof(float) == sizeof(uint));
    int       id  = overwrite.null() ? allocClauseId(learnt) : overwrite.id();
    void*     mem = overwrite.null() ? ymalloc<char>(sizeof(Clause_t) + sizeof(uint)*(ps.size() + (int)learnt)) : (void*)overwrite.ptr();
    Clause_t* c   = new (mem) Clause_t;

    c->id_         = id;
    c->abst_       = 0;
    c->size_learnt = (int)learnt | (ps.size() << 1);
    for (int i = 0; i < ps.size(); i++){
        c->data[i] = ps[i];
        c->abst_  |= abstLit(ps[i]);
    }
    if (learnt) Clause(c).activity() = 0.0;

    return Clause(c);
}


// PUBLIC: May return NULL if clause is already satisfied by the top-level assignment.
//
Clause Solver::addClause(const vec<Lit>& ps_, bool learnt, Clause overwrite)
{
    if (!ok) return Clause_NULL;

    vec<Lit>    qs;

    // Contains no eliminated variables?
    for (int i = 0; i < ps_.size(); i++)
        assert(!var_elimed[var(ps_[i])]);

    if (!learnt){
        assert(decisionLevel() == 0);
        ps_.copyTo(qs);             // Make a copy of the input vector.

        // Remove false literals:
        for (int i = 0; i < qs.size();){
            if (value(qs[i]) != l_Undef){
                if (value(qs[i]) == l_True){
                    if (!overwrite.null()) deallocClause(overwrite);
                    return Clause_NULL; }  // Clause always true -- don't add anything.
                else
                    qs[i] = qs.last(),
                    qs.pop();
            }else
                i++;
        }

        // Remove duplicates:
        sortUnique(qs);
        for (int i = 0; i < qs.size()-1; i++){
            if (qs[i] == ~qs[i+1]){
                if (!overwrite.null()) deallocClause(overwrite);
                return Clause_NULL;        // Clause always true -- don't add anything.
            }
        }
    }
    const vec<Lit>& ps = learnt ? ps_ : qs;     // 'ps' is now the (possibly) reduced vector of literals.
    //*HACK!*/if (ps.size() <= 3) learnt = false;
    if (opt_keep_all) learnt = false;

    if (ps.size() == 0){
        if (!overwrite.null()) deallocClause(overwrite);
        ok = false;
        return Clause_NULL;
    }else if (ps.size() == 1){
        if (!overwrite.null()) deallocClause(overwrite);
        if (decisionLevel() > 0) units.push(ps[0]);
        if (!enqueue(ps[0])){
            assert(decisionLevel() == 0);
            ok = false; }
        return Clause_NULL; }
    else{
        // Allocate clause:
        Clause c = allocClause(ps, learnt, overwrite);

        // Subsumption:
        if (occur_mode != occ_Off){
            if (fwd_subsump && isSubsumed(c)){
                deallocClause(c);
                return Clause_NULL; }
            if (!learnt) subsume0(c);
        }

        // Occur lists:
        if (updateOccur(c)){
            for (int i = 0; i < ps.size(); i++)
                occur[index(ps[i])].push(c),
                touch(var(ps[i]));
            if (overwrite.null())
                cl_added.add(c);
            else
                cl_touched.add(c);
        }

        // For learnt clauses only:
        if (learnt){
            // Put the second watch on the literal with highest decision level:
            int     max_i = 1;
            int     max   = level[var(ps[1])];
            for (int i = 2; i < ps.size(); i++)
                if (level[var(ps[i])] > max)
                    max   = level[var(ps[i])],
                    max_i = i;
            c[1]     = ps[max_i];
            c[max_i] = ps[1];

            // Bumping:
            claBumpActivity(c); // (newly learnt clauses should be considered active)
        }
        //*TEST*/for (int i = 0; i < c.size(); i++) varBumpActivity(c[i]);

        // Attach clause:
        if (watches_setup)
            watch(c, ~c[0]),
            watch(c, ~c[1]);

        if (learnt) learnts[c.id()] = c;
        else        constrs[c.id()] = c;

        if (!learnt){
            n_literals += c.size();
            for (int i = 0; i < c.size(); i++)
                n_occurs[index(c[i])]++;
        }

        return c;
    }
}


void Solver::deallocClause(Clause c, bool quick)    // (quick means only free memory; used in destructor of 'Solver')
{
    if (quick)
        yfree(c.ptr());
    else{
        // Free resources:
        freeClauseId(c.id(), c.learnt());
        yfree(c.ptr());
    }
}


void Solver::unlinkClause(Clause c, Var elim)
{
    if (elim != var_Undef){
        assert(!c.learnt());
        io_tmp.clear();
        io_tmp.push(toLit(c.size()));
        for (int i = 0; i < c.size(); i++)
            io_tmp.push(c[i]);
        fwrite((Lit*)io_tmp, 4, io_tmp.size(), elim_out);
    }

    if (updateOccur(c)){
        for (int i = 0; i < c.size(); i++){
            maybeRemove(occur[index(c[i])], c);
          #ifndef TOUCH_LESS
            touch(c[i]);
          #endif
        }
    }

    if (watches_setup)
        unwatch(c, ~c[0]),
        unwatch(c, ~c[1]);
    if (c.learnt()) learnts[c.id()] = Clause_NULL;
    else            constrs[c.id()] = Clause_NULL;

    if (!c.learnt()){
        n_literals -= c.size();
        for (int i = 0; i < c.size(); i++)
            n_occurs[index(c[i])]--;
    }

    // Remove from iterator vectors/sets:
    for (int i = 0; i < iter_vecs.size(); i++){
        vec<Clause>& cs = *iter_vecs[i];
        for (int j = 0; j < cs.size(); j++)
            if (cs[j] == c)
                cs[j] = NULL;
    }
    for (int i = 0; i < iter_sets.size(); i++){
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }

    // Remove clause from clause touched set:
    if (updateOccur(c))
        cl_touched.exclude(c),
        cl_added  .exclude(c);

}


// 'p' is the literal that became TRUE. Returns FALSE if conflict found. 'keep_watch' should be set
//  to FALSE before call. It will be re-set to TRUE if the watch should be kept in the current
//  watcher list.
//
bool Solver::propagateClause(Clause c, Lit p, bool& keep_watch)
{
    assert(watches_setup);

    // Make sure the false literal is c[1]:
    Lit     false_lit = ~p;
    if (c(0) == false_lit)
        c(0) = c(1), c(1) = false_lit;
    assert(c(1) == false_lit);

    // If 0th watch is true, then clause is already satisfied.
    if (value(c(0)) == l_True){
        keep_watch = true;
        return true; }

    // Look for new watch:
    for (int i = 2; i < c.size(); i++){
        if (value(c(i)) != l_False){
            c(1) = c(i), c(i) = false_lit;
            watches[index(~c(1))].push(c);
            return true; } }

    // Clause is unit under assignment:
    keep_watch = true;
    return enqueue(c(0), c);
}


// Can assume 'out_reason' to be empty.
// Calculate reason for 'p'. If 'p == lit_Undef', calculate reason for conflict.
//
void Solver::calcReason(Clause c, Lit p, vec<Lit>& out_reason)
{
    assert(p == lit_Undef || p == c[0]);
    for (int i = ((p == lit_Undef) ? 0 : 1); i < c.size(); i++)
        assert(value(c[i]) == l_False),
        out_reason.push(~c[i]);
    if (c.learnt()) claBumpActivity(c);
}


// Will remove clause and add a new shorter, potentially unit and thus adding facts to propagation queue.
//
void Solver::strengthenClause(Clause c, Lit p)
{
    assert(c.size() > 1);

    vec<Lit>    new_clause;
    for (int i = 0; i < c.size(); i++)
        if (c[i] != p && (value(c[i]) != l_False || level[var(c[i])] > 0))
            new_clause.push(c[i]);

    unlinkClause(c);    // (will touch all variables)
    addClause(new_clause, c.learnt(), c);
}


//=================================================================================================
// Optimized occur table builder:


void Solver::setOccurMode(OccurMode new_occur_mode)
{
    assert(new_occur_mode != occ_All);      // (not implemented)
    occur_mode = new_occur_mode;

    if (occur_mode == occ_Off)
        for (int i = 0; i < occur.size(); i++)
            occur[i].clear(true);
    else{
        assert(occur_mode == occ_Permanent);
        // Allocate vectors of right capacities:
        for (int i = 0; i < nVars()*2; i++){
            vec<Clause> tmp(xmalloc<Clause>(n_occurs[i]), n_occurs[i]); tmp.clear();
            tmp.moveTo(occur[i]); }
        // Fill vectors:
        for (int i = 0; i < constrs.size(); i++){
            if (constrs[i].null()) continue;
            for (int j = 0; j < constrs[i].size(); j++)
                assert(occur[index(constrs[i][j])].size() < n_occurs[index(constrs[i][j])]),
                occur[index(constrs[i][j])].push(constrs[i]);
        }
    }
}


void Solver::setupWatches(void)
{
    if (watches_setup) return;
    assert(learnts.size() == 0);

    for (int i = 0; i < constrs.size(); i++)
        if (!constrs[i].null())
            watch(constrs[i], ~constrs[i][0]),
            watch(constrs[i], ~constrs[i][1]);
    watches_setup = true;
}
