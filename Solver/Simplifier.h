/**************************************************************************************************

Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004

A simple Chaff-like SAT-solver with support for incremental SAT.

**************************************************************************************************/

#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "Solver.h"
#include "Queue.h"
#include "TmpFiles.h"

// For derivation output (verbosity level 2)
#define L_IND    "%-*d"
#define L_ind    decisionLevel()*3+3,decisionLevel()
#define L_LIT    "%sx%d"
#define L_lit(p) p.sign()?"~":"", p.var()

inline string name(const lbool& p) {
    if (p.isUndef())
        return "l_Undef";
    else {
        if (p.getBool())
            return "l_True";
        else
            return "l_False";
    }
}

template <class T>
uint32_t calcAbstraction(T& ps) {
    uint32_t abstraction = 0;
    for (uint32_t i = 0; i != ps.size(); i++)
        abstraction |= 1 << (ps[i].var() & 31);
    return abstraction;
}

class ClauseSimp
{
    public:
        ClauseSimp(Clause* c, const uint _index) :
            clause(c)
            , index(_index)
        {
            abst = calcAbstraction(*c);
        }
    
        Clause* clause;
        uint64_t abst;
        uint64_t index;
};

enum OccurMode { occ_Off, occ_Permanent, occ_All };

class Simplifier
{
public:
    
    Simplifier(Solver& S2);
    
    //Main
    vec<ClauseSimp>     clauses;
    vec<char>           touched;        // Is set to true when a variable is part of a removed clause. Also true initially (upon variable creation).
    vec<Var>            touched_list;   // A list of the true elements in 'touched'.
    vec<ClauseSimp>     cl_touched;     // Clauses strengthened.
    vec<ClauseSimp>     cl_added;       // Clauses created.
    vec<char>           var_elimed;     // 'eliminated[var]' is TRUE if variable has been eliminated.
        
    //Other
    vec<vec<ClauseSimp> >  occur;       // 'occur[index(lit)]' is a list of constraints containing 'lit'.
    OccurMode              occur_mode;  // What clauses to keep in the occur lists.
    
    //IO
    FILE*               elim_out;       // File storing eliminated clauses (needed to calculate model).
    char*               elim_out_file;  // (name of file)
    vec<vec<ClauseSimp>* > iter_vecs;   // Vectors currently used for iterations. Removed clauses will be looked up and replaced by 'Clause_NULL'.
    vec<vec<ClauseSimp>* > iter_sets;   // Sets currently used for iterations.
    
    // Temporaries (to reduce allocation overhead):
    //
    vec<char>           seen_tmp;       // (used in various places)
    vec<Lit>            io_tmp;         // (used for reading/writing clauses from/to disk)
    
    // The Solver
    //
    Solver& solver;
    
    // Database management:
    //
    void createTmpFiles(const char* filename) {
        if (filename == NULL)
            elim_out = createTmpFile("/tmp/tmp_elims__", "w+b", elim_out_file);
        else
            elim_out = fopen(filename, "w+b"),
            elim_out_file = NULL;
    }
    void deleteTmpFiles(void) { if (elim_out_file != NULL) deleteTmpFile(elim_out_file, true); }
    void registerIteration  (vec<ClauseSimp>& iter_set) { iter_sets.push(&iter_set); }
    void unregisterIteration(vec<ClauseSimp>& iter_set) { remove(iter_sets, &iter_set); }
    void setOccurMode(OccurMode occur_mode);
    void setupWatches(void);
    
    // Subsumption:
    //
    void unlinkClause(Clause& c, const bool reallyRemove = true, Var elim = var_Undef);
    void touch(Var x);
    void touch(Lit p);
    bool updateOccur(Clause& c);
    int  literalCount(void);        // (just progress measure)
    void findSubsumed(ClauseSimp& ps, vec<ClauseSimp>& out_subsumed);
    void findSubsumed(vec<Lit>& ps, vec<ClauseSimp>& out_subsumed);
    bool isSubsumed(Clause& ps);
    bool hasClause(Clause& ps);
    void subsume0(ClauseSimp& ps, int& counter = *(int*)NULL);
    void subsume1(ClauseSimp& ps, int& counter = *(int*)NULL);
    void simplifyBySubsumption(bool with_var_elim = true);
    void smaller_database(int& clauses_subsumed, int& literals_removed);
    void almost_all_database(int& clauses_subsumed, int& literals_removed);
    void orderVarsForElim(vec<Var>& order);
    int  substitute(Lit x, Clause& def, vec<Clause*>& poss, vec<Clause*>& negs, vec<Clause*>& new_clauses);
    Lit  findUnitDef(Var x, vec<Clause*>& poss, vec<Clause>& negs);
    bool findDef(Lit x, vec<Clause*>& poss, vec<Clause>& negs, Clause& out_def);
    bool maybeEliminate(Var x);
    void checkConsistency(void);
    void clauseReduction(void);
    void asymmetricBranching(Lit p);
    void exclude(vec<ClauseSimp>& cs, Clause* c);
    void update(vec<ClauseSimp>& cs, Clause* c, const uint64_t abst);
    bool subset(uint64_t A, uint64_t B);
    template<class T1, class T2>
    bool subset(T1& A, T2& B, vec<char>& seen);
};

template <class T, class T2>
void maybeRemove(vec<T>& ws, const T2& elem) {
    if (ws.size() > 0)
        removeW(ws, elem);
}

inline void Simplifier::touch(Var x) { 
    if (!touched[x]) {
        touched[x] = 1;
        touched_list.push(x);
    }
}
inline void Simplifier::touch(Lit p) {
    touch(p.var());
}

inline bool Simplifier::updateOccur(Clause& c) {
    /*return occur_mode == occ_All || (occur_mode == occ_Permanent && !c.learnt());*/
    return true;
}


inline bool Simplifier::subset(uint64_t A, uint64_t B)
{
    return (A & ~B) == 0;
}

// Assumes 'seen' is cleared (will leave it cleared)
template<class T1, class T2>
bool Simplifier::subset(T1& A, T2& B, vec<char>& seen)
{
    for (int i = 0; i < B.size(); i++)
        seen[B[i].toInt()] = 1;
    for (int i = 0; i < A.size(); i++){
        if (!seen[A[i].toInt()]){
            for (int i = 0; i < B.size(); i++) 
                seen[B[i].toInt()] = 0;
            return false;
        }
    }
    for (int i = 0; i < B.size(); i++) 
        seen[B[i].toInt()] = 0;
    return true;
}



inline void dump(Clause& c, bool newline = true, FILE* out = stdout) {
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT, L_lit(c[i]));
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}

inline void dump(Solver& S, Clause& c, bool newline = true, FILE* out = stdout) {
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT":%c", L_lit(c[i]), name(S.value(c[i])).c_str());
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}

inline void dump(const vec<Lit>& c, bool newline = true, FILE* out = stdout) {
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT, L_lit(c[i]));
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}

inline void dump(Solver& S, vec<Lit>& c, bool newline = true, FILE* out = stdout) {
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT":%c", L_lit(c[i]), name(S.value(c[i])).c_str());
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}


#endif //SIMPLIFIER_H