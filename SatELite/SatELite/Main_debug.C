#include "Main.h"
#include "Solver.h"
#include <unistd.h>


//=================================================================================================
// Solver Stub:


typedef vec<Lit> VClause;

struct VSolver {
    vec<VClause*>   clauses;
    int             n_vars;

    VSolver(void) : n_vars(0) {}
   ~VSolver(void) { for (int i = 0; i < clauses.size(); i++) delete clauses[i]; }

    VClause* addClause(VClause& c);
    int nVars(void) { return n_vars; }

};


VClause* VSolver::addClause(VClause& c)
{
    for (int i = 0; i < c.size()-1; i++)
        if (c[i] == ~c[i+1])
            return NULL;

//  VClause* tmp = new VClause(c.size());
    VClause* tmp = new VClause(xmalloc<Lit>(c.size()), c.size());   // (allocate block of right size to save memory)
    for (int i = 0; i < c.size(); i++)
        (*tmp)[i] = c[i];
    for (int i = 0; i < c.size(); i++)
        n_vars = max(n_vars, var(c[i]) + 1);
    clauses.push(tmp);
    return tmp;
}


//=================================================================================================
// DIMACS Parser:


static void skipWhitespace(char*& in) {
    while ((*in >= 9 && *in <= 13) || *in == 32)
        in++; }

static void skipLine(char*& in) {
    for (;;){
        if (*in == 0) return;
        if (*in == '\n') { in++; return; }
        in++; } }

static int parseInt(char*& in) {
    int     val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if      (*in == '-') neg = true, in++;
    else if (*in == '+') in++;
    if (*in < '0' || *in > '9') fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", *in), exit(1);
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
        in++;
    return neg ? -val : val; }

static void readClause(char*& in, VSolver& S, vec<Lit>& lits) {
    int     parsed_lit, var;
    lits.clear();
    for (;;){
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        lits.push( (parsed_lit > 0) ? Lit(var) : ~Lit(var) );
    }
}


static void parse_DIMACS_main(char* in, VSolver& S) {
    vec<Lit>        lits;
    for (;;){
        skipWhitespace(in);
        if (*in == 0)
            break;
        else if (*in == 'c' || *in == 'p')
            skipLine(in);
        else{
            readClause(in, S, lits);
            S.addClause(lits);
        }
    }
}


static void parse_DIMACS(FILE* in, VSolver& S) {
    char* text = readFile(in);
    parse_DIMACS_main(text, S);
    free(text); }


//=================================================================================================
// Verify model:


void verifyModel(cchar* name, cchar* model)
{
    // Parse CNF:
    VSolver S;
    int     len  = strlen(name);
    char*   tmp  = NULL;
    int     stat = 0;
    FILE*   in;
    if (len > 5 && strcmp(name+len-5, ".bcnf") == 0){
        reportf("(cannot verify BCNF files)\n"); return; }
    if (len > 3 && strcmp(name+len-3, ".gz") == 0){
        tmp = xstrdup("tmp_XXXXXX");
        int fd = mkstemp(tmp);
        if (fd == -1)
            fprintf(stderr, "ERROR! Could not create temporary file for unpacking problem.\n"),
            exit(1);
        else
            close(fd);
        stat = system(sFree(nsprintf("zcat %s  > %s", name, tmp)));
        in = fopen(tmp, "rb");
    }else
        in = fopen(name, "rb");
    if (stat != 0 || in == NULL)
        fprintf(stderr, "ERROR! Could not open file: %s\n", name),
        exit(1);
    parse_DIMACS(in, S);
    fclose(in);
    if (tmp != NULL)
        remove(tmp);

    // Parse model:
    vec<bool>   true_lits(S.nVars()*2, false);
    int         lit;
    in = fopen(model, "rb"); assert(in != NULL);
    for(;;){
        int n = fscanf(in, "%d", &lit);
        if (n != 1 || lit == 0) break;
        if (lit < 0)
            true_lits[index(~Lit(-lit-1))] = true;
        else
            true_lits[index( Lit( lit-1))] = true;
    }
    fclose(in);

    //for (int i = 0; i < true_lits.size(); i++)
    //    if (true_lits[i]) printf(L_LIT" ", L_lit(toLit(i)));
    //printf("\n");

    // Check satisfaction:
    for (int i = 0; i < S.clauses.size(); i++){
        VClause& c = *S.clauses[i];
        for (int j = 0; j < c.size(); j++){
            if (true_lits[index(c[j])])
                goto Satisfied;
        }

        printf("FALSE MODEL!!!\n");
        printf("{");
        for (int j = 0; j < c.size(); j++)
            printf(" x%d:%d", var(c[j]), true_lits[index(c[j])]);
        printf(" }\n");
        exit(0);

      Satisfied:;
    }
}
