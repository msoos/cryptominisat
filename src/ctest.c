#include "cryptominisat_c.h"

c_Lit new_lit(uint32_t var, bool neg) { c_Lit x = {(var << 1) | neg}; return x; }

void assert(bool x) { /*TODO*/ }

void test(void) {
    int new; // make sure this is actually compiled as C

    SATSolver *solver = cmsat_new();
    cmsat_set_num_threads(solver, 4);
    cmsat_new_vars(solver, 3);

    c_Lit clause[4] = {};
    clause[0] = new_lit(0, false);
    cmsat_add_clause(solver, clause, 1);

    clause[0] = new_lit(1, true);
    cmsat_add_clause(solver, clause, 1);

    clause[0] = new_lit(0, true);
    clause[1] = new_lit(1, false);
    clause[2] = new_lit(2, false);
    cmsat_add_clause(solver, clause, 3);

    c_lbool ret = cmsat_solve(solver);
    assert(ret.x == L_TRUE);

    slice_lbool model = cmsat_get_model(solver);
    assert(model.vals[0].x == L_TRUE);
    assert(model.vals[1].x == L_FALSE);
    assert(model.vals[2].x == L_TRUE);

    cmsat_free(solver);
}
