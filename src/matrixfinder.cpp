/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "matrixfinder.h"
#include "constants.h"
#include "solver.h"
#include "gaussian.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "sqlstats.h"
#include "xorfinder.h"
#include "varreplacer.h"

#include <set>
#include <map>
#include <iomanip>
#include <cmath>

//#define VERBOSE_DEBUG
//#define PART_FINDING

using namespace CMSat;

using std::set;
using std::map;

MatrixFinder::MatrixFinder(Solver* _solver) :
    solver(_solver)
{
}

inline uint32_t MatrixFinder::fingerprint(const Xor& x) const
{
    uint32_t fingerprint = 0;

    for (uint32_t v: x)
        fingerprint |= v;

    return fingerprint;
}

inline bool MatrixFinder::firstPartOfSecond(const Xor& c1, const Xor& c2) const
{
    uint32_t i1, i2;
    for (i1 = 0, i2 = 0; i1 < c1.size() && i2 < c2.size();) {
        if (c1[i1] != c2[i2])
            i2++;
        else {
            i1++;
            i2++;
        }
    }

    return (i1 == c1.size());
}

inline bool MatrixFinder::belong_same_matrix(const Xor& x) {
    uint32_t comp_num = numeric_limits<uint32_t>::max();
    for (const auto& v: x.vars) {
        if (table[v] == var_Undef) return false; //Belongs to none, abort
        if (comp_num == numeric_limits<uint32_t>::max()) comp_num = table[v]; //Belongs to this one
        else {
            if (comp_num != table[v]) {
                //Another var in this XOR belongs to another component
                return false;
            }
        }
    }
    return true;
}

// Puts XORs from xorclauses into matrices. Matrices are created but not initialized
// Detaches XORs that have been put into matrices
// Returns SAT/UNSAT
bool MatrixFinder::find_matrices(bool& matrix_created)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->ok);
    assert(solver->gmatrices.empty());

    for(auto& gw: solver->gwatches) gw.clear();
    solver->remove_and_clean_all();
    matrix_created = true;

    table.clear();
    table.resize(solver->nVars(), var_Undef);
    reverseTable.clear();
    matrix_no = 0;
    double my_time = cpuTime();

    XorFinder finder(nullptr, solver);
    solver->clauseCleaner->clean_xor_clauses(solver->xorclauses, false);

    finder.grab_mem();
    set<uint32_t> clash_vars;
    if (solver->xorclauses.size() < solver->conf.gaussconf.min_gauss_xor_clauses) {
        matrix_created = false;
        verb_print(4, "[matrix] too few xor clauses for GJ: " << solver->xorclauses.size());
        solver->gqueuedata.clear();
        return solver->attach_xorclauses();
    }
    if (solver->xorclauses.size() > solver->conf.gaussconf.max_gauss_xor_clauses
        && solver->conf.sampling_vars_set
    ) {
        matrix_created = false;
        verb_print(1,
            "c WARNING sampling vars have been given but there"
            "are too many XORs and it would take too much time to put them"
            "into matrices. Skipping!");
        solver->gqueuedata.clear();
        return solver->attach_xorclauses();
    }
    if (!solver->conf.gaussconf.doMatrixFind) {
        verb_print(1,"c Matrix finding disabled through switch. Not using matrixes");
        solver->gqueuedata.clear();
        return solver->attach_xorclauses();
    }

    vector<uint32_t> newSet;
    set<uint32_t> tomerge;
    for (const Xor& x : solver->xorclauses) {
        if (belong_same_matrix(x)) continue;
        tomerge.clear();
        newSet.clear();
        for (const uint32_t& v : x) {
            if (table[v] != var_Undef) tomerge.insert(table[v]);
            else newSet.push_back(v);
        }

        // Move new elements to the one the other(s) belong to
        if (tomerge.size() == 1) {
            const uint32_t into = *tomerge.begin();
            auto intoReverse = reverseTable.find(into);
            for (const auto& elem: newSet) {
                intoReverse->second.push_back(elem);
                table[elem] = into;
            }
            continue;
        }

        //Move all to a new set
        for (const uint32_t& v: tomerge) {
            newSet.insert(newSet.end(), reverseTable[v].begin(), reverseTable[v].end());
            reverseTable.erase(v);
        }
        for (const auto& elem: newSet) table[elem] = matrix_no;
        reverseTable[matrix_no] = newSet;
        matrix_no++;
    }

    #ifdef VERBOSE_DEBUG
    for (const auto& m : reverseTable) {
        cout << "XOR table set: "; for (const auto& a: m.second) cout << a << ", "; cout << "----" << endl;
    }
    #endif

    uint32_t numMatrixes = setup_matrices_attach_remaining_cls();

    const bool time_out =  false;
    const double time_used = cpuTime() - my_time;
    verb_print(1, "[matrix] Using " << numMatrixes
        << " matrices recovered from " << solver->xorclauses.size() << " xors"
        << solver->conf.print_times(time_used, time_out));

    if (solver->sqlStats) solver->sqlStats->time_passed_min( solver , "matrix find" , time_used);
    return solver->okay();
}

