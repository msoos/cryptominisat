#ifndef Main_h
#define Main_h

#include "Global.h"

//=================================================================================================


extern bool opt_confl_1sub  ;
extern bool opt_confl_ksub  ;
extern bool opt_var_elim    ;
extern bool opt_0sub        ;
extern bool opt_1sub        ;
extern bool opt_2sub        ;
extern bool opt_repeated_sub;
extern bool opt_def_elim    ;
extern bool opt_unit_def    ;
extern bool opt_hyper1_res  ;
extern bool opt_pure_literal;
extern bool opt_asym_branch ;
extern bool opt_pre_sat     ;
extern bool opt_keep_all    ;
extern bool opt_no_random   ;

extern bool   opt_niver;
extern cchar* output_file;
extern cchar* varmap_file;
extern int    verbosity;

void verifyModel(cchar* input_file, cchar* model_file);


//=================================================================================================
#endif
