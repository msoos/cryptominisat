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

#include "constants.h"
#include "solverconf.h"
#include <limits>
#include <iomanip>
#include <sstream>
using namespace CMSat;

//UPDATEs for DEVEL
//Fixing to:
//1832701 out-9660847.wlm01-4        239 147 92 a54ed65   --simdrat 1 --bva 1 --slsgetphase 1 --slstype ccnr_yalsat --confbtwsimp 70000 --confbtwsimpinc 1.1 --modbranchstr 3
//sat race 2019
//2049104 out-9833438.wlm01-7-drat0  220 135 85 c975a55   --simdrat 1 --slseveryn 2 --slstype ccnr --bvalim 150000 --tern 1 --terncreate 1 --ternkeep 6 -m 3 --distillincconf 0.04 --distillminconf 20000


//UPDATEs for MASTER
//1830735 out-9839185.wlm01-8-drat0  243 148 95 2a30cfb
//--simdrat 1 --bva 1 --slstype ccnr --slseveryn 2 --bvalim 250000 --tern 1 --terncreate 1 --ternkeep 6 -m 3 --distillincconf 0.02 --distillminconf 10000 --slsgetphase 1

//Fixing to:
//--simdrat 1 --bva 1 --slstype ccnr --slseveryn 2 --bvalim 250000 --tern 1 --terncreate 1 --ternkeep 6 -m 3 --distillincconf 0.02 --distillminconf 10000 --slsgetphase 1 --slstobump 100 --gluehist 60 --diffdeclevelchrono 20 --conftochrono 0

//Fixing to:
//1722973 out-9860239.wlm01-1-drat0 254 154 100 a6005cf   --simdrat 1 --gluehist 50 --moremoreminim 1 --lev1usewithin 70000 --bva2lit 1

//Fixing to:
//1838305 out-9882018.wlm01-1-drat0  241 145 96 16de7b4   --simdrat 1 --substimelimbinratio 0.1 --substimelimlongratio 0.9 --distilltier1ratio 0.03 --sublonggothrough 1.0 --varelimto 750 --bvaeveryn 7

//Fixing to:
//1706988 out-9885914.wlm01-6-drat0 253 153 100 def3339   --simdrat 1 --vsidsalternate 1 --vsidsalterval1 0.92 --vsidsalterval2 0.99 --maplealternate 1 --maplealterval1 0.70 --maplealterval2 0.90

//NOTE: diffdeclevel 0 is not good
//1813642 out-9896604.wlm01-0-drat0 242 146 96   6d8c7e2  _satcomp2020 --simdrat 1
//1867880 out-9896604.wlm01-1-drat0 237 139 98   6d8c7e2  _satcomp2020 --simdrat 1 --diffdeclevelchrono 0

//XOR is good, even on GAUSS
//1755046 out-9896604.wlm01-8-drat0  250 146 104 edd5be7  _devel --xor 1 --breakid 1 --breakideveryn 5
//1773691 out-9896604.wlm01-13-drat0 247 144 103 edd5be7  _devel --xor 0 --breakid 1 --breakideveryn 5

//Tuning to
//1684592 out-9915739.wlm01-3-drat0 256 149 107 b3b7cfb  _devel --printsol 0 --xorfindtout 400 --gaussusefulcutoff 0.2

//Tuning to
//1732414 out-9915739.wlm01-7-drat0 254 157 97 b3b7cfb  _devel --printsol 0 --simdrat 1 --polarstablen 4

//Tuning to
//1713867 out-175744.wlm01-11-drat0 252 154 98 de15a43  _devel --printsol 0 --simdrat 1 --slsbumptype 6 --polarbestinvmult 9 --lucky 20

//Predict setup that works for countbitswagner064:
//--predlongchunkmult 0.8 --predlongmult 0.8 --predshortmult 1.4 --predforevertopperc 40

//Tuning to
//mybase="--printsol 0 --simdrat 1 --predforeverpow 0.1 --predforevermult 0.40 --predlongmult 0.5 --predshortmult 0.5 --preddontmovetime 1 --predadjustsize 0 --predforeverchunkmult 4"


//tuning to
//--base="--printsol 0 --simdrat 1 --predforeverpow 0.1 --predforevermult 0.40 --predlongmult 0.5 --predshortmult 0.5 --preddontmovetime 1 --predadjustsize 0 --predforeverchunkmult 4 --occredmax 50 --branchstr vsidsx_once+maple1+maple2+vsids2+maple1+maple2+vsidsx --ternkeep 5 --distillmaxm 10"


