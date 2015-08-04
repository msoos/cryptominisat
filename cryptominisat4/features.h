#ifndef _FEATURES_H_
#define _FEATURES_H_

#include <limits>

namespace CMSat {

struct Features
{
    void print_stats() const;

    int numVars;
    int numClauses;
    double unary = 0;
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

    double lt_confl_size = 0.0;
    double lt_confl_glue = 0.0;
    double lt_num_resolutions = 0.0;
    double trail_depth_delta_hist = 0.0;
    double branch_depth_hist = 0.0;
    double branch_depth_delta_hist = 0.0;

    double lt_confl_size_min = 0.0;
    double lt_confl_size_max = 0.0;
    double lt_confl_glue_min = 0.0;
    double lt_confl_glue_max = 0.0;
    double branch_depth_hist_min = 0.0;
    double branch_depth_hist_max = 0.0;
    double trail_depth_delta_hist_min = 0.0;
    double trail_depth_delta_hist_max = 0.0;
    double lt_num_resolutions_min = 0.0;
    double lt_num_resolutions_max = 0.0;

    double props_per_confl = 0.0;
    double num_restarts = 0.0;
    double decisions = 0.0;
    double blocked_restart = 0.0;
    double learntBins = 0;
    double learntTris = 0;
};

}

#endif //_FEATURES_H_
