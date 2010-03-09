/**************************************************************************************************
From: Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004
**************************************************************************************************/

#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "Solver.h"
#include "Queue.h"
#include "TmpFiles.h"
#include "CSet.h"

enum OccurMode { occ_Off, occ_Permanent, occ_All };

class Subsumer
{
public:
    
    Subsumer(Solver& S2);
    const bool simplifyBySubsumption();
    void unlinkModifiedClause(vec<Lit>& cl, ClauseSimp c);
    void unlinkClause(ClauseSimp cc, Var elim = var_Undef);
    void updateClause(Clause& cl, ClauseSimp& c);
    void newVar();
    
private:
    
    //Main
    vec<ClauseSimp>        clauses;
    vec<char>              touched;        // Is set to true when a variable is part of a removed clause. Also true initially (upon variable creation).
    vec<Var>               touched_list;   // A list of the true elements in 'touched'.
    CSet                   cl_touched;     // Clauses strengthened.
    CSet                   cl_added;       // Clauses created.
    vec<vec<ClauseSimp> >  occur;          // 'occur[index(lit)]' is a list of constraints containing 'lit'.
    OccurMode              occur_mode;     // What clauses to keep in the occur lists.
    vec<vec<ClauseSimp>* > iter_vecs;      // Vectors currently used for iterations. Removed clauses will be looked up and replaced by 'Clause_NULL'.
    vec<CSet* >            iter_sets;      // Sets currently used for iterations.
    Solver&                solver;         // The Solver
    
    
    FILE*               elim_out;       // File storing eliminated clauses (needed to calculate model).
    char*               elim_out_file;  // (name of file)
    vec<char>           var_elimed;     // 'eliminated[var]' is TRUE if variable has been eliminated.
    
    // Temporaries (to reduce allocation overhead):
    //
    vec<bool>           seen_tmp;       // (used in various places)
    vec<Lit>            io_tmp;         // (used for reading/writing clauses from/to disk)
    
    // Database management:
    //
    void createTmpFiles(const char* filename);
    void deleteTmpFiles(void);
    
    //Start-up
    void addFromSolver(vec<Clause*>& cs);
    
    //Iterations
    void registerIteration  (CSet& iter_set) { iter_sets.push(&iter_set); }
    void unregisterIteration(CSet& iter_set) { remove(iter_sets, &iter_set); }
    void registerIteration  (vec<ClauseSimp>& iter_vec) { iter_vecs.push(&iter_vec); }
    void unregisterIteration(vec<ClauseSimp>& iter_vec) { remove(iter_vecs, &iter_vec); }
    
    // Subsumption:
    void touch(const Var x);
    void touch(const Lit p);
    bool updateOccur(Clause& c);
    void findSubsumed(ClauseSimp& ps, vec<ClauseSimp>& out_subsumed);
    void findSubsumed(vec<Lit>& ps, vec<ClauseSimp>& out_subsumed);
    bool isSubsumed(Clause& ps);
    void subsume0(ClauseSimp& ps);
    void subsume1(ClauseSimp& ps);
    void smaller_database();
    void almost_all_database();
    template<class T1, class T2>
    bool subset(const T1& A, const T2& B, vec<bool>& seen);
    bool subsetAbst(uint64_t A, uint64_t B);
    
    void orderVarsForElim(vec<Var>& order);
    int  substitute(Lit x, Clause& def, vec<Clause*>& poss, vec<Clause*>& negs, vec<Clause*>& new_clauses);
    Lit  findUnitDef(Var x, vec<Clause*>& poss, vec<Clause>& negs);
    bool findDef(Lit x, vec<Clause*>& poss, vec<Clause>& negs, Clause& out_def);
    bool maybeEliminate(Var x);
    void asymmetricBranching(Lit p);
    void exclude(vec<ClauseSimp>& cs, Clause* c);
    void MigrateToPsNs(vec<ClauseSimp>& poss, vec<ClauseSimp>& negs, vec<Lit>& ps, vec<Lit>& ns, const Var x);
    void DeallocPsNs(vec<Lit>& ps, vec<Lit>& ns);
    bool merge(Clause& ps, Clause& qs, Lit without_p, Lit without_q, vec<char>& seen, vec<Lit>& out_clause);
    
    uint32_t clauses_subsumed;
    uint32_t literals_removed;
    uint32_t origNClauses;
    uint32_t numCalls;
    bool fullSubsume;
};

template <class T, class T2>
void maybeRemove(vec<T>& ws, const T2& elem)
{
    if (ws.size() > 0)
        removeW(ws, elem);
}

inline void Subsumer::touch(const Var x)
{
    if (!touched[x]) {
        touched[x] = 1;
        touched_list.push(x);
    }
}
inline void Subsumer::touch(const Lit p)
{
    touch(p.var());
}

inline bool Subsumer::updateOccur(Clause& c)
{
    return occur_mode == occ_All || (occur_mode == occ_Permanent && !c.learnt()) || c.size() == 2;
}

inline void Subsumer::createTmpFiles(const char* filename)
{
    if (filename == NULL)
        elim_out = createTmpFile("/tmp/tmp_elims__", "w+b", elim_out_file);
    else
        elim_out = fopen(filename, "w+b"), elim_out_file = NULL;
}

inline void Subsumer::deleteTmpFiles(void)
{
    if (elim_out_file != NULL) deleteTmpFile(elim_out_file, true);
}


inline bool Subsumer::subsetAbst(uint64_t A, uint64_t B)
{
    return !(A & ~B);
}

// Assumes 'seen' is cleared (will leave it cleared)
template<class T1, class T2>
bool Subsumer::subset(const T1& A, const T2& B, vec<bool>& seen)
{
    for (uint i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 1;
    for (uint i = 0; i != A.size(); i++) {
        if (!seen[A[i].toInt()]) {
            for (uint i = 0; i != B.size(); i++)
                seen[B[i].toInt()] = 0;
            return false;
        }
    }
    for (uint i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 0;
    return true;
}

inline void Subsumer::newVar()
{
    occur       .push();
    occur       .push();
    seen_tmp    .push(0);       // (one for each polarity)
    seen_tmp    .push(0);
    touched     .push(1);
    var_elimed  .push(0);
}

#endif //SIMPLIFIER_H
