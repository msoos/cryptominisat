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

enum OccurMode { occ_Off, occ_Permanent, occ_All };

class Simplifier
{
    public:
    
    Simplifier(Solver& S2);
    
    
    //Main
    vec<char>           touched;        // Is set to true when a variable is part of a removed clause. Also true initially (upon variable creation).
    vec<Var>            touched_list;   // A list of the true elements in 'touched'.
    vec<Clause*>        cl_touched;     // Clauses strengthened.
    vec<Clause*>        cl_added;       // Clauses created.
    
    vec<char>           var_elimed;     // 'eliminated[var]' is TRUE if variable has been eliminated.
        
    //Other
    vec<vec<Clause*> >  occur;          // 'occur[index(lit)]' is a list of constraints containing 'lit'.
    OccurMode           occur_mode;     // What clauses to keep in the occur lists.
    
    //IO
    FILE*               elim_out;       // File storing eliminated clauses (needed to calculate model).
    char*               elim_out_file;  // (name of file)
    vec<vec<Clause*>* > iter_vecs;      // Vectors currently used for iterations. Removed clauses will be looked up and replaced by 'Clause_NULL'.
    vec<vec<Clause*>* > iter_sets;      // Sets currently used for iterations.
    
    
    Solver& solver;
    
    // Other database management:
    //
    void    createTmpFiles(const char* filename) {
        if (filename == NULL)
            elim_out = createTmpFile("/tmp/tmp_elims__", "w+b", elim_out_file);
        else
            elim_out = fopen(filename, "w+b"),
            elim_out_file = NULL;
    }
    void    deleteTmpFiles(void) { if (elim_out_file != NULL) deleteTmpFile(elim_out_file, true); }
    void    registerIteration  (vec<Clause*>& iter_set) { iter_sets.push(&iter_set); }
    void    unregisterIteration(vec<Clause*>& iter_set) { remove(iter_sets, &iter_set); }
    void    setOccurMode(OccurMode occur_mode);
    void    setupWatches(void);
    
    // Temporaries (to reduce allocation overhead):
    //
    vec<char>           seen_tmp;       // (used in various places)
    vec<Lit>            io_tmp;         // (used for reading/writing clauses from/to disk)
    
    // Subsumption:
    //
    void unlinkClause(Clause& c, const bool reallyRemove = true, Var elim = var_Undef);
    void touch(Var x);
    void touch(Lit p);
    bool updateOccur(Clause& c);
    int  literalCount(void);        // (just progress measure)
    void findSubsumed(Clause& ps, vec<Clause*>& out_subsumed);
    void findSubsumed(vec<Lit>& ps, vec<Clause*>& out_subsumed);
    bool isSubsumed(Clause& ps);
    bool hasClause(Clause& ps);
    void subsume0(Clause* ps, int& counter = *(int*)NULL);
    void subsume1(Clause& ps, int& counter = *(int*)NULL);
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
    
    void exclude(vec<Clause*>& cs, Clause* c);
    
};

template <class T>
void maybeRemove(vec<T>& ws, const T& elem) {
    if (ws.size() > 0)
        remove(ws, elem);
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
    return occur_mode == occ_All || (occur_mode == occ_Permanent && !c.learnt());
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