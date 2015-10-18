
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

#ifndef _FEATURES_TO_RECONF_H_
#define _FEATURES_TO_RECONF_H_

#include "solvefeatures.h"
#include <iostream>
using std::cout;
using std::endl;

namespace CMSat {

double get_score1(const SolveFeatures& feat, const int verb);
double get_score3(const SolveFeatures& feat, const int verb);
double get_score6(const SolveFeatures& feat, const int verb);
double get_score7(const SolveFeatures& feat, const int verb);
double get_score12(const SolveFeatures& feat, const int verb);
double get_score13(const SolveFeatures& feat, const int verb);
double get_score14(const SolveFeatures& feat, const int verb);

int get_reconf_from_features(const SolveFeatures& feat, const int verb)
{
	double best_score = 0.0;
	int best_val = 0;
	double score;


	score = get_score1(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 1 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 1;
	}


	score = get_score3(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 3 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 3;
	}


	score = get_score6(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 6 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 6;
	}


	score = get_score7(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 7 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 7;
	}


	score = get_score12(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 12 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 12;
	}


	score = get_score13(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 13 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 13;
	}


	score = get_score14(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 14 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 14;
	}


	if (verb >= 2)
		cout << "c Winning reconf is " << best_val << endl;
	return best_val;
}



double get_score1(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.avg_branch_depth_delta > 1.06600))
	{
		total_plus += 0.792;
	}
	if ((feat.avg_trail_depth_delta < 2540.09200) &&
		(feat.avg_branch_depth_delta < 1.06600))
	{
		total_neg += 0.944;
	}
	if ((feat.var_cl_ratio < 0.16700) &&
		(feat.confl_per_restart < 115.15000))
	{
		total_neg += 0.941;
	}
	if ((feat.trinary > 0.57000) &&
		(feat.vcg_var_std > 1.77700) &&
		(feat.pnr_cls_mean > 0.49000) &&
		(feat.avg_branch_depth_delta > 1.06600))
	{
		total_neg += 0.917;
	}
	if ((feat.vcg_cls_max > 0.02300) &&
		(feat.vcg_cls_max < 0.02700) &&
		(feat.branch_depth_min > 3.00000) &&
		(feat.irred_cl_distrib.glue_distr_mean > 536366820.00000))
	{
		total_neg += 0.900;
	}
	if ((feat.var_cl_ratio > 0.17200) &&
		(feat.vcg_var_std < 1.06100) &&
		(feat.avg_num_resolutions > 334.38400))
	{
		total_neg += 0.857;
	}
	if ((feat.vcg_cls_max > 0.02300) &&
		(feat.branch_depth_min > 3.00000) &&
		(feat.branch_depth_max > 42.00000) &&
		(feat.irred_cl_distrib.glue_distr_mean > 536366820.00000))
	{
		total_neg += 0.900;
	}
	if ((feat.pnr_var_std > 0.32700) &&
		(feat.confl_per_restart < 115.15000))
	{
		total_neg += 0.909;
	}
	if ((feat.trinary < 0.57000) &&
		(feat.vcg_var_mean < 0.00000) &&
		(feat.pnr_var_mean > 0.47800) &&
		(feat.branch_depth_min > 26.00000) &&
		(feat.red_cl_distrib.glue_distr_var > 96.81100))
	{
		total_neg += 0.800;
	}
	if ((feat.branch_depth_min < 3.00000))
	{
		total_plus += 0.957;
	}
	// num_rules: 10
	// rule_no: 10
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score3(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.learnt_tris_per_confl < 0.17600))
	{
		total_plus += 0.744;
	}
	if ((feat.vcg_var_mean > 0.00600) &&
		(feat.pnr_var_std < 0.34100))
	{
		total_neg += 0.958;
	}
	if ((feat.numClauses < 215151.00000) &&
		(feat.binary < 0.02400))
	{
		total_neg += 0.952;
	}
	if ((feat.horn > 0.41500) &&
		(feat.vcg_cls_max < 0.00300) &&
		(feat.learnt_tris_per_confl < 0.07400) &&
		(feat.irred_cl_distrib.size_distr_mean < 4.24000))
	{
		total_neg += 0.875;
	}
	if ((feat.binary > 0.02400) &&
		(feat.horn > 0.98700) &&
		(feat.pnr_var_min < 0.10000) &&
		(feat.learnt_bins_per_confl < 0.00100) &&
		(feat.branch_depth_max > 34.00000))
	{
		total_neg += 0.917;
	}
	if ((feat.var_cl_ratio > 0.15400) &&
		(feat.horn > 0.41500) &&
		(feat.vcg_cls_max < 0.00300) &&
		(feat.pnr_var_mean > 0.53300))
	{
		total_neg += 0.900;
	}
	if ((feat.learnt_tris_per_confl > 0.17600))
	{
		total_neg += 0.750;
	}
	if ((feat.vcg_cls_max < 0.00300) &&
		(feat.avg_branch_depth > 152.16701) &&
		(feat.avg_branch_depth_delta > 10.26400) &&
		(feat.props_per_confl < 0.00100))
	{
		total_neg += 0.750;
	}
	if ((feat.trinary > 0.80200) &&
		(feat.horn < 0.98700))
	{
		total_neg += 0.833;
	}
	// num_rules: 9
	// rule_no: 9
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score6(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.decisions_per_conflict > 1.13400))
	{
		total_plus += 0.756;
	}
	if ((feat.decisions_per_conflict < 1.13400))
	{
		total_neg += 0.875;
	}
	if ((feat.binary < 0.02400) &&
		(feat.vcg_cls_std > 0.22200))
	{
		total_neg += 0.944;
	}
	if ((feat.numVars > 15330.00000) &&
		(feat.vcg_var_std < 0.80400) &&
		(feat.vcg_cls_max < 0.00000) &&
		(feat.pnr_var_std < 0.26400) &&
		(feat.avg_branch_depth_delta > 1.17200))
	{
		total_neg += 0.875;
	}
	if ((feat.trinary > 0.71800) &&
		(feat.avg_branch_depth > 15.76100) &&
		(feat.avg_branch_depth < 21.11900))
	{
		total_neg += 0.917;
	}
	if ((feat.binary > 0.02400) &&
		(feat.vcg_cls_spread < 0.00200) &&
		(feat.pnr_var_std < 0.26400) &&
		(feat.num_resolutions_max < 409.00000))
	{
		total_neg += 0.875;
	}
	// num_rules: 6
	// rule_no: 6
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score7(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.irred_cl_distrib.size_distr_mean < 27.99100))
	{
		total_plus += 0.595;
	}
	if ((feat.numVars < 72190.00000) &&
		(feat.vcg_cls_spread < 0.02300) &&
		(feat.avg_confl_glue < 12.97600) &&
		(feat.learnt_bins_per_confl < 0.01300) &&
		(feat.branch_depth_max > 39.00000) &&
		(feat.red_cl_distrib.size_distr_var > 37.34500) &&
		(feat.num_xors_found_last < 2768.00000))
	{
		total_neg += 0.929;
	}
	if ((feat.irred_cl_distrib.size_distr_mean > 27.99100))
	{
		total_neg += 0.920;
	}
	if ((feat.var_cl_ratio < 0.24600) &&
		(feat.learnt_bins_per_confl > 0.01300) &&
		(feat.learnt_tris_per_confl < 0.08800) &&
		(feat.trail_depth_delta_min > 10.00000) &&
		(feat.num_xors_found_last < 16634.00000))
	{
		total_neg += 0.957;
	}
	if ((feat.branch_depth_min < 784.00000) &&
		(feat.decisions_per_conflict > 20.13400))
	{
		total_neg += 0.923;
	}
	if ((feat.avg_num_resolutions > 26.26600) &&
		(feat.red_cl_distrib.size_distr_var < 37.34500))
	{
		total_neg += 0.905;
	}
	if ((feat.vcg_cls_spread < 0.02300) &&
		(feat.pnr_var_std > 0.10400) &&
		(feat.confl_glue_min > 1.00000) &&
		(feat.avg_branch_depth > 13.38100) &&
		(feat.confl_per_restart > 150.00301) &&
		(feat.confl_per_restart < 1513.53200) &&
		(feat.red_cl_distrib.size_distr_var > 37.34500))
	{
		total_neg += 0.857;
	}
	if ((feat.learnt_bins_per_confl > 0.01300) &&
		(feat.learnt_tris_per_confl > 0.02900) &&
		(feat.learnt_tris_per_confl < 0.08800) &&
		(feat.avg_branch_depth > 13.38100) &&
		(feat.num_xors_found_last < 16634.00000))
	{
		total_neg += 0.929;
	}
	if ((feat.numVars > 44442.00000) &&
		(feat.numVars < 72190.00000) &&
		(feat.avg_branch_depth > 13.38100) &&
		(feat.confl_per_restart > 150.00301))
	{
		total_neg += 0.840;
	}
	if ((feat.numVars < 72190.00000) &&
		(feat.numClauses > 1330461.00000))
	{
		total_neg += 0.889;
	}
	// num_rules: 10
	// rule_no: 10
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score12(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.binary > 0.02500) &&
		(feat.trinary < 0.74500) &&
		(feat.pnr_var_mean < 0.54200) &&
		(feat.branch_depth_min < 251.00000) &&
		(feat.decisions_per_conflict > 1.12700) &&
		(feat.irred_cl_distrib.size_distr_mean > 4.12900))
	{
		total_plus += 0.920;
	}
	if ((feat.avg_confl_glue < 12.69700))
	{
		total_plus += 0.808;
	}
	if ((feat.avg_confl_glue > 12.69700) &&
		(feat.decisions_per_conflict < 1.12700))
	{
		total_neg += 0.947;
	}
	if ((feat.binary < 0.02500) &&
		(feat.vcg_cls_std > 0.22200))
	{
		total_neg += 0.895;
	}
	if ((feat.trinary > 0.74500))
	{
		total_neg += 0.667;
	}
	if ((feat.vcg_cls_max < 0.01200) &&
		(feat.branch_depth_min > 251.00000) &&
		(feat.trail_depth_delta_min > 4.00000))
	{
		total_neg += 0.824;
	}
	if ((feat.var_cl_ratio > 0.11900) &&
		(feat.branch_depth_min < 5.00000) &&
		(feat.irred_cl_distrib.size_distr_mean < 4.12900))
	{
		total_neg += 0.917;
	}
	if ((feat.var_cl_ratio > 0.11900) &&
		(feat.irred_cl_distrib.size_distr_mean < 4.12900) &&
		(feat.red_cl_distrib.glue_distr_mean < 5.29200) &&
		(feat.red_cl_distrib.glue_distr_var > 6.69200))
	{
		total_neg += 0.917;
	}
	if ((feat.pnr_var_mean > 0.54200) &&
		(feat.irred_cl_distrib.size_distr_mean > 5.86000))
	{
		total_neg += 0.889;
	}
	if ((feat.binary > 0.02500) &&
		(feat.trinary < 0.74500) &&
		(feat.branch_depth_min > 5.00000) &&
		(feat.branch_depth_min < 251.00000) &&
		(feat.decisions_per_conflict > 1.12700) &&
		(feat.red_cl_distrib.glue_distr_mean > 5.29200))
	{
		total_plus += 0.853;
	}
	// num_rules: 10
	// rule_no: 10
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score13(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.confl_per_restart > 119.49100))
	{
		total_plus += 0.691;
	}
	if ((feat.binary < 0.54100) &&
		(feat.pnr_var_min < 0.00300) &&
		(feat.confl_per_restart < 119.49100))
	{
		total_neg += 0.929;
	}
	if ((feat.horn_mean < 0.00000) &&
		(feat.pnr_cls_std > 1.67700) &&
		(feat.confl_glue_max > 196.00000))
	{
		total_neg += 0.952;
	}
	if ((feat.vcg_var_std > 2.31100) &&
		(feat.pnr_var_mean > 0.45300) &&
		(feat.pnr_var_std < 0.32600) &&
		(feat.avg_branch_depth > 16.84300) &&
		(feat.confl_per_restart > 119.49100))
	{
		total_neg += 0.882;
	}
	if ((feat.trinary > 0.38600) &&
		(feat.pnr_var_std < 0.32600) &&
		(feat.avg_branch_depth > 16.84300) &&
		(feat.trail_depth_delta_min < 3.00000) &&
		(feat.red_cl_distrib.glue_distr_var < 25.42400))
	{
		total_neg += 0.923;
	}
	if ((feat.trinary > 0.38600) &&
		(feat.pnr_var_std < 0.32600) &&
		(feat.branch_depth_min > 42.00000) &&
		(feat.branch_depth_min < 70.00000) &&
		(feat.trail_depth_delta_min < 28.00000))
	{
		total_neg += 0.846;
	}
	if ((feat.pnr_var_std < 0.32600) &&
		(feat.confl_glue_max < 41.00000) &&
		(feat.avg_branch_depth > 16.84300))
	{
		total_neg += 0.900;
	}
	if ((feat.pnr_var_std > 0.32600) &&
		(feat.branch_depth_min > 251.00000) &&
		(feat.decisions_per_conflict > 22.81700))
	{
		total_neg += 0.857;
	}
	if ((feat.trinary > 0.38600) &&
		(feat.pnr_var_std < 0.32600) &&
		(feat.trail_depth_delta_min > 15.00000) &&
		(feat.trail_depth_delta_min < 28.00000))
	{
		total_neg += 0.875;
	}
	if ((feat.trinary > 0.38600) &&
		(feat.pnr_var_std < 0.32600) &&
		(feat.branch_depth_min < 70.00000) &&
		(feat.trail_depth_delta_min < 3.00000) &&
		(feat.red_cl_distrib.glue_distr_var > 36.69400))
	{
		total_neg += 0.900;
	}
	if ((feat.numClauses > 1446546.00000) &&
		(feat.pnr_cls_std > 1.67700))
	{
		total_neg += 0.938;
	}
	if ((feat.vcg_var_std < 2.31100) &&
		(feat.pnr_var_std < 0.32600) &&
		(feat.branch_depth_min > 226.00000))
	{
		total_neg += 0.769;
	}
	// num_rules: 12
	// rule_no: 12
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score14(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.binary > 0.05000))
	{
		total_plus += 0.735;
	}
	if ((feat.numVars < 411125.00000) &&
		(feat.numClauses > 1292650.00000) &&
		(feat.vcg_cls_max < 0.00600) &&
		(feat.learnt_tris_per_confl > 0.00100))
	{
		total_neg += 0.905;
	}
	if ((feat.binary < 0.05000) &&
		(feat.pnr_var_min < 0.16000) &&
		(feat.pnr_cls_std > 0.43800) &&
		(feat.branch_depth_min < 188.00000) &&
		(feat.irred_cl_distrib.glue_distr_mean < 536869820.00000))
	{
		total_neg += 0.941;
	}
	if ((feat.pnr_var_std < 0.20700) &&
		(feat.pnr_cls_std > 0.43800) &&
		(feat.learnt_tris_per_confl > 0.01600) &&
		(feat.trail_depth_delta_min < 3.00000))
	{
		total_neg += 0.933;
	}
	if ((feat.pnr_cls_std < 0.43800))
	{
		total_plus += 0.886;
	}
	if ((feat.pnr_var_mean > 0.53000) &&
		(feat.learnt_tris_per_confl > 0.00200) &&
		(feat.irred_cl_distrib.size_distr_var > 8.55100))
	{
		total_neg += 0.917;
	}
	if ((feat.binary > 0.05000) &&
		(feat.pnr_var_mean > 0.48200) &&
		(feat.pnr_var_std > 0.20700) &&
		(feat.avg_branch_depth_delta > 8.00000))
	{
		total_neg += 0.889;
	}
	if ((feat.vcg_cls_spread > 0.00000) &&
		(feat.pnr_cls_mean < 0.47500) &&
		(feat.learnt_bins_per_confl > 0.01200) &&
		(feat.trail_depth_delta_max > 1793.00000) &&
		(feat.avg_branch_depth_delta < 8.00000))
	{
		total_neg += 0.900;
	}
	if ((feat.avg_branch_depth_delta > 8.00000) &&
		(feat.avg_branch_depth_delta < 9.46400) &&
		(feat.props_per_confl < 0.00500))
	{
		total_neg += 0.875;
	}
	if ((feat.binary < 0.05000) &&
		(feat.pnr_var_min < 0.16000) &&
		(feat.pnr_cls_std > 0.43800) &&
		(feat.num_resolutions_max < 385.00000) &&
		(feat.irred_cl_distrib.glue_distr_mean < 536869820.00000))
	{
		total_neg += 0.938;
	}
	if ((feat.pnr_var_std < 0.20700) &&
		(feat.learnt_tris_per_confl < 0.01600))
	{
		total_plus += 0.889;
	}
	if ((feat.binary > 0.05000) &&
		(feat.pnr_var_std < 0.20700) &&
		(feat.pnr_cls_mean > 0.51700) &&
		(feat.pnr_cls_std > 0.43800))
	{
		total_neg += 0.889;
	}
	// num_rules: 12
	// rule_no: 12
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


} //end namespace

#endif //_FEATURES_TO_RECONF_H_


