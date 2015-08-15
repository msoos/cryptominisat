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

#ifndef _FEATURES_H_
#define _FEATURES_H_

#include <limits>

namespace CMSat {

struct Features
{
    void print_stats() const;

    int numVars;
    int numClauses;
    double binary = 0;
    double trinary = 0;
    double horn = 0;

    double eps = 0.00001;
    double vcg_var_mean = 0;
    double vcg_var_std = 0;
    double vcg_var_min = std::numeric_limits<double>::max();
    double vcg_var_max = std::numeric_limits<double>::min();
    double vcg_var_spread;

    double vcg_cls_mean = 0;
    double vcg_cls_std = 0;
    double vcg_cls_min = std::numeric_limits<double>::max();
    double vcg_cls_max = std::numeric_limits<double>::min();
    double vcg_cls_spread;

    double pnr_var_mean = 0;
    double pnr_var_std = 0;
    double pnr_var_min = std::numeric_limits<double>::max();
    double pnr_var_max = std::numeric_limits<double>::min();
    double pnr_var_spread;

    double pnr_cls_mean = 0;
    double pnr_cls_std = 0;
    double pnr_cls_min = std::numeric_limits<double>::max();
    double pnr_cls_max = std::numeric_limits<double>::min();
    double pnr_cls_spread;

    double horn_mean = 0;
    double horn_std = 0;
    double horn_min = std::numeric_limits<double>::max();
    double horn_max = std::numeric_limits<double>::min();
    double horn_spread;
    double var_cl_ratio;

    double avg_confl_size = 0.0;
    double avg_confl_glue = 0.0;
    double avg_num_resolutions = 0.0;
    double avg_trail_depth_delta = 0.0;
    double avg_branch_depth = 0.0;
    double avg_branch_depth_delta = 0.0;

    double confl_size_min = 0.0;
    double confl_size_max = 0.0;
    double confl_glue_min = 0.0;
    double confl_glue_max = 0.0;
    double branch_depth_min = 0.0;
    double branch_depth_max = 0.0;
    double trail_depth_delta_min = 0.0;
    double trail_depth_delta_max = 0.0;
    double num_resolutions_min = 0.0;
    double num_resolutions_max = 0.0;

    double props_per_confl = 0.0;
    double confl_per_restart = 0.0;
    double decisions_per_conflict = 0.0;
    double learnt_bins_per_confl = 0;
    double learnt_tris_per_confl = 0;
};

}

#endif //_FEATURES_H_
