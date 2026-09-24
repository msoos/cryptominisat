/******************************************
Copyright (C) 2009-2026 Authors of CryptoMiniSat, see AUTHORS file

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

#pragma once

#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "constants.h"
#include "solverconf.h"

namespace CMSat {

template<class T> T parse_opt(const std::string& s) {
    if constexpr (std::is_same_v<T, std::string>) {
        return s;
    } else if constexpr (std::is_same_v<T, bool>) {
        return parse_opt<int>(s) != 0;
    } else if constexpr (std::is_floating_point_v<T>) {
        size_t pos = 0;
        double val;
        try { val = std::stod(s, &pos); }
        catch (const std::exception&) { throw std::invalid_argument("not a number: " + s); }
        if (pos != s.size()) throw std::invalid_argument("trailing characters in number: " + s);
        return val;
    } else if constexpr (std::is_integral_v<T>) {
        T val{};
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc::result_out_of_range) throw std::invalid_argument("integer out of range: " + s);
        if (ec != std::errc{}) throw std::invalid_argument("not an integer: " + s);
        if (ptr != s.data() + s.size()) throw std::invalid_argument("trailing characters in integer: " + s);
        return val;
    } else {
        static_assert(sizeof(T) == 0, "parse_opt: unsupported option type");
    }
}

inline PolarityMode parse_polarity(const std::string& mode) {
    if (mode == "true") return PolarityMode::polarmode_pos;
    if (mode == "false") return PolarityMode::polarmode_neg;
    if (mode == "rnd") return PolarityMode::polarmode_rnd;
    if (mode == "auto") return PolarityMode::polarmode_automatic;
    if (mode == "weight") return PolarityMode::polarmode_weighted;
    throw std::invalid_argument("unknown polarity mode: " + mode);
}

struct ConfOpt {
    const char* name;
    const char* help;
    bool lib = false; // settable via SATSolver::set_option()
    const char* short_name = nullptr;
};

// Every command-line option that sets a SolverConf member. main.cpp registers
// these with argparse, SATSolver::set_option() looks up the ones marked lib.
template<class F> void for_each_conf_opt(SolverConf& conf, F&& f) {
    f({"--verb", "[0-10] Verbosity of solver. 0 = only solution"}, conf.verbosity);
    f({"--prefix", "Prefix of every comment line printed"}, conf.prefix);
    f({"--seed", "[0..] Random seed", true, "-r"}, conf.orig_seed);
    f({"--mult", "Time multiplier for all simplification cutoffs", true, "-m"}, conf.orig_global_timeout_multiplier);
    f({"--nextm", "Global multiplier when the next inprocessing should take place"}, conf.global_next_multiplier);
    f({"--memoutmult", "Multiplier for memory-out checks on inprocessing functions. It limits things such as clause-link-in. Useful when you have limited memory but still want to do some inprocessing"}, conf.var_and_mem_out_mult);
    f({"--scc", "Find equivalent literals through SCC and replace them", true}, conf.do_find_and_replace_eq_lits);
    #ifdef FINAL_PREDICTOR
    f({"--predloc", "Directory with predictor-<table>-<tier>-<type>.json (see --predtables), empty = use built-in models"}, conf.pred_conf_location);
    f({"--predtype", "Type of predictor. Supported: py, xgb"}, conf.predictor_type);
    f({"--predtables", "Per short/long/forever: 0 = used_later, 1 = used_later_anc. 000 = normal for all, 111 = ancestor for all"}, conf.pred_tables);
    f({"--predbestfeats", "Best features file, only for --predtype py"}, conf.predict_best_feat_fname);
    f({"--predsortby", "Reduce removes the candidates with the lowest predicted use over the next: 0 = short, 1 = long, 2 = forever horizon, 3 = sum of the three (near-term counts 3x, long-term still counts)"}, conf.pred_sort_by);
    f({"--dumppreddistrib", "Dump predictions of all clauses at every reduce to pred_distrib.csv"}, conf.dump_pred_distrib);
    #endif
    f({"--restart", "Enable restarts", true}, conf.do_restart);
    f({"--restartint", "Minimum number of conflicts between restarts"}, conf.restartint);
    f({"--restartmargin", "Percent the fast glue EMA must be above the slow one to restart"}, conf.restartmargin);
    f({"--emagluefast", "Window size of the fast glue EMA"}, conf.emagluefast);
    f({"--emaglueslow", "Window size of the slow glue EMA"}, conf.emaglueslow);
    f({"--stabilize", "Alternate stable (reluctant doubling) and focused (glue EMA) phases", true}, conf.do_stabilize);
    f({"--stabilizeint", "Length of first stabilizing phase, in conflicts"}, conf.stabilizeint);
    f({"--stabilizefactor", "Multiplier of stabilizing phase length at each phase change"}, conf.stabilizefactor);
    f({"--stabilizemaxint", "Maximum stabilizing phase length"}, conf.stabilizemaxint);
    f({"--reluctant", "Reluctant doubling base period for stable-phase restarts, 0 = never restart there"}, conf.reluctantint);
    f({"--reluctantmax", "Maximum reluctant doubling period multiplier"}, conf.reluctantmax);
    f({"--reduce", "Enable learnt clause DB reduction", true}, conf.reduce);
    f({"--reduceint", "Base reduce interval, in conflicts"}, conf.reduceint);
    f({"--reducetarget", "Percent of unused reduce candidates removed per reduce"}, conf.reducetarget);
    f({"--reducelow", "Fraction per mille of reduce candidates removed at the first reduce, rising towards --reducehigh as kissat. Set >= reducehigh to use --reducetarget"}, conf.reducelow);
    f({"--reducehigh", "Asymptotic fraction per mille of reduce candidates removed"}, conf.reducehigh);
    f({"--reducekeepused", "Keep every learnt clause used since the last reduce, as CaDiCaL. 0: kissat, tier3 is always a candidate"}, conf.reduce_keep_used);
    f({"--eagersubsume", "Demote the last learnt clauses subsumed by a new one, as kissat"}, conf.eager_subsume);
    f({"--reducetier1glue", "Glue at/below which learnt clauses are kept forever"}, conf.reducetier1glue);
    f({"--reducetier2glue", "Glue at/below which learnt clauses get a double life"}, conf.reducetier2glue);
    f({"--dyntiers", "Recompute tier1/tier2 glue limits from glue usage at each reduce, as kissat"}, conf.dynamic_tiers);
    f({"--flush", "Once in a while flush ALL unused redundant clauses"}, conf.flush);
    f({"--flushfactor", "Flush interval multiplier"}, conf.flushfactor);
    f({"--flushint", "Initial flush interval, in conflicts"}, conf.flushint);
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    f({"--everypred", "Calculate satzilla features every N conflicts (STATS builds)"}, conf.every_pred_reduce);
    #endif
    f({"--branchstr", "Branch strategy string that switches between different branch strategies while solving e.g. 'vsids1+vsids2'", true}, conf.branch_strategy_setup);
    f({"--sls", "Run local search ('walk') during rephasing", true}, conf.do_sls);
    f({"--walknonstable", "Run local search during focused phases too"}, conf.walknonstable);
    f({"--walkseedphase", "Start local search off the CDCL phases, as CaDiCaL does"}, conf.walkseedphase);
    f({"--walkinitially", "Local search rounds to run before simplifying and searching, 0=none"}, conf.walkinitially);
    f({"--walkxorweight", "Weight of XOR constraints in yalsat's break values, times 100 (range 0-1000)"}, conf.walkxorweight);
    f({"--walkmineff", "Minimum local search effort, in yalsat mems"}, conf.walkmineff);
    f({"--walkmaxeff", "Maximum local search effort, in yalsat mems"}, conf.walkmaxeff);
    f({"--walkreleff", "Local search effort per mille of the search propagations done so far"}, conf.walkreleff);
    f({"--slsmaxmem", "Maximum number of MB to give to the local search solver. Skips local search if handing over the formula would need more."}, conf.sls_memoutMB);
    f({"--ccnrneighmaxsz", "CCNR builds no neighbor edges for clauses longer than this. The neighborhood is quadratic in clause size, so one huge clause costs GBs and makes every flip of its vars charge thousands of mems"}, conf.ccnr_neighbor_max_cl_size);
    f({"--backboneccnrlim", "Mems budget, in millions, for each of the CCNR local search tries that pre-filter backbone candidates. Too low and no model is found, so cadiback must test every variable"}, conf.backbone_ccnr_mems_limitM);
    f({"--rephase", "Enable resetting the saved phases", true}, conf.do_rephase);
    f({"--rephaseint", "Rephase interval, in conflicts. The interval grows arithmetically."}, conf.rephaseint);
    f({"--phase", "Default decision polarity"}, conf.phase);
    f({"--lucky", "Search for lucky phases before the CDCL loop", true}, conf.lucky);
    f({"--target", "Decide on target phases. 0 = never, 1 = stable phases only, 2 = always", true}, conf.target_phases);
    f({"--transred", "Remove useless binary clauses (transitive reduction)", true}, conf.do_trans_red);
    f({"--intree", "Carry out intree-based probing", true}, conf.do_intree_probe);
    f({"--fullprobe", "Regularly probe both polarities of variables during search"}, conf.do_full_probe);
    f({"--fullprobemaxm", "Time in mega-bogoprops to perform full probing"}, conf.full_probe_time_limitM);
    f({"--intreemaxm", "Time in mega-bogoprops to perform intree probing"}, conf.intree_time_limitM);
    f({"--intreeeff", "Intree probing budget as a fraction of all propagations since its last call"}, conf.intree_effort);
    f({"--otfhyper", "Perform hyper-binary resolution during probing"}, conf.do_hyperbin_and_transred);
    f({"--hyperkeepconfl", "Conflicts an unused intree hyper-bin is kept for before being dropped"}, conf.hyperbin_keep_confl);
    f({"--schedsimp", "Perform simplification rounds. If 0, we never perform any.", true}, conf.do_simplify_problem);
    f({"--presimp", "Perform simplification at the very start", true}, conf.simplify_at_startup);
    f({"--allpresimp", "Perform simplification at EVERY start -- only matters in library mode"}, conf.simplify_at_every_startup);
    f({"--nonstop", "Never stop the search() process in class SATSolver", true, "-n"}, conf.never_stop_search);
    f({"--maxnumsimppersolve", "Maximum number of simplifications to perform for every solve() call. After this, no more inprocessing will take place."}, conf.max_num_simplify_per_solve_call);
    f({"--schedule", "Schedule for simplification during run", true}, conf.simplify_schedule_nonstartup);
    f({"--preschedule", "Schedule for simplification at startup", true}, conf.simplify_schedule_startup);
    f({"--occsimp", "Perform occurrence-list-based optimisations (variable elimination, subsumption, bounded variable addition...)", true}, conf.perform_occur_based_simp);
    f({"--confbtwsimp", "Start first simplification after this many conflicts", true}, conf.num_conflicts_of_search);
    f({"--confbtwsimpinc", "Simp rounds increment by this power of N"}, conf.num_conflicts_of_search_inc);
    f({"--tern", "Perform Ternary resolution"}, conf.do_ternary);
    f({"--terntimelim", "Time-out in bogoprops M of ternary resolution as per paper 'Look-Ahead Versus Look-Back for Satisfiability Problems'"}, conf.ternary_res_time_limitM);
    f({"--terncreate", "Create only this multiple (of linked in cls) ternary resolution clauses per simp run"}, conf.ternary_max_create);
    f({"--ternbincreate", "Allow ternary resolving to generate binary clauses"}, conf.allow_ternary_bin_create);
    f({"--occredmax", "Don't add to occur list any redundant clause larger than this"}, conf.max_red_link_in_size);
    f({"--occredmaxmb", "Don't allow redundant occur size to be beyond this many MB"}, conf.max_occur_red_mb);
    f({"--occirredmaxmb", "Don't allow irredundant occur size to be beyond this many MB"}, conf.max_occur_irred_mb);
    f({"--strengthen", "Perform clause contraction through self-subsuming resolution as part of the occurrence-subsumption system"}, conf.do_strengthen_with_occur);
    f({"--weakentimelim", "Time-out in bogoprops M of weakening used"}, conf.weaken_time_limitM);
    f({"--substimelim", "Time-out in bogoprops M of subsumption of long clauses with long clauses, after computing occur"}, conf.subsumption_time_limitM);
    f({"--substimelimbinratio", "Ratio of subsumption time limit to spend on sub&str long clauses with bin"}, conf.subsumption_time_limit_ratio_sub_str_w_bin);
    f({"--substimelimlongratio", "Ratio of subsumption time limit to spend on sub long clauses with long"}, conf.subsumption_time_limit_ratio_sub_w_long);
    f({"--strstimelim", "Time-out in bogoprops M of strengthening of long clauses with long clauses, after computing occur"}, conf.strengthening_time_limitM);
    f({"--sublonggothrough", "How many times go through subsume"}, conf.subsume_gothrough_multip);
    f({"--bva", "Perform bounded variable addition", true}, conf.do_bva);
    f({"--varelim", "Perform variable elimination as per Een and Biere", true}, conf.do_var_elim);
    f({"--varelimto", "Var elimination bogoprops M time limit"}, conf.varelim_time_limitM);
    f({"--varelimover", "Do BVE until the resulting no. of clause increase is less than X. Only power of 2 makes sense, i.e. 2,4,8..."}, conf.min_bva_gain);
    f({"--emptyelim", "Perform empty resolvent elimination using bit-map trick"}, conf.do_empty_varelim);
    f({"--varelimmaxmb", "Maximum extra MB of memory to use for new clauses during varelim"}, conf.var_linkin_limit_MB);
    f({"--eratio", "Eliminate this ratio of free variables at most per variable elimination iteration"}, conf.var_elim_ratio_per_iter);
    f({"--varelimocclim", "Don't try to eliminate a variable whose more frequent polarity occurs more than this many times. 0 = no limit"}, conf.varelim_occ_cutoff);
    f({"--varelimprodlim", "Don't try to eliminate a variable whose pos*neg occurrence product is over this"}, conf.varelim_occ_prod_cutoff);
    f({"--varelimschedtouched", "Only schedule for elimination the vars whose clauses changed since BVE last looked (CaDiCaL's Flags::elim). 0 = schedule every eligible var"}, conf.varelim_sched_only_touched);
    f({"--weakenclsmaxsz", "Don't weaken a clause longer than this during BVE. 0 = no limit"}, conf.weaken_max_cls_size);
    f({"--varelimclsmaxsz", "Don't try to eliminate a variable that occurs in a clause longer than this. 0 = no limit"}, conf.varelim_max_cls_size);
    f({"--varelimclslim", "Maximum resolvent size during BVE, -1 = no limit"}, conf.velim_resolvent_too_large);
    f({"--varelimirregocclim", "Don't run kitten-based irregular gate finding if the variable has more occurrences than this"}, conf.varelim_irreg_gate_occ_cutoff);
    f({"--varelimirregconfl", "Picosat conflict budget for one irregular gate query during BVE"}, conf.varelim_irreg_gate_confl_limit);
    f({"--varelimirregunit", "Turn a one-sided irregular-gate core into a unit instead of a gate"}, conf.varelim_irreg_gate_unit);
    f({"--varelimprod", "Weight of pos*neg in the BVE ordering score"}, conf.varelim_score_prod);
    f({"--varelimsum", "Weight of pos+neg in the BVE ordering score"}, conf.varelim_score_sum);
    f({"--varelimcheckres", "BVE should check whether resolvents subsume others and check for exact size increase"}, conf.varelim_check_resolvent_subs);
    f({"--occrelocatelim", "When strengthening removes a literal whose occurrence list is longer than this, move the clause to a new place instead of searching the list"}, conf.occ_relocate_lim);
    f({"--xor", "Discover long XORs", true}, conf.do_find_xors);
    f({"--maxxorsize", "Maximum XOR size to find", true}, conf.max_xor_to_find);
    f({"--xorfindtout", "Time limit for finding XORs", true}, conf.xor_finder_time_limitM);
    f({"--maxxormat", "Maximum matrix size (=num elements) that we should try to echelonize", true}, conf.max_xor_matrix);
    f({"--gates", "Find gates."}, conf.do_gate_find);
    f({"--printgatedot", "Print gate structure regularly to file 'gatesX.dot'"}, conf.do_print_gate_dot);
    f({"--gatefindto", "Max time in bogoprops M to find gates"}, conf.gatefinder_time_limitM);
    f({"--recur", "Perform recursive minimisation"}, conf.do_recursive_minim);
    f({"--moreminim", "Perform strong minimisation at conflict gen."}, conf.do_minim_red_more);
    f({"--moremoreminim", "Perform even stronger minimisation at conflict gen."}, conf.do_minim_red_more_more);
    f({"--moremorealways", "Always strong-minimise clause"}, conf.do_always_fminim);
    f({"--decbased", "Create decision-based conflict clauses when the UIP clause is too large"}, conf.do_decision_based_cl);
    f({"--bumpreasondepth", "Bump vars in reasons of learnt clause lits up to this depth. 0 = off"}, conf.bump_reason_depth);
    f({"--shrink", "All-UIP shrinking of learnt clauses"}, conf.do_shrink_uip);
    f({"--otfs", "On-the-fly strengthening of clauses during conflict analysis"}, conf.do_otfs);
    f({"--diffdeclevelchrono", "Difference in decision level is more than this, perform chronological backtracking instead of non-chronological backtracking. Giving -1 means it is never turned on (overrides '--confltochrono -1' in this case)."}, conf.diff_declev_for_chrono);
    f({"--chronoreusetrail", "On backjump, only backtrack to the level of the best-ranked var above the jump level"}, conf.do_chrono_reuse_trail);
    f({"--restartreusetrail", "On restart, keep decisions that would be re-made anyway"}, conf.do_restart_reuse_trail);
    #ifdef USE_SQLITE3
    f({"--sqlitedboverwrite", "Overwrite the SQLite database file if it exists"}, conf.sql_overwrite_file);
    f({"--cldatadumpratio", "Only dump this ratio of clauses' data, randomly selected. Since machine learning doesn't need that much data, this can reduce the data you have to deal with."}, conf.dump_individual_cldata_ratio);
    f({"--cllockdatagen", "Lock for data generation into lev0, setting locked_for_data_gen. Only works when clause is marked for dumping ('--cldatadumpratio' )"}, conf.lock_for_data_gen_ratio);
    #endif
    f({"--verbstat", "Change verbosity of statistics at the end of the solving [0..3]"}, conf.verb_stats);
    f({"--verbrestart", "Print more thorough, but different stats"}, conf.print_full_restart_stat);
    f({"--verballrestarts", "Print a line for every restart"}, conf.print_all_restarts);
    f({"--restartprint", "Print restart status lines at least every N conflicts"}, conf.print_restart_line_every_n_confl);
    f({"--distill", "Regularly execute clause distillation", true}, conf.do_distill_clauses);
    f({"--distillbin", "Regularly execute binary clause distillation"}, conf.do_distill_bin_clauses);
    f({"--distillbineff", "Binary clause distillation budget as a fraction of all propagations since its last call"}, conf.distill_bin_effort);
    f({"--distillmaxm", "Maximum number of Mega-bogoprops(~time) to spend on vivifying/distilling long cls by enqueueing and propagating"}, conf.distill_long_cls_time_limitM);
    f({"--distillincconf", "Multiplier for current number of conflicts OTF distill"}, conf.distill_increase_conf_ratio);
    f({"--distillminconf", "Minimum number of conflicts between OTF distill"}, conf.distill_min_confl);
    f({"--distillredreleff", "Per-mille of all bogoprops since last call to spend distilling red cls"}, conf.distill_red_releff);
    f({"--distillirredreleff", "Per-mille of all bogoprops since last call to spend distilling irred cls"}, conf.distill_irred_releff);
    f({"--distillmineffm", "Floor of the distill effort reference, in mega-bogoprops"}, conf.distill_min_effortM);
    f({"--distillschedmax", "Max clauses scheduled per distill pass"}, conf.distill_sched_max);
    f({"--distillinst", "Try to remove the last literal during distillation"}, conf.distill_instantiate);
    f({"--xorgatemaxsize", "Largest clause XOR-gate finding considers, before the log2 occurrence cap", true}, conf.xor_gate_find_maxsize);
    f({"--distillremlevel", "Clause removal during distillation. 0 = never, 1 = only on a real conflict, 2 = also when a literal is positively implied"}, conf.distill_rem_level);
    f({"--distillirredalsoremratio", "How much of irred to distill when doing also removal"}, conf.distill_irred_alsoremove_ratio);
    f({"--distillirrednoremratio", "How much of irred to distill when doing no removal"}, conf.distill_irred_noremove_ratio);
    f({"--oraclemult", "Time multiplier for all oracle-based (oracle-vivif*, oracle-sparsify*) cutoffs"}, conf.oracle_mult);
    f({"--oraclegetlearnts", "Keep the clauses the oracle learnt during vivification as redundant clauses"}, conf.oracle_get_learnts);
    f({"--oracleremovedislearnt", "Clauses removed by the oracle are re-added as redundant instead of being deleted"}, conf.oracle_removed_is_learnt);
    f({"--oraclefindbins", "[0..] Effort spent looking for binary clauses during oracle vivification. 0 = off"}, conf.oracle_find_bins);
    f({"--sweep", "Perform SAT sweeping with kitten (occ-sweep)", true}, conf.do_sweep);
    f({"--sweeptimelimM", "Tick limit cap for one occ-sweep run, in millions"}, conf.sweep_time_limitM);
    f({"--sweepeff", "occ-sweep budget as a fraction of all bogoprops since its last call"}, conf.sweep_effort);
    f({"--sweepmineffm", "Floor of the occ-sweep budget, in mega-ticks"}, conf.sweep_min_effortM);
    f({"--sweepvars", "Starting number of variables in a sweeping environment"}, conf.sweep_vars);
    f({"--sweepclauses", "Starting number of clauses in a sweeping environment"}, conf.sweep_clauses);
    f({"--sweepdepth", "Starting depth of a sweeping environment"}, conf.sweep_depth);
    f({"--sweepfliprounds", "Rounds of model flipping during sweeping"}, conf.sweep_flip_rounds);
    f({"--renumber", "Renumber variables to increase CPU cache efficiency"}, conf.do_renumber_vars);
    f({"--mustconsolidate", "Always consolidate, even if not useful. This is used for debugging ONLY"}, conf.must_always_conslidate);
    f({"--savemem", "Save memory by deallocating variable space after renumbering. Only works if renumbering is active."}, conf.do_save_mem);
    f({"--mustrenumber", "Treat all 'renumber' strategies as 'must-renumber'"}, conf.must_renumber);
    f({"--fullwatchconseveryn", "Consolidate watchlists fully once every N conflicts. Scheduled during simplification rounds."}, conf.full_watch_consolidate_every_n_confl);
    f({"--strmaxt", "Maximum MBP to spend on distilling long irred cls through watches"}, conf.watch_based_str_time_limitM);
    f({"--implicitmanip", "Subsume and strengthen implicit clauses with each other"}, conf.do_str_sub_implicit);
    f({"--implsubsto", "Timeout (in bogoprop Millions) of implicit subsumption"}, conf.subsume_implicit_time_limitM);
    f({"--implstrto", "Timeout (in bogoprop Millions) of implicit strengthening"}, conf.distill_implicit_with_implicit_time_limitM);
    f({"--cardfind", "Find cardinality constraints"}, conf.do_find_card);
    f({"--sync", "Sync threads every N conflicts"}, conf.sync_every_confl);
    f({"--printtimes", "Print time it took for each simplification run. If set to 0, logs are easier to compare"}, conf.do_print_times);
    f({"--maxsccdepth", "The maximum for scc search depth"}, conf.max_scc_depth);
    f({"--maxmatrixrows", "Set maximum no. of rows for gaussian matrix. Too large matrices"
        " should be discarded for reasons of efficiency", true}, conf.gaussconf.max_matrix_rows);
    f({"--maxmatrixcols", "Set maximum no. of columns for gaussian matrix. Too large matrices"
        " should be discarded for reasons of efficiency", true}, conf.gaussconf.max_matrix_columns);
    f({"--autodisablegauss", "Automatically disable gauss when performing badly", true}, conf.gaussconf.autodisable);
    f({"--minmatrixrows", "Set minimum no. of rows for gaussian matrix. Normally, too small"
        " matrices are discarded for reasons of efficiency", true}, conf.gaussconf.min_matrix_rows);
    f({"--maxnummatrices", "Maximum number of matrices to treat.", true}, conf.gaussconf.max_num_matrices);
    f({"--gaussusefulcutoff", "Turn off Gauss if less than this many usefulness ratio is recorded", true}, conf.gaussconf.min_usefulness_cutoff);
    f({"--gaussmincalls", "Only consider disabling a matrix after this many Gauss calls", true}, conf.gaussconf.autodisable_min_calls);
    f({"--gausscheckevery", "Check whether to disable a matrix every this many conflicts", true}, conf.gaussconf.autodisable_check_every);
}

// Throws std::invalid_argument on a bad combination of options
inline void check_conf(SolverConf& conf) {
    if (conf.walkmineff > conf.walkmaxeff)
        throw std::invalid_argument("'--walkmineff' must not be above '--walkmaxeff'");
    if (conf.max_xor_to_find > MAX_XOR_RECOVER_SIZE)
        throw std::invalid_argument("'--maxxorsize' cannot be larger than "
            + std::to_string(MAX_XOR_RECOVER_SIZE));
}

}
