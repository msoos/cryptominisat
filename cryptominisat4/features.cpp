#include "features.h"
#include <stdio.h>

using namespace CMSat;


void Features::print_stats() const
{
    const char* sep = ", ";

    fprintf( stdout, "c [features] ");
    fprintf( stdout, "numVars: %d%s", numVars, sep );
    fprintf( stdout, "numClauses: %d%s", numClauses, sep );
    fprintf( stdout, "(numVars/(1.0*numClauses) %.5f%s", var_cl_ratio, sep );

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
    fprintf( stdout, "horn %.5f", horn );

    fprintf( stdout, "lt_confl_size %.5f", lt_confl_size, sep);
    fprintf( stdout, "lt_confl_glue %.5f", lt_confl_glue, sep);
    fprintf( stdout, "lt_num_resolutions %.5f", lt_num_resolutions, sep);
    fprintf( stdout, "trail_depth_delta_hist %.5f", trail_depth_delta_hist, sep);
    fprintf( stdout, "branch_depth_hist %.5f", branch_depth_hist, sep);
    fprintf( stdout, "branch_depth_delta_hist %.5f", branch_depth_delta_hist, sep);

    fprintf( stdout, "\n");
}
