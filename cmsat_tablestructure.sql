DROP TABLE IF EXISTS `tags`;
CREATE TABLE `tags` (
  `name` varchar(500) NOT NULL,
  `val` varchar(500) NOT NULL
);

DROP TABLE IF EXISTS `timepassed`;
CREATE TABLE `timepassed` (
  `simplifications` bigint(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `runtime` float NOT NULL,
  `name` varchar(200) NOT NULL,
  `elapsed` float NOT NULL,
  `timeout` int(20) DEFAULT NULL,
  `percenttimeremain` float DEFAULT NULL
);

DROP TABLE IF EXISTS `memused`;
CREATE TABLE `memused` (
  `simplifications` bigint(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `runtime` float NOT NULL,
  `name` varchar(200) NOT NULL,
  `MB` int(20) NOT NULL
);

DROP TABLE IF EXISTS `solverRun`;
CREATE TABLE `solverRun` (
  `runtime` float NOT NULL,
  `gitrev` varchar(100) NOT NULL
);

DROP TABLE IF EXISTS `startup`;
CREATE TABLE `startup` (
  `startTime` datetime NOT NULL
);

DROP TABLE IF EXISTS `finishup`;
CREATE TABLE `finishup` (
  `endTime` datetime NOT NULL,
  `status` varchar(255) NOT NULL
);

-- TODO Add more features here!
DROP TABLE IF EXISTS `reduceDB`;
CREATE TABLE `reduceDB` (
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `latest_satzilla_feature_calc` int(20) NOT NULL,
  `cur_restart_type` varchar(6) NOT NULL,
  `runtime` float NOT NULL,

  `clauseID` int(20) NOT NULL,
  `dump_no` int(20) NOT NULL,
  `conflicts_made` bigint(20) NOT NULL,
  `propagations_made` bigint(20) NOT NULL,
  `sum_propagations_made` bigint(20) NOT NULL,
  `clause_looked_at` bigint(20) NOT NULL,
  `used_for_uip_creation` bigint(20) NOT NULL,
  `last_touched_diff` bigint(20) NOT NULL,
  `activity_rel` float(20) NOT NULL,
  `locked` int(20) NOT NULL,
  `in_xor` int(20) NOT NULL,
  `glue` int(20) NOT NULL,
  `size` int(20) NOT NULL,
  `ttl` int(20) NOT NULL,
  `is_ternary_resolvent` int(20) NOT NULL,
  `act_ranking_top_10` int(20) NOT NULL,
  `act_ranking` int(20) NOT NULL,
  `tot_cls_in_db` int(20) NOT NULL,
  `sum_uip1_used`  int(20) NOT NULL
);

DROP TABLE IF EXISTS `restart`;
CREATE TABLE `restart` (
  `restartID` int (20) NOT NULL,
  `clauseID` bigint(20) DEFAULT NULL,
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `conflicts_this_restart` int(20) NOT NULL,
  `latest_satzilla_feature_calc` int(20) NOT NULL,
  `runtime` float NOT NULL,

  `numIrredBins` int(20) NOT NULL,
  `numIrredLongs` int(20) NOT NULL,
  `numRedBins` int(20) NOT NULL,
  `numRedLongs` int(20) NOT NULL,

  `numIrredLits` bigint(20) NOT NULL,
  `numredLits` bigint(20) NOT NULL,

  -- conflict stats
  `glue` float,
  `glueSD` float,
  `glueMin` int(20),
  `glueMax` int(20),

  `size` float,
  `sizeSD` float,
  `sizeMin` int(20),
  `sizeMax` int(20),

  `resolutions` float,
  `resolutionsSD` float,
  `resolutionsMin` int(20),
  `resolutionsMax` int(20),

  -- search stats
  `branchDepth` float,
  `branchDepthSD` float,
  `branchDepthMin` int(20),
  `branchDepthMax` int(20),

  `branchDepthDelta` float,
  `branchDepthDeltaSD` float,
  `branchDepthDeltaMin` int(20),
  `branchDepthDeltaMax` int(20),

  `trailDepth` float,
  `trailDepthSD` float,
  `trailDepthMin` int(20),
  `trailDepthMax` int(20),

  `trailDepthDelta` float,
  `trailDepthDeltaSD` float,
  `trailDepthDeltaMin` int(20),
  `trailDepthDeltaMax` int(20),

  `propBinIrred` bigint(20) NOT NULL,
  `propBinRed` bigint(20) NOT NULL,
  `propLongIrred` bigint(20) NOT NULL,
  `propLongRed` bigint(20) NOT NULL,

  `conflBinIrred` bigint(20) NOT NULL,
  `conflBinRed` bigint(20) NOT NULL,
  `conflLongIrred` bigint(20) NOT NULL,
  `conflLongRed` bigint(20) NOT NULL,

  `learntUnits` int(20) NOT NULL,
  `learntBins` int(20) NOT NULL,
  `learntLongs` int(20) NOT NULL,

  `resolBinIrred` bigint(20) NOT NULL,
  `resolBinRed` bigint(20) NOT NULL,
  `resolLIrred` bigint(20) NOT NULL,
  `resolLRed` bigint(20) NOT NULL,

  `propagations` bigint(20) NOT NULL,
  `decisions` bigint(20) NOT NULL,

  `flipped` bigint(20) NOT NULL,
  `varSetPos` bigint(20) NOT NULL,
  `varSetNeg` bigint(20) NOT NULL,
  `free` int(20) NOT NULL,
  `replaced` int(20) NOT NULL,
  `eliminated` int(20) NOT NULL,
  `set` int(20) NOT NULL,

  `branch_strategy` int NOT NULL,
  `restart_type` int NOT NULL
);

DROP TABLE IF EXISTS `restart_dat_for_var`;
DROP TABLE IF EXISTS `restart_dat_for_cl`;
CREATE TABLE `restart_dat_for_var` AS SELECT * FROM `restart` WHERE 0;
CREATE TABLE `restart_dat_for_cl` AS SELECT * FROM `restart` WHERE 0;


DROP TABLE IF EXISTS `clause_stats`;
CREATE TABLE `clause_stats` (
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `prev_restart` bigint(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `latest_satzilla_feature_calc` int(20) NOT NULL,
  `clauseID` bigint(20) NOT NULL,
  `restartID` int(20) NOT NULL,

  `orig_glue` int(20) NOT NULL,
  `glue_before_minim` int(20) NOT NULL,
  `orig_size` int(20) NOT NULL,
  `conflicts_this_restart` bigint(20) NOT NULL,
  `num_overlap_literals` int(20) NOT NULL,
  `num_antecedents` int(20) NOT NULL,
  `num_total_lits_antecedents` int(20) NOT NULL,
  `is_decision` int(20) NOT NULL,

  `backtrack_level` int(20) NOT NULL,
  `decision_level` int(20) NOT NULL,
  `decision_level_pre1` int(20) NOT NULL,
  `decision_level_pre2` int(20) NOT NULL,
  `trail_depth_level` int(20) NOT NULL,
  `cur_restart_type` varchar(6) NOT NULL,

  `atedecents_binIrred` int(20) NOT NULL,
  `atedecents_binRed` int(20) NOT NULL,
  `atedecents_longIrred` int(20) NOT NULL,
  `atedecents_longRed` int(20) NOT NULL,

  `antecedents_glue_long_reds_avg` float,
  `antecedents_glue_long_reds_var` float,
  `antecedents_glue_long_reds_min` int(20),
  `antecedents_glue_long_reds_max` int(20),

  `antecedents_long_red_age_avg` float,
  `antecedents_long_red_age_var` float,
  `antecedents_long_red_age_min` bigint(20),
  `antecedents_long_red_age_max` bigint(20),

  `decision_level_hist` float,
  `backtrack_level_hist_lt` float,
  `trail_depth_level_hist` float,
  `size_hist` float,
  `glue_hist` float,
  `num_resolutions_hist_lt` float,

  `antec_sum_size_hist` float,
  `antec_overlap_hist` float,

  `branch_depth_hist_queue` float NOT NULL,
  `trail_depth_hist` float NOT NULL,
  `trail_depth_hist_longer` float NOT NULL,
  `num_resolutions_hist` float,
  `confl_size_hist` float,
  `trail_depth_delta_hist` float,
  `backtrack_level_hist` float,
  `glue_hist_queue` float NOT NULL,
  `glue_hist_long` float
);

DROP TABLE IF EXISTS `satzilla_features`;
CREATE TABLE `satzilla_features` (
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `latest_satzilla_feature_calc` int(20) NOT NULL,

  `numVars` int(20) NOT NULL,
  `numClauses` int(20) NOT NULL,
  `var_cl_ratio` double NOT NULL,

  -- Clause distribution
  `binary` double NOT NULL,
  `horn` double NOT NULL,
  `horn_mean` double NOT NULL,
  `horn_std` double NOT NULL,
  `horn_min` double NOT NULL,
  `horn_max` double NOT NULL,
  `horn_spread` double NOT NULL,

  `vcg_var_mean` double NOT NULL,
  `vcg_var_std` double NOT NULL,
  `vcg_var_min` double NOT NULL,
  `vcg_var_max` double NOT NULL,
  `vcg_var_spread` double NOT NULL,

  `vcg_cls_mean` double NOT NULL,
  `vcg_cls_std` double NOT NULL,
  `vcg_cls_min` double NOT NULL,
  `vcg_cls_max` double NOT NULL,
  `vcg_cls_spread` double NOT NULL,

  `pnr_var_mean` double NOT NULL,
  `pnr_var_std` double NOT NULL,
  `pnr_var_min` double NOT NULL,
  `pnr_var_max` double NOT NULL,
  `pnr_var_spread` double NOT NULL,

  `pnr_cls_mean` double NOT NULL,
  `pnr_cls_std` double NOT NULL,
  `pnr_cls_min` double NOT NULL,
  `pnr_cls_max` double NOT NULL,
  `pnr_cls_spread` double NOT NULL,

  -- Conflict clauses
  `avg_confl_size` double NOT NULL,
  `confl_size_min` double NOT NULL,
  `confl_size_max` double NOT NULL,
  `avg_confl_glue` double NOT NULL,
  `confl_glue_min` double NOT NULL,
  `confl_glue_max` double NOT NULL,
  `avg_num_resolutions` double NOT NULL,
  `num_resolutions_min` double NOT NULL,
  `num_resolutions_max` double NOT NULL,
  `learnt_bins_per_confl` double NOT NULL,

  -- Search
  `avg_branch_depth` double NOT NULL,
  `branch_depth_min` double NOT NULL,
  `branch_depth_max` double NOT NULL,
  `avg_trail_depth_delta` double NOT NULL,
  `trail_depth_delta_min` double NOT NULL,
  `trail_depth_delta_max` double NOT NULL,
  `avg_branch_depth_delta` double NOT NULL,
  `props_per_confl` double NOT NULL,
  `confl_per_restart` double NOT NULL,
  `decisions_per_conflict` double NOT NULL,

    -- clause distributions
  `red_glue_distr_mean` double NOT NULL,
  `red_glue_distr_var` double NOT NULL,
  `red_size_distr_mean` double NOT NULL,
  `red_size_distr_var` double NOT NULL,
  `red_activity_distr_mean` double NOT NULL,
  `red_activity_distr_var` double NOT NULL,

  `irred_glue_distr_mean` double NOT NULL,
  `irred_glue_distr_var` double NOT NULL,
  `irred_size_distr_mean` double NOT NULL,
  `irred_size_distr_var` double NOT NULL,
  `irred_activity_distr_mean` double NOT NULL,
  `irred_activity_distr_var` double NOT NULL
);

DROP TABLE IF EXISTS `used_clauses`;
create table `used_clauses` (
    `clauseID` bigint(20) NOT NULL
    , `used_at` bigint(20) NOT NULL
);

DROP TABLE IF EXISTS `var_data_fintime`;
create table `var_data_fintime` (
    `var`                                              int(20) NOT NULL
    , `sumConflicts_at_picktime`                       int(20) NOT NULL
    , `rel_activity_at_fintime`                        double NOT NULL

    , `inside_conflict_clause_at_fintime`              int(20) NOT NULL
    , `inside_conflict_clause_antecedents_at_fintime`  int(20) NOT NULL
    , `inside_conflict_clause_glue_at_fintime`         int(20) NOT NULL

    , `sumDecisions_at_fintime`                        int(20) NOT NULL
    , `sumConflicts_at_fintime`                        int(20) NOT NULL
    , `sumPropagations_at_fintime`                     int(20) NOT NULL
    , `sumAntecedents_at_fintime`                      int(20) NOT NULL
    , `sumAntecedentsLits_at_fintime`                  int(20) NOT NULL
    , `sumConflictClauseLits_at_fintime`               int(20) NOT NULL
    , `sumDecisionBasedCl_at_fintime`                  int(20) NOT NULL
    , `sumClLBD_at_fintime`                            int(20) NOT NULL
    , `sumClSize_at_fintime`                           int(20) NOT NULL
);

DROP TABLE IF EXISTS `var_data_picktime`;
create table `var_data_picktime` (
    `var`                                                       int(20) NOT NULL
    , `dec_depth`                                               int(20) NOT NULL
    , `rel_activity_at_picktime`                                double  NOT NULL
    , `latest_vardist_feature_calc`                             int(20) NOT NULL

    , `inside_conflict_clause_at_picktime`                      int(20) NOT NULL
    , `inside_conflict_clause_antecedents_at_picktime`          int(20) NOT NULL
    , `inside_conflict_clause_glue_at_picktime`                 int(20) NOT NULL

    , `inside_conflict_clause_during_at_picktime`               int(20) NOT NULL
    , `inside_conflict_clause_antecedents_during_at_picktime`   int(20) NOT NULL
    , `inside_conflict_clause_glue_during_at_picktime`          int(20) NOT NULL

    , `num_decided`                                             int(20) NOT NULL
    , `num_decided_pos`                                         int(20) NOT NULL
    , `num_propagated`                                          int(20) NOT NULL
    , `num_propagated_pos`                                      int(20) NOT NULL

    , `conflicts_since_in_1uip`                                 int(20) NOT NULL
    , `conflicts_since_decided`                                 int(20) NOT NULL
    , `conflicts_since_propagated`                              int(20) NOT NULL
    , `conflicts_since_canceled`                                int(20) NOT NULL

    , `sumDecisions_at_picktime`                                int(20) NOT NULL
    , `sumConflicts_at_picktime`                                int(20) NOT NULL
    , `sumPropagations_at_picktime`                             int(20) NOT NULL
    , `sumAntecedents_at_picktime`                              int(20) NOT NULL
    , `sumAntecedentsLits_at_picktime`                          int(20) NOT NULL
    , `sumConflictClauseLits_at_picktime`                       int(20) NOT NULL
    , `sumDecisionBasedCl_at_picktime`                          int(20) NOT NULL
    , `sumClLBD_at_picktime`                                    int(20) NOT NULL
    , `sumClSize_at_picktime`                                   int(20) NOT NULL

    , `sumConflicts_below_during`                               int(20) NOT NULL
    , `sumDecisions_below_during`                               int(20) NOT NULL
    , `sumPropagations_below_during`                            int(20) NOT NULL
    , `sumAntecedents_below_during`                             int(20) NOT NULL
    , `sumAntecedentsLits_below_during`                         int(20) NOT NULL
    , `sumConflictClauseLits_below_during`                      int(20) NOT NULL
    , `sumDecisionBasedCl_below_during`                         int(20) NOT NULL
    , `sumClLBD_below_during`                                   int(20) NOT NULL
    , `sumClSize_below_during`                                  int(20) NOT NULL

    , flipped_confs_ago                                         int(20) NOT NULL
);

DROP TABLE IF EXISTS `dec_var_clid`;
create table `dec_var_clid` (
    `var` int(20) NOT NULL
    , `sumConflicts_at_picktime` bigint(20) NOT NULL
    , `clauseID` int(2) DEFAULT NULL
);

DROP TABLE IF EXISTS `var_data_use`;
create table `var_data_use` (
    `var` int(20) NOT NULL
    , `sumConflicts_at_picktime` bigint(20) NOT NULL

    , `cls_marked` int(2) DEFAULT NULL
    , `useful_clauses` int(20) DEFAULT NULL
    , `useful_clauses_used` int(20) DEFAULT NULL
);

DROP TABLE IF EXISTS `cl_last_in_solver`;
create table `cl_last_in_solver` (
  `conflicts` bigint(20) NOT NULL
  , `clauseID` bigint(20) NOT NULL
);


DROP TABLE IF EXISTS `var_dist`;
create table `var_dist` (
  `var` bigint(20) NOT NULL
  , `latest_vardist_feature_calc` bigint(20) NOT NULL
  , `conflicts` bigint(20) NOT NULL

  , `num_irred_long_cls`                     bigint(20) NOT NULL
  , `num_red_long_cls`                     bigint(20) NOT NULL
  , `num_irred_bin_cls`                     bigint(20) NOT NULL
  , `num_red_bin_cls`                     bigint(20) NOT NULL

  , `red_num_times_in_bin_clause`                     bigint(20) NOT NULL
  , `red_num_times_in_long_clause`                    bigint(20) NOT NULL
  , `red_satisfies_cl`                                bigint(20) NOT NULL
  , `red_falsifies_cl`                                bigint(20) NOT NULL
  , `red_tot_num_lit_of_bin_it_appears_in`            bigint(20) NOT NULL
  , `red_tot_num_lit_of_long_cls_it_appears_in`       bigint(20) NOT NULL
  , `red_sum_var_act_of_cls`                          double NOT NULL

  , `irred_num_times_in_bin_clause`                   bigint(20) NOT NULL
  , `irred_num_times_in_long_clause`                  bigint(20) NOT NULL
  , `irred_satisfies_cl`                              bigint(20) NOT NULL
  , `irred_falsifies_cl`                              bigint(20) NOT NULL
  , `irred_tot_num_lit_of_bin_it_appears_in`          bigint(20) NOT NULL
  , `irred_tot_num_lit_of_long_cls_it_appears_in`     bigint(20) NOT NULL
  , `irred_sum_var_act_of_cls`                        double NOT NULL

  , `tot_act_long_red_cls`                            bigint(20) NOT NULL
);
