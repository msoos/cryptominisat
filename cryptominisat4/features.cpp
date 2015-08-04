#include "features.h"
#include <stdio.h>

using namespace CMSat;


void Features::print_stats() const
{
    const char* sep = ", ";

    fprintf( stdout, "c [features] ");
    fprintf( stdout, "numVars %d%s", numVars, sep );
    fprintf( stdout, "numClauses %d%s", numClauses, sep );
    //was: (numVars/(1.0*numClauses)
    fprintf( stdout, "var_cl_ratio %.5f%s", var_cl_ratio, sep );

    fprintf( stdout, "vcg_var_mean %.5f%s", vcg_var_mean, sep );
    fprintf( stdout, "vcg_var_std %.5f%s", vcg_var_std, sep );
    fprintf( stdout, "vcg_var_min %.5f%s", vcg_var_min, sep );
    fprintf( stdout, "vcg_var_max %.5f%s", vcg_var_max, sep );
    fprintf( stdout, "vcg_var_spread %.5f%s", vcg_var_spread, sep );

    fprintf( stdout, "vcg_cls_mean %.5f%s", vcg_cls_mean, sep );
    fprintf( stdout, "vcg_cls_std %.5f%s", vcg_cls_std, sep );
    fprintf( stdout, "vcg_cls_min %.5f%s", vcg_cls_min, sep );
    fprintf( stdout, "vcg_cls_max %.5f%s", vcg_cls_max, sep );
    fprintf( stdout, "vcg_cls_spread %.5f%s", vcg_cls_spread, sep );

    fprintf( stdout, "pnr_var_mean %.5f%s", pnr_var_mean, sep );
    fprintf( stdout, "pnr_var_std %.5f%s", pnr_var_std, sep );
    fprintf( stdout, "pnr_var_min %.5f%s", pnr_var_min, sep );
    fprintf( stdout, "pnr_var_max %.5f%s", pnr_var_max, sep );
    fprintf( stdout, "pnr_var_spread %.5f%s", pnr_var_spread, sep );

    fprintf( stdout, "pnr_cls_mean %.5f%s", pnr_cls_mean, sep );
    fprintf( stdout, "pnr_cls_std %.5f%s", pnr_cls_std, sep );
    fprintf( stdout, "pnr_cls_min %.5f%s", pnr_cls_min, sep );
    fprintf( stdout, "pnr_cls_max %.5f%s", pnr_cls_max, sep );
    fprintf( stdout, "pnr_cls_spread %.5f%s", pnr_cls_spread, sep );

    fprintf( stdout, "unary %.5f%s", unary, sep );
    fprintf( stdout, "binary %.5f%s", binary, sep );
    fprintf( stdout, "trinary %.5f%s", trinary, sep );
    fprintf( stdout, "horn_mean %.5f%s", horn_mean, sep );
    fprintf( stdout, "horn_std %.5f%s", horn_std, sep );
    fprintf( stdout, "horn_min %.5f%s", horn_min, sep );
    fprintf( stdout, "horn_max %.5f%s", horn_max, sep );
    fprintf( stdout, "horn_spread %.5f%s", horn_spread, sep );
    fprintf( stdout, "horn %.5f%s", horn, sep);

    fprintf( stdout, "lt_confl_size %.5f%s", lt_confl_size, sep);
    fprintf( stdout, "lt_confl_glue %.5f%s", lt_confl_glue, sep);
    fprintf( stdout, "lt_num_resolutions %.5f%s", lt_num_resolutions, sep);
    fprintf( stdout, "trail_depth_delta_hist %.5f%s", trail_depth_delta_hist, sep);
    fprintf( stdout, "branch_depth_hist %.5f%s", branch_depth_hist, sep);
    fprintf( stdout, "branch_depth_delta_hist %.5f%s", branch_depth_delta_hist, sep);

    fprintf( stdout, "lt_confl_size_min %.5f%s", lt_confl_size_min, sep);
    fprintf( stdout, "lt_confl_size_max %.5f%s", lt_confl_size_max, sep);
    fprintf( stdout, "lt_confl_glue_min %.5f%s", lt_confl_glue_min, sep);
    fprintf( stdout, "lt_confl_glue_max %.5f%s", lt_confl_glue_max, sep);
    fprintf( stdout, "branch_depth_hist_min %.5f%s", branch_depth_hist_min, sep);
    fprintf( stdout, "branch_depth_hist_max %.5f%s", branch_depth_hist_max, sep);
    fprintf( stdout, "trail_depth_delta_hist_min %.5f%s", trail_depth_delta_hist_min, sep);
    fprintf( stdout, "trail_depth_delta_hist_max %.5f%s", trail_depth_delta_hist_max, sep);
    fprintf( stdout, "lt_num_resolutions_min %.5f%s", lt_num_resolutions_min, sep);
    fprintf( stdout, "lt_num_resolutions_max %.5f%s", lt_num_resolutions_max, sep);

    fprintf( stdout, "props_per_confl %.5f%s", props_per_confl, sep);
    fprintf( stdout, "num_restarts %.5f%s", num_restarts, sep);
    fprintf( stdout, "decisions %.5f%s", decisions, sep);
    fprintf( stdout, "blocked_restart %.5f%s", blocked_restart, sep);
    fprintf( stdout, "learntBins %.5f%s", learntBins, sep);
    fprintf( stdout, "learntTris %.5f", learntTris);

    fprintf( stdout, "\n");
}
