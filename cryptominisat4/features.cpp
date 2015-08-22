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

#include "features.h"
#include <iostream>
using std::string;
using std::cout;
using std::endl;

using namespace CMSat;


void Features::print_stats() const
{
    cout << "c [features] ";
    cout << "numVars " << numVars << ", ";
    cout << "numClauses " << numClauses << ", ";
    cout << "var_cl_ratio " << var_cl_ratio << ", ";


    //Clause distribution
    cout << "binary " << binary << ", ";
    cout << "trinary " << trinary << ", ";
    cout << "horn " << horn << ", ";
    cout << "horn_mean " << horn_mean << ", ";
    cout << "horn_std " << horn_std << ", ";
    cout << "horn_min " << horn_min << ", ";
    cout << "horn_max " << horn_max << ", ";
    cout << "horn_spread " << horn_spread << ", ";

    cout << "vcg_var_mean " << vcg_var_mean << ", ";
    cout << "vcg_var_std " << vcg_var_std << ", ";
    cout << "vcg_var_min " << vcg_var_min << ", ";
    cout << "vcg_var_max " << vcg_var_max << ", ";
    cout << "vcg_var_spread " << vcg_var_spread << ", ";

    cout << "vcg_cls_mean " << vcg_cls_mean << ", ";
    cout << "vcg_cls_std " << vcg_cls_std << ", ";
    cout << "vcg_cls_min " << vcg_cls_min << ", ";
    cout << "vcg_cls_max " << vcg_cls_max << ", ";
    cout << "vcg_cls_spread " << vcg_cls_spread << ", ";

    cout << "pnr_var_mean " << pnr_var_mean << ", ";
    cout << "pnr_var_std " << pnr_var_std << ", ";
    cout << "pnr_var_min " << pnr_var_min << ", ";
    cout << "pnr_var_max " << pnr_var_max << ", ";
    cout << "pnr_var_spread " << pnr_var_spread << ", ";

    cout << "pnr_cls_mean " << pnr_cls_mean << ", ";
    cout << "pnr_cls_std " << pnr_cls_std << ", ";
    cout << "pnr_cls_min " << pnr_cls_min << ", ";
    cout << "pnr_cls_max " << pnr_cls_max << ", ";
    cout << "pnr_cls_spread " << pnr_cls_spread << ", ";

    //Conflicts
    cout << "avg_confl_size " << avg_confl_size << ", ";
    cout << "confl_size_min " << confl_size_min << ", ";
    cout << "confl_size_max " << confl_size_max << ", ";
    cout << "avg_confl_glue " << avg_confl_glue << ", ";
    cout << "confl_glue_min " << confl_glue_min << ", ";
    cout << "confl_glue_max " << confl_glue_max << ", ";
    cout << "avg_num_resolutions " << avg_num_resolutions << ", ";
    cout << "num_resolutions_min " << num_resolutions_min << ", ";
    cout << "num_resolutions_max " << num_resolutions_max << ", ";
    cout << "learnt_bins_per_confl " << learnt_bins_per_confl << ", ";
    cout << "learnt_tris_per_confl "<< learnt_tris_per_confl << ", ";

    //Search
    cout << "avg_branch_depth " << avg_branch_depth << ", ";
    cout << "branch_depth_min " << branch_depth_min << ", ";
    cout << "branch_depth_max " << branch_depth_max << ", ";
    cout << "avg_trail_depth_delta " << avg_trail_depth_delta << ", ";
    cout << "trail_depth_delta_min " << trail_depth_delta_min << ", ";
    cout << "trail_depth_delta_max " << trail_depth_delta_max << ", ";
    cout << "avg_branch_depth_delta " << avg_branch_depth_delta << ", ";
    cout << "props_per_confl " << props_per_confl << ", ";
    cout << "confl_per_restart " << confl_per_restart << ", ";
    cout << "decisions_per_conflict " << decisions_per_conflict << ", ";

    //distributions
    irred_cl_distrib.print("irred-");
    red_cl_distrib.print("red-");

    cout << "num_gates_found_last " << num_gates_found_last << ", ";
    cout << "num_xors_found_last " << num_xors_found_last;
    cout << endl;
}

void Features::Distrib::print(const string& pre_print) const
{
    cout << pre_print <<"glue_distr_mean " << glue_distr_mean << ", ";
    cout << pre_print <<"glue_distr_var " << glue_distr_var << ", ";
    cout << pre_print <<"size_distr_mean " << size_distr_mean << ", ";
    cout << pre_print <<"size_distr_var " << size_distr_var << ", ";
    cout << pre_print <<"uip_use_distr_mean " << uip_use_distr_mean << ", ";
    cout << pre_print <<"uip_use_distr_var " << uip_use_distr_var << ", ";
    cout << pre_print <<"activity_distr_mean " << activity_distr_mean << ", ";
    cout << pre_print <<"activity_distr_var " << activity_distr_var << ", ";
}
