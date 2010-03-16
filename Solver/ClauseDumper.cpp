#include "ClauseDumper.h"

string ClauseDumper::name(const lbool& p)
{
    if (p.isUndef())
        return "l_Undef";
    else {
        if (p.getBool())
            return "l_True";
        else
            return "l_False";
    }
}

void ClauseDumper::dump(Clause& c, bool newline, FILE* out)
{
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT, L_lit(c[i]));
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}

void ClauseDumper::dump(Solver& S, Clause& c, bool newline, FILE* out)
{
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT":%c", L_lit(c[i]), name(S.value(c[i])).c_str());
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}

void ClauseDumper::dump(const vec<Lit>& c, bool newline, FILE* out)
{
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT, L_lit(c[i]));
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}

void ClauseDumper::dump(Solver& S, vec<Lit>& c, bool newline, FILE* out)
{
    fprintf(out, "{");
    for (int i = 0; i < c.size(); i++)
        fprintf(out, " "L_LIT":%c", L_lit(c[i]), name(S.value(c[i])).c_str());
    
    fprintf(out, " }%s", newline ? "\n" : "");
    fflush(out);
}