DLL_PUBLIC SolverConf::SolverConf() :
        // Polarities
        polarity_mode(PolarityMode::polarmode_automatic)

        //Clause cleaning
        , pred_short_size(5500)
        , pred_long_size(18500)
        , pred_forever_size(10500) // Used only if pred_forever_cutoff is 0
        , pred_forever_cutoff(0) //this sets a static cutoff
        , order_tier2_by(2) //order Tier2 by this tier's sort function. 2 means Tier2, i.e. default

        , pred_forever_size_pow(0.01) // Used only if pred_forever_cutoff is 0
        //
        , pred_long_chunk(4700)
        , pred_forever_chunk(2000) // Used only if pred_forever_cutoff is 0
        , pred_forever_chunk_mult(0)
        //
        , move_from_tier0(1) //if 1 = moves it, rather than deletes it
        , move_from_tier1(1) //if 1 = moves it, rather than deletes it
        //
        , pred_long_check_every_n(3)
        , pred_forever_check_every_n(12)
        , pred_distill_only_smallgue(false)
        , pred_dontmove_until_timeinside(1) //always move, don't wait

        , every_lev1_reduce(10000) // kept for a while then moved to lev2
        , every_lev2_reduce(15000) // cleared regularly
        , every_pred_reduce(10000) //5000 seems to work better
        , must_touch_lev1_within(70000)

        , max_temp_lev2_learnt_clauses(30000) //only used if every_lev2_reduce==0
        , inc_max_temp_lev2_red_cls(1.0)      //only used if every_lev2_reduce==0
        , protect_cl_if_improved_glue_below_this_glue_for_one_turn(30)
        , glue_put_lev0_if_below_or_eq(3) // never removed
        , glue_put_lev1_if_below_or_eq(6) // kept for a while then moved to lev2
        #ifdef FINAL_PREDICTOR
        , dump_pred_distrib(0)
        #endif
        , clause_decay(0.999)
        , adjust_glue_if_too_many_tier0(0.7)
        , min_num_confl_adjust_glue_cutoff(150ULL*1000ULL)
        //NOTE: The "Scavel" system's "usedt" does NOT speed up the solver
        //test conducted: out-drat-check-8359337.wlm01-1-drat0

        //Restarting
        , restart_first(100)
        , restart_inc(1.1)
        , restartType(Restart::automatic)
        , do_blocking_restart(1)
        , blocking_restart_trail_hist_length(5000)
        , blocking_restart_multip(1.4)
        , fixed_restart_num_confl(100)
        , local_glue_multiplier(0.80)
        , shortTermHistorySize (50)
        , lower_bound_for_blocking_restart(10000)
        , ratio_glue_geom(5)
        , doAlwaysFMinim(false)

        //branch strategy
        , branch_strategy_setup("vmtf+vsids")

        //Clause minimisation
        , doRecursiveMinim (true)
        , doMinimRedMore(true)
        , doMinimRedMoreMore(2)
        , max_glue_more_minim(6)
        , max_size_more_minim(30)
        , more_red_minim_limit_binary(200)
        , max_num_lits_more_more_red_min(1)

        //Verbosity
        , verbosity        (0)
        , xor_detach_verb  (0)
        , doPrintGateDot   (false)
        , print_full_restart_stat   (false)
        , print_all_restarts (false)
        , verbStats        (1)
        , do_print_times(1)
        , print_restart_line_every_n_confl(8192)

        //Limits
        , maxTime          (numeric_limits<double>::max())
        , max_confl         (numeric_limits<uint64_t>::max())

        //Glues
        , update_glues_on_analyze(true)
        , max_glue_cutoff_gluehistltlimited(50)

        //Chono BT
        , diff_declev_for_chrono (20)

        //decision-based clause generation. These values have been validated
        //see 8099966.wlm01
        , do_decision_based_cl(1)
        , decision_based_cl_max_levels(9)
        , decision_based_cl_min_learned_size(50)

        //SQL
        , dump_individual_restarts_and_clauses(true)
        , dump_individual_cldata_ratio(0.01)
        , sql_overwrite_file(0)
        , lock_for_data_gen_ratio(0.1)

        //Var-elim
        , doVarElim        (true)
        , varelim_cutoff_too_many_clauses(2000)
        , do_empty_varelim (true)
        , do_full_varelim(true)
        , empty_varelim_time_limitM(300LL)
        , varelim_time_limitM(750)
        , varelim_sub_str_limitM(600)
        , varElimRatioPerIter(1.60)
        , velim_resolvent_too_large(20)
        , var_linkin_limit_MB(1000)
        , varelim_gate_find_limit(800)
        , varelim_check_resolvent_subs(false)

        //Subs, str limits for simplifier
        , subsumption_time_limitM(300)
        , weaken_time_limitM(300)
        , dummy_str_time_limitM(20)
        , subsumption_time_limit_ratio_sub_str_w_bin(0.1)
        , subsumption_time_limit_ratio_sub_w_long(0.9)
        , strengthening_time_limitM(300)
        , occ_based_lit_rem_time_limitM(50)


        //Ternary resolution
        , doTernary(true)
        , ternary_res_time_limitM(100)
        , ternary_keep_mult(5)
        , ternary_max_create(0.3)
        , allow_ternary_bin_create(false)

        //Bosphorus
        , do_bosphorus(false)
        , bosphorus_every_n(1)

        //BreakID
        , doBreakid(true)
        , breakid_use_assump(true)
        , breakid_every_n(5)
        , breakid_vars_limit_K(300)
        , breakid_cls_limit_K(600)
        , breakid_lits_limit_K(3500)
        , breakid_time_limit_K(2000)
        , breakid_max_constr_per_permut(50)
        , breakid_matrix_detect(true)

        //Bounded variable addition
        , do_bva(true)
        , min_bva_gain(16)
        , bva_limit_per_call(250000)
        , bva_also_twolit_diff(true)
        , bva_extra_lit_and_red_start(0)
        , bva_time_limitM(50)
        , bva_every_n(7)

        //Probing
        , do_full_probe    (true)
        , doIntreeProbe    (true)
        , doTransRed       (true)
        , full_probe_time_limitM(20ULL)
        , intree_time_limitM(400ULL)
        , intree_scc_varreplace_time_limitM(30ULL)
        , do_hyperbin_and_transred(true)

        //XOR
        , doFindXors       (true)
        , maxXorToFind     (7)
        , maxXorToFindSlow (5)
        , maxXORMatrix     (400ULL)
        , xor_finder_time_limitM(400)
        , allow_elim_xor_vars(1)
        , xor_var_per_cut(2)
        , force_preserve_xors(true)

        //Cardinality
        , doFindCard(0)

        //Var-replacer
        , doFindAndReplaceEqLits(true)
        , max_scc_depth (10000)

        //Iterative Alo Scheduling
        , simplify_at_startup(false)
        , simplify_at_every_startup(false)
        , do_simplify_problem(true)
        , full_simplify_at_startup(false)
        , never_stop_search(false)
        , num_conflicts_of_search(40ULL*1000ULL)
        , num_conflicts_of_search_inc(1.4)
        , num_conflicts_of_search_inc_max(10)
        , max_num_simplify_per_solve_call(25)
        , simplify_schedule_startup(
            "sub-impl, occ-backw-sub,"
            "scc-vrepl,"
            "breakid, "
            "occ-bve,occ-xor"
        )
        //validated with run 8114195.wlm01
        , simplify_schedule_nonstartup(
            //"scc-vrepl,"
            //"intree-probe,"
            "scc-vrepl,sub-impl,"
            "breakid,"
             //occurrence based
            "occ-backw-sub-str,occ-clean-implicit,occ-bve,"//occ-gates,"
            "occ-bva,occ-ternary-res,occ-xor,card-find,"
            //consolidate after OCC
            "cl-consolidate,"
            //strengthen again
            "scc-vrepl,"
            //renumber then it's time for SLS
            "renumber,"
            "bosphorus,"
            "louvain-comms,"
        )

        , simplify_schedule_external(
            "scc-vrepl,"
            "sub-impl,"
            "intree-probe,"
            "sub-str-cls-with-bin,distill-cls,distill-bins,"
            "scc-vrepl,sub-impl,str-impl,sub-impl,"
            "breakid,"
            //occurrence based
            "occ-backw-sub-str,occ-clean-implicit,occ-bve,"//occ-gates,"
            "occ-bva,occ-ternary-res,occ-xor,card-find,"
            //consolidate after OCC
            "cl-consolidate,"
            //strengthen again
            "str-impl,sub-str-cls-with-bin,distill-cls,distill-bins,"
            "scc-vrepl,"
            //renumber then it's time for SLS
            "renumber,"
            "sub-impl,"
            "bosphorus,"
            "louvain-comms,"
        )

        //Occur based simplification
        , perform_occur_based_simp(true)
        , do_strengthen_with_occur       (true)
        , maxRedLinkInSize (50)
        , maxOccurIrredMB  (2500)
        , maxOccurRedMB    (600)
        , maxOccurRedLitLinkedM(50)
        , subsume_gothrough_multip(1.0)

        //WalkSAT
        , doSLS(true)
        , sls_every_n(2)
        , yalsat_max_mems(10)
        , sls_memoutMB(500)
        , walksat_max_runs(50)
        , sls_get_phase(1)
        , sls_ccnr_asipire(1)
        , which_sls("ccnr")
        , sls_how_many_to_bump(100)
        , sls_bump_var_max_n_times(100)
        , sls_bump_type(6)

        //Distillation
        , do_distill_clauses(true)
        , do_distill_bin_clauses(true)
        , distill_long_cls_time_limitM(20ULL)
        , watch_based_str_time_limitM(20LL)
        , distill_increase_conf_ratio(0.10)
        , distill_min_confl(10000)
        , distill_red_tier0_ratio(10.0)
        , distill_red_tier1_ratio(0.03)
        , distill_irred_alsoremove_ratio(1.2)
        , distill_irred_noremove_ratio(1.0) //from out-3946531.wlm01-15-drat0
        , distill_rand_shuffle_order_every_n(3)
        #ifdef FINAL_PREDICTOR
        , distill_sort(3)
        #else
        , distill_sort(1)
        #endif

        //Memory savings
        , doRenumberVars   (true)
        , must_renumber    (false)
        , doSaveMem        (true)
        , full_watch_consolidate_every_n_confl (4ULL*1000ULL*1000ULL) //validated in run 8113323.wlm01

        //Misc optimisations
        , doStrSubImplicit (true)
        , subsume_implicit_time_limitM(100LL)
        , distill_implicit_with_implicit_time_limitM(200LL)

        //Gates
        , doGateFind       (false)
        , gatefinder_time_limitM(200)

        //Gauss
        , doM4RI(true)
        , xor_detach_reattach(false)
        , force_use_all_matrixes(false)

        //Sampling
        , sampling_vars(NULL)

        //Timeouts
        , global_next_multiplier(1.0)
        , orig_global_timeout_multiplier(3.0)
        , global_timeout_multiplier(1.0) // WILL BE UNSET, NOT RELEVANT
        , global_timeout_multiplier_multiplier(1.1)
        , global_multiplier_multiplier_max(3)
        , var_and_mem_out_mult(1.0)

        //Multi-thread, MPI
        , sync_every_confl(7000) //THREAD syncing
        , every_n_mpi_sync(3) //every N thread sync, we do an MPI sync
        , thread_num(0)
        , is_mpi(false)

        //misc
        , origSeed(0)
        , simulate_frat(false)
{
    ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
    ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.44;
}


DLL_PUBLIC std::string SolverConf::print_times(
    const double time_used
    , const bool time_out
    , const double time_remain
) const {
    if (do_print_times) {
        std::stringstream ss;
        ss
        << " T: " << std::setprecision(2) << std::fixed << time_used
        << " T-out: " << (time_out ? "Y" : "N")
        << " T-r: " << time_remain*100.0  << "%";

        return ss.str();
    }

    return std::string();
}

DLL_PUBLIC std::string SolverConf::print_times(
    const double time_used
    , const bool time_out
) const {
    if (do_print_times) {
        std::stringstream ss;
        ss
        << " T: " << std::setprecision(2) << std::fixed << time_used
        << " T-out: " << (time_out ? "Y" : "N");

        return ss.str();
    }

    return std::string();
}

DLL_PUBLIC std::string SolverConf::print_times(
    const double time_used
) const {
    if (do_print_times) {
        std::stringstream ss;
        ss
        << " T: " << std::setprecision(2) << std::fixed << time_used;

        return ss.str();
    }

    return std::string();
}
