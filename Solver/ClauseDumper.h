#ifndef CLAUSEDUMPER_H
#define CLAUSEDUMPER_H

#include "Solver.h"
#include "stdio.h"
#include <string>

// For derivation output (verbosity level 2)
#define L_IND    "%-*d"
#define L_ind    decisionLevel()*3+3,decisionLevel()
#define L_LIT    "%sx%d"
#define L_lit(p) p.sign()?"~":"", p.var()

class ClauseDumper {
    public:
        static string name(const lbool& p);
        static void dump(Clause& c, bool newline = true, FILE* out = stdout);
        static void dump(Solver& S, Clause& c, bool newline = true, FILE* out = stdout);
        static void dump(const vec<Lit>& c, bool newline = true, FILE* out = stdout);
        static void dump(Solver& S, vec<Lit>& c, bool newline = true, FILE* out = stdout);
};

#endif //CLAUSEDUMPER_H