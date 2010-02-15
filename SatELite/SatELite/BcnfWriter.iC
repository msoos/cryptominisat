//
// NOTE! Must include "SolverTypes.h" before including this file.
//

#define BCNF_CHUNK_LIMIT 1048576


//=================================================================================================
// BCNF Writer:


class BcnfWriter {
    FILE*   out;
    int     n_vars;
    int     n_clauses;
    int     chunk_sz;
    int*    chunk;

public:
    int     stated_n_vars;      // }- The "p cnf" line.
    int     stated_n_clauses;   // }

    BcnfWriter(cchar* output_file);
   ~BcnfWriter(void);

    void addClause(vec<Lit>& c);
    int  nVars   (void) { return n_vars; }
    int  nClauses(void) { return n_clauses; }
};


BcnfWriter::BcnfWriter(cchar* output_file)
    : n_vars(0), n_clauses(0), chunk_sz(1), stated_n_vars(-1), stated_n_clauses(-1)
{
    out = fopen(output_file, "w+b");
    if (out == NULL) fprintf(stderr, "ERROR! Could not open file for writing: %s\n", output_file), exit(2);   // <<= Exception handling
    fputc(0, out); fputc(0, out); fputc(0, out); fputc(0, out);     // Room for header: "BCNF"
    fputc(0, out); fputc(0, out); fputc(0, out); fputc(0, out);     // Room for byte-order: 1,2,3,4
    fputc(0, out); fputc(0, out); fputc(0, out); fputc(0, out);     // Room for #variables
    fputc(0, out); fputc(0, out); fputc(0, out); fputc(0, out);     // Room for #clauses

    chunk = xmalloc<int>(BCNF_CHUNK_LIMIT);
}


BcnfWriter::~BcnfWriter(void)
{
    chunk[0] = chunk_sz;
    chunk[chunk_sz++] = -1;
    fwrite(chunk, 4, chunk_sz, out);
    xfree(chunk);

    fflush(out);
    rewind(out);
    int byte_order = 0x01020304;
    fputc('B', out); fputc('C', out); fputc('N', out); fputc('F', out);
    fwrite(&byte_order, 1, 4, out);
    fwrite(&n_vars    , 1, 4, out);
    fwrite(&n_clauses , 1, 4, out);
    fclose(out);
}


void BcnfWriter::addClause(vec<Lit>& c)
{
    n_clauses++;

    if (chunk_sz + 3 + c.size() >= BCNF_CHUNK_LIMIT){  // leave room for final terminator size ("-1") and the chunk size itself  (not really part of the chunk, but just in case).
        chunk[0] = chunk_sz;
        chunk[chunk_sz++] = -1;
        fwrite(chunk, 4, chunk_sz, out);
        chunk_sz = 1; }

    assert(chunk_sz + 3 + c.size() < BCNF_CHUNK_LIMIT);
    chunk[chunk_sz++] = c.size();
    for (int i = 0; i < c.size(); i++)
        chunk[chunk_sz++] = index(c[i]),
        n_vars = max(n_vars, var(c[i]) + 1);
}
