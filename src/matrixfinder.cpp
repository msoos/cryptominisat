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

inline bool MatrixFinder::belong_same_matrix(const Xor& x)
{
    uint32_t comp_num = numeric_limits<uint32_t>::max();
    for (uint32_t v : x) {
        if (table[v] == var_Undef) {
            //Belongs to none, abort
            return false;
        }

        if (comp_num == numeric_limits<uint32_t>::max()) {
            //Belongs to this one
            comp_num = table[v];
        } else {
            if (comp_num != table[v]) {
                //Another var in this XOR belongs to another component
                return false;
            }
        }
    }
    return true;
}

bool MatrixFinder::find_matrices(bool& can_detach)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->ok);
    assert(solver->gmatrices.empty());
    can_detach = true;

    table.clear();
    table.resize(solver->nVars(), var_Undef);
    reverseTable.clear();
    clash_vars_unused.clear();
    matrix_no = 0;
    double myTime = cpuTime();

    XorFinder finder(NULL, solver);

    for(auto& x: solver->xorclauses_unused) solver->xorclauses.push_back(std::move(x));
    solver->xorclauses_unused.clear();
    solver->clauseCleaner->clean_xor_clauses(solver->xorclauses);

    finder.grab_mem();
    finder.move_xors_without_connecting_vars_to_unused();
    if (!finder.xor_together_xors(solver->xorclauses)) return false;

    finder.move_xors_without_connecting_vars_to_unused();
    finder.clean_equivalent_xors(solver->xorclauses);
    verb_print(1, "[matrix] unused xors from cleaning: " << solver->xorclauses_unused.size());
    for(const auto& x: solver->xorclauses_unused)
        clash_vars_unused.insert(x.clash_vars.begin(), x.clash_vars.end());

    if (solver->xorclauses.size() < solver->conf.gaussconf.min_gauss_xor_clauses) {
        can_detach = false;
        verb_print(4, "c [matrix] too few xor clauses for GJ: " << solver->xorclauses.size());
        return true;
    }

    if (solver->xorclauses.size() > solver->conf.gaussconf.max_gauss_xor_clauses
        && solver->conf.sampling_vars != NULL
    ) {
        can_detach = false;
        verb_print(1,
            "c WARNING sampling vars have been given but there"
            "are too many XORs and it would take too much time to put them"
            "into matrices. Skipping!");
        return true;
    }

    //Just one giant matrix.
    if (!solver->conf.gaussconf.doMatrixFind) {
        verb_print(1,"c Matrix finding disabled through switch. Putting all xors into matrix.");
        solver->gmatrices.push_back(new EGaussian(solver, 0, solver->xorclauses));
        solver->gqueuedata.resize(solver->gmatrices.size());
        return true;
    }

    vector<uint32_t> newSet;
    set<uint32_t> tomerge;
    for (const Xor& x : solver->xorclauses) {
        if (belong_same_matrix(x)) {
            continue;
        }

        tomerge.clear();
        newSet.clear();
        for (uint32_t v : x) {
            if (table[v] != var_Undef)
                tomerge.insert(table[v]);
            else
                newSet.push_back(v);
        }
        if (tomerge.size() == 1) {
            const uint32_t into = *tomerge.begin();
            auto intoReverse = reverseTable.find(into);
            for (uint32_t i = 0; i < newSet.size(); i++) {
                intoReverse->second.push_back(newSet[i]);
                table[newSet[i]] = into;
            }
            continue;
        }

        for (uint32_t v: tomerge) {
            newSet.insert(newSet.end(), reverseTable[v].begin(), reverseTable[v].end());
            reverseTable.erase(v);
        }
        for (uint32_t i = 0; i < newSet.size(); i++)
            table[newSet[i]] = matrix_no;
        reverseTable[matrix_no] = newSet;
        matrix_no++;
    }

    #ifdef VERBOSE_DEBUG
    for (map<uint32_t, vector<uint32_t> >::iterator it = reverseTable.begin()
        , end = reverseTable.end()
        ; it != end
        ; ++it
    ) {
        cout << "XOR table set: " << endl;
        for (vector<uint32_t>::iterator it2 = it->second.begin(), end2 = it->second.end()
            ; it2 != end2
            ; it2++
        ) {
            cout << *it2 << ", ";
        }
        cout << "-------" << endl;
    }
    #endif

    uint32_t numMatrixes = setMatrixes();

    const bool time_out =  false;
    const double time_used = cpuTime() - myTime;
    verb_print(1, "[matrix] Using " << numMatrixes
        << " matrices recovered from " << solver->xorclauses.size() << " xors"
        << solver->conf.print_times(time_used, time_out));

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "matrix find"
            , time_used
        );
    }

    return solver->okay();
}

