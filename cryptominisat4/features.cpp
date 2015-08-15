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

    fprintf( stdout, "avg_confl_size %.5f%s", avg_confl_size, sep);
    fprintf( stdout, "avg_confl_glue %.5f%s", avg_confl_glue, sep);
    fprintf( stdout, "avg_num_resolutions %.5f%s", avg_num_resolutions, sep);
    fprintf( stdout, "avg_trail_depth_delta %.5f%s", avg_trail_depth_delta, sep);
    fprintf( stdout, "avg_branch_depth %.5f%s", avg_branch_depth, sep);
    fprintf( stdout, "avg_branch_depth_delta %.5f%s", avg_branch_depth_delta, sep);

    fprintf( stdout, "confl_size_min %.5f%s", confl_size_min, sep);
    fprintf( stdout, "confl_size_max %.5f%s", confl_size_max, sep);
    fprintf( stdout, "confl_glue_min %.5f%s", confl_glue_min, sep);
    fprintf( stdout, "confl_glue_max %.5f%s", confl_glue_max, sep);
    fprintf( stdout, "branch_depth_min %.5f%s", branch_depth_min, sep);
    fprintf( stdout, "branch_depth_max %.5f%s", branch_depth_max, sep);
    fprintf( stdout, "trail_depth_delta_min %.5f%s", trail_depth_delta_min, sep);
    fprintf( stdout, "trail_depth_delta_max %.5f%s", trail_depth_delta_max, sep);
    fprintf( stdout, "num_resolutions_min %.5f%s", num_resolutions_min, sep);
    fprintf( stdout, "num_resolutions_max %.5f%s", num_resolutions_max, sep);

    fprintf( stdout, "props_per_confl %.5f%s", props_per_confl, sep);
    fprintf( stdout, "confl_per_restart %.5f%s", confl_per_restart, sep);
    fprintf( stdout, "decisions_per_conflict %.5f%s", decisions_per_conflict, sep);
    fprintf( stdout, "learnt_bins_per_confl %.8f%s", learnt_bins_per_confl, sep);
    fprintf( stdout, "learnt_tris_per_confl %.8f", learnt_tris_per_confl);

    fprintf( stdout, "\n");
}