uint32_t MatrixFinder::setup_matrices_attach_remaining_cls() {
    if (solver->conf.sampling_vars_set) {
        uint32_t size_at_least = (double)solver->conf.sampling_vars.size()*3;
        if (solver->conf.gaussconf.max_matrix_rows < size_at_least) {
            solver->conf.gaussconf.max_matrix_rows = size_at_least;
            verb_print(1,"c [matrix] incrementing max number of rows to " << size_at_least);
        }
    }

    vector<MatrixShape> matrix_shape;
    vector<vector<Xor> > xorsInMatrix(matrix_no);
    for (uint32_t i = 0; i < matrix_no; i++) {
        matrix_shape.push_back(MatrixShape(i));
        matrix_shape[i].num = i;
        matrix_shape[i].cols = reverseTable[i].size();
    }

    // Move xorclauses temporarily
    for (Xor& x : solver->xorclauses) {
        if (x.trivial()) {
            del_xor_reason(x);
            if (x.XID != 0) *solver->frat << delx << x << fin;
            continue;
        }
        if (solver->frat->enabled()) assert(x.XID != 0);

        //take 1st variable to check which matrix it's in.
        const uint32_t matrix = table[x[0]];
        assert(matrix < matrix_no);

        matrix_shape[matrix].rows ++;
        matrix_shape[matrix].sum_xor_sizes += x.size();
        xorsInMatrix[matrix].push_back(x);
    }
    solver->xorclauses.clear();

    for(auto& m: matrix_shape)
        if (m.tot_size() > 0) m.density = (double)m.sum_xor_sizes / (double)(m.tot_size());

    std::sort(matrix_shape.begin(), matrix_shape.end(), mysorter());

    uint32_t realMatrixNum = 0;
    uint32_t unusedMatrix = 0;
    uint32_t too_few_rows_matrix = 0;
    uint32_t unused_matrix_printed = 0;
    for (int a = matrix_no-1; a >= 0; a--) {
        MatrixShape& m = matrix_shape[a];
        uint32_t i = m.num;
        if (m.rows == 0) continue;
        bool use_matrix = true;

        //Over- or undersized
        if (use_matrix && m.rows > solver->conf.gaussconf.max_matrix_rows) {
            use_matrix = false;
            verb_print(1,"[matrix] Too many rows in matrix: " << m.rows << " -> set usage to NO");
        }
        if (use_matrix && m.cols > solver->conf.gaussconf.max_matrix_columns) {
            use_matrix = false;
            verb_print(1,"[matrix] Too many columns in matrix: " << m.cols << " -> set usage to NO");
        }

        if (use_matrix && m.rows < solver->conf.gaussconf.min_matrix_rows) {
            use_matrix = false;
            too_few_rows_matrix++;
            verb_print(2,"[matrix] Too few rows in matrix: " << m.rows << " -> set usage to NO");
        }

        //calculate sampling var ratio
        //for statistics ONLY
        double ratio_sampling = 0.0;
        if (solver->conf.sampling_vars_set) {
            //'seen' with what is in Matrix
            for(uint32_t int_var: reverseTable[i]) solver->seen[int_var] = 1;

            uint32_t tot_sampling_vars  = 0;
            uint32_t sampling_var_inside_matrix = 0;
            for(uint32_t outer_var: solver->conf.sampling_vars) {
                outer_var = solver->varReplacer->get_var_replaced_with_outer(outer_var);
                uint32_t int_var = solver->map_outer_to_inter(outer_var);
                tot_sampling_vars++;
                if (solver->value(int_var) != l_Undef) {
                    sampling_var_inside_matrix++;
                } else if (int_var < solver->nVars()
                    && solver->seen[int_var]
                ) {
                    sampling_var_inside_matrix++;
                }
            }

            //Clear 'seen'
            for(uint32_t int_var: reverseTable[i]) solver->seen[int_var] = 0;
            ratio_sampling = (double)sampling_var_inside_matrix/(double)tot_sampling_vars;
        }

        //Over the max number of matrixes
        if (use_matrix && realMatrixNum >= solver->conf.gaussconf.max_num_matrices) {
            verb_print(3, "[matrix] above max number of matrixes -> set usage to NO");
            use_matrix = false;
        }

        if (m.rows > solver->conf.gaussconf.min_matrix_rows) {
            //Override in case sampling vars ratio is high
            if (solver->conf.sampling_vars_set) {
                verb_print(2, "[matrix] ratio_sampling: " << ratio_sampling);
                if (ratio_sampling >= 0.6) { //TODO Magic constant
                    verb_print(1, "[matrix] sampling ratio good -> set usage to YES");
                    use_matrix = true;
                } else {
                    verb_print(2, "[matrix] sampling ratio bad -> set usage to NO");
                    use_matrix = false;
                }
            }
        }

        if (use_matrix) {
            solver->gmatrices.push_back(new EGaussian(solver, realMatrixNum, xorsInMatrix[i]));
            solver->gqueuedata.resize(solver->gmatrices.size());
            if (solver->conf.verbosity) cout << "c [matrix] Good   matrix " << std::setw(2) << realMatrixNum;
            realMatrixNum++;
            assert(solver->gmatrices.size() == realMatrixNum);
        } else {
            for(auto& x: xorsInMatrix[i]) {
                x.in_matrix = 1000;
                solver->xorclauses.push_back(x);
            }
            if (solver->conf.verbosity && unused_matrix_printed < 10) {
                if (m.rows >= solver->conf.gaussconf.min_matrix_rows || solver->conf.verbosity >= 2)
                cout << "c [matrix] UNused matrix   ";
            }
            unusedMatrix++;
        }

        if (solver->conf.verbosity) {
            double avg = (double)m.sum_xor_sizes/(double)m.rows;
            if (!use_matrix &&
                    ((m.rows < solver->conf.gaussconf.min_matrix_rows &&
                    solver->conf.verbosity < 2) ||
                    (unused_matrix_printed >= 10))
                ) continue;

            if (!use_matrix) unused_matrix_printed++;

            cout << std::setw(7) << m.rows << " x"
            << std::setw(5) << reverseTable[i].size()
            << "  density:"
            << std::setw(5) << std::fixed << std::setprecision(4) << m.density
            << "  xorlen avg: "
            << std::setw(5) << std::fixed << std::setprecision(2)  << avg;
            if (solver->conf.sampling_vars_set) {
                cout << "  perc of sampl vars: "
                << std::setw(5) << std::fixed << std::setprecision(3)
                << ratio_sampling*100.0 << " %";
            }
            cout  << endl;
        }
    }
    solver->attach_xorclauses();

    if (solver->conf.verbosity && unusedMatrix > 0) {
        cout << "c [matrix] unused matrices: " << unusedMatrix
        <<  " of which too few rows: " << too_few_rows_matrix << endl;
    }
    return realMatrixNum;
}
