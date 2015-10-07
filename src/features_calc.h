/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef __FEATURES_FAST_H__
#define __FEATURES_FAST_H__

#include <vector>
#include <limits>
#include <utility>
#include "features.h"
#include "cloffset.h"
using std::vector;
using std::pair;
using std::make_pair;

namespace CMSat {

class Solver;

struct FeaturesCalc {
public:
    FeaturesCalc(const Solver* _solver) :
        solver(_solver) {
    }
    Features extract();

private:
    void fill_vars_cls();
    void calculate_clause_stats();
    void calculate_variable_stats();
    void calculate_extra_var_stats();
    void calculate_extra_clause_stats();
    void calculate_cl_distributions(
        const vector<ClOffset>& clauses
        , struct Features::Distrib& distrib_data
    );


    const Solver* solver;
    template<class Function, class Function2>
    void for_one_clause(
        const Watched& cl
        , const Lit lit
        ,  Function func
        ,  Function2 func_each_lit
    ) const;
    template<class Function, class Function2>
    void for_all_clauses(Function for_each_clause,  Function2 func_each_lit) const;
    struct VARIABLE {
        int numPos = 0;
        int size = 0;
        int horn = 0;
    };

    vector<VARIABLE> myVars;
    Features feat;
};

} //end namespace

#endif //__FEATURES_FAST_H__