uint32_t MatrixFinder::setMatrixes()
{
    if (solver->conf.sampling_vars) {
        uint32_t size_at_least = (double)solver->conf.sampling_vars->size()*3;
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

    for (const Xor& x : solver->xorclauses) {
        TBUDDY_DO(if (solver->frat->enabled()) assert(x.bdd));

        //take 1st variable to check which matrix it's in.
        const uint32_t matrix = table[x[0]];
        assert(matrix < matrix_no);

        //for stats
        matrix_shape[matrix].rows ++;
        matrix_shape[matrix].sum_xor_sizes += x.size();
        xorsInMatrix[matrix].push_back(x);
    }
    solver->xorclauses.clear();

    for(auto& m: matrix_shape) {
        if (m.tot_size() > 0) {
            m.density = (double)m.sum_xor_sizes / (double)(m.tot_size());
        }
    }

    std::sort(matrix_shape.begin(), matrix_shape.end(), mysorter());

    uint32_t realMatrixNum = 0;
    uint32_t unusedMatrix = 0;
    uint32_t too_few_rows_matrix = 0;
    uint32_t unused_matrix_printed = 0;
    for (int a = matrix_no-1; a >= 0; a--) {
        MatrixShape& m = matrix_shape[a];
        uint32_t i = m.num;
        if (m.rows == 0) {
            continue;
        }

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
        double ratio_sampling;
        if (solver->conf.sampling_vars) {
            //'seen' with what is in Matrix
            for(uint32_t int_var: reverseTable[i]) {
                solver->seen[int_var] = 1;
            }

            uint32_t tot_sampling_vars  = 0;
            uint32_t sampling_var_inside_matrix = 0;
            for(uint32_t outside_var: *solver->conf.sampling_vars) {
                uint32_t outer_var = solver->map_to_with_bva(outside_var);
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
            verb_print(3, "c [matrix] above max number of matrixes -> set usage to NO");
            use_matrix = false;
        }

        if (m.rows > solver->conf.gaussconf.min_matrix_rows) {
            //Override in case sampling vars ratio is high
            if (solver->conf.sampling_vars) {
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

        //if already detached, we MUST use the matrix
        for(const auto& x: xorsInMatrix[i]) {
            if (x.detached) {
                use_matrix = true;
                if (solver->conf.verbosity) {
                    cout << "c we MUST use the matrix, it contains a previously detached XOR"
                    << " -> set usage to YES" << endl;
                }
                break;
            }
        }

        if (solver->conf.force_use_all_matrixes) {
            use_matrix = true;
            if (solver->conf.verbosity) {
                cout << "c solver configured to force use all matrixes"
                << " -> set usage to YES" << endl;
            }
        }

        if (use_matrix) {
            solver->gmatrices.push_back(
                new EGaussian(solver, realMatrixNum, xorsInMatrix[i]));
            solver->gqueuedata.resize(solver->gmatrices.size());

            if (solver->conf.verbosity) {
                cout << "c [matrix] Good   matrix " << std::setw(2) << realMatrixNum;
            }
            realMatrixNum++;
            assert(solver->gmatrices.size() == realMatrixNum);
        } else {
            for(auto& x: xorsInMatrix[i]) {
                solver->xorclauses_unused.push_back(x);
                //cout<< "c [matrix]xor not in matrix, now unused_xors size: " << unused_xors.size() << endl;
                clash_vars_unused.insert(x.clash_vars.begin(), x.clash_vars.end());
            }
            if (solver->conf.verbosity && unused_matrix_printed < 10) {
                if (m.rows >= solver->conf.gaussconf.min_matrix_rows ||
                    solver->conf.verbosity >= 2)
                {
                    cout << "c [matrix] UNused matrix   ";
                }
            }
            unusedMatrix++;
        }

        if (solver->conf.verbosity) {
            double avg = (double)m.sum_xor_sizes/(double)m.rows;
            if (!use_matrix &&
                    ((m.rows < solver->conf.gaussconf.min_matrix_rows &&
                    solver->conf.verbosity < 2) ||
                    (unused_matrix_printed >= 10))
                )
            {
                continue;
            }

            if (!use_matrix) {
                unused_matrix_printed++;
            }

            cout << std::setw(7) << m.rows << " x"
            << std::setw(5) << reverseTable[i].size()
            << "  density:"
            << std::setw(5) << std::fixed << std::setprecision(4) << m.density
            << "  xorlen avg: "
            << std::setw(5) << std::fixed << std::setprecision(2)  << avg;
            if (solver->conf.sampling_vars) {
                cout << "  perc of sampl vars: "
                << std::setw(5) << std::fixed << std::setprecision(3)
                << ratio_sampling*100.0 << " %";
            }
            cout  << endl;
        }
    }

    if (solver->conf.verbosity && unusedMatrix > 0) {
        cout << "c [matrix] unused matrices: " << unusedMatrix
        <<  " of which too few rows: " << too_few_rows_matrix << endl;
    }

    return realMatrixNum;
}
