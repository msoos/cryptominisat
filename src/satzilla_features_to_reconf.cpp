/******************************************
Copyright (c) 2016, Mate Soos

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

#include "satzilla_features.h"
#include <iostream>
using std::cout;
using std::endl;

namespace CMSat {

double get_score0(const SatZillaFeatures& satzilla_feat, const int verb);
double get_score4(const SatZillaFeatures& satzilla_feat, const int verb);
double get_score6(const SatZillaFeatures& satzilla_feat, const int verb);
double get_score7(const SatZillaFeatures& satzilla_feat, const int verb);
double get_score12(const SatZillaFeatures& satzilla_feat, const int verb);
double get_score16(const SatZillaFeatures& satzilla_feat, const int verb);
double get_score17(const SatZillaFeatures& satzilla_feat, const int verb);

int get_reconf_from_satzilla_features(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double best_score = 0.0;
	int best_val = 0;
	double score;


	score = get_score0(satzilla_feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 0 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 0;
	}


	score = get_score4(satzilla_feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 4 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 4;
	}


	score = get_score6(satzilla_feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 6 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 6;
	}


	score = get_score7(satzilla_feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 7 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 7;
	}


	score = get_score12(satzilla_feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 12 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 12;
	}


	score = get_score16(satzilla_feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 16 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 16;
	}


	score = get_score17(satzilla_feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 17 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 17;
	}


	if (verb >= 2)
		cout << "c Winning reconf is " << best_val << endl;
	return best_val;
}



double get_score0(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((satzilla_feat.confl_per_restart < 330.10001))
	{
		total_plus += 0.615;
	}
	if ((satzilla_feat.vcg_cls_min < 0.00000) &&
		(satzilla_feat.pnr_var_max > 0.60000) &&
		(satzilla_feat.pnr_cls_std > 3.10000) &&
		(satzilla_feat.confl_per_restart > 181.80000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var < 0.30000) &&
		(satzilla_feat.red_cl_distrib.activity_distr_var < 87161348000.00000))
	{
		total_neg += 0.920;
	}
	if ((satzilla_feat.numClauses > 24521.00000) &&
		(satzilla_feat.trail_depth_delta_max < 135198.00000) &&
		(satzilla_feat.confl_per_restart > 330.10001))
	{
		total_neg += 0.952;
	}
	if ((satzilla_feat.horn > 0.00000) &&
		(satzilla_feat.pnr_var_mean > 0.40000) &&
		(satzilla_feat.pnr_var_std > 0.50000) &&
		(satzilla_feat.confl_size_max > 108.00000) &&
		(satzilla_feat.trail_depth_delta_max < 208897.00000) &&
		(satzilla_feat.irred_cl_distrib.activity_distr_var < 595761410.00000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var > 0.30000))
	{
		total_neg += 0.952;
	}
	if ((satzilla_feat.binary > 0.10000) &&
		(satzilla_feat.vcg_cls_std < 3.70000) &&
		(satzilla_feat.pnr_var_mean > 0.40000) &&
		(satzilla_feat.avg_confl_size > 15.30000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.irred_cl_distrib.activity_distr_var < 130750880.00000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var > 0.40000))
	{
		total_neg += 0.923;
	}
	if ((satzilla_feat.vcg_cls_std < 0.40000) &&
		(satzilla_feat.confl_size_min > 1.00000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var > 0.30000))
	{
		total_neg += 0.800;
	}
	if ((satzilla_feat.vcg_var_std < 1.30000) &&
		(satzilla_feat.pnr_cls_mean > 0.50000))
	{
		total_neg += 0.889;
	}
	if ((satzilla_feat.numClauses > 3631149.00000) &&
		(satzilla_feat.branch_depth_min > 18.00000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var < 0.30000))
	{
		total_neg += 0.857;
	}
	if ((satzilla_feat.pnr_var_mean > 0.30000) &&
		(satzilla_feat.confl_size_max > 4843.00000) &&
		(satzilla_feat.branch_depth_min > 18.00000))
	{
		total_neg += 0.857;
	}
	if ((satzilla_feat.avg_confl_size < 15.30000))
	{
		total_plus += 0.718;
	}
	if ((satzilla_feat.horn < 0.00000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var > 0.30000))
	{
		total_plus += 0.875;
	}
	// num_rules: 11
	// rule_no: 11
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score4(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((satzilla_feat.confl_size_max > 101.00000))
	{
		total_plus += 0.581;
	}
	if ((satzilla_feat.branch_depth_max < 133.00000))
	{
		total_neg += 0.651;
	}
	if ((satzilla_feat.irred_cl_distrib.glue_distr_mean > 940.00000) &&
		(satzilla_feat.irred_cl_distrib.glue_distr_var > 22169.50000))
	{
		total_neg += 0.971;
	}
	if ((satzilla_feat.vcg_var_max > 0.00000) &&
		(satzilla_feat.pnr_cls_mean < 0.60000) &&
		(satzilla_feat.confl_size_max > 101.00000) &&
		(satzilla_feat.confl_size_max < 303.00000))
	{
		total_plus += 0.947;
	}
	if ((satzilla_feat.vcg_var_std < 0.30000) &&
		(satzilla_feat.confl_size_max > 101.00000) &&
		(satzilla_feat.decisions_per_conflict < 2.60000) &&
		(satzilla_feat.irred_cl_distrib.glue_distr_mean > 998.40002))
	{
		total_plus += 0.864;
	}
	if ((satzilla_feat.numClauses > 252434.00000) &&
		(satzilla_feat.binary < 0.10000) &&
		(satzilla_feat.branch_depth_max > 408.00000) &&
		(satzilla_feat.avg_branch_depth_delta < 8.40000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var > 0.30000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var < 0.40000))
	{
		total_neg += 0.917;
	}
	if ((satzilla_feat.branch_depth_max < 133.00000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var > 0.40000))
	{
		total_plus += 0.909;
	}
	if ((satzilla_feat.confl_size_max < 572.00000) &&
		(satzilla_feat.irred_cl_distrib.glue_distr_var > 22169.50000))
	{
		total_neg += 0.962;
	}
	if ((satzilla_feat.binary < 0.10000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_var > 5.30000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var > 0.30000))
	{
		total_neg += 0.947;
	}
	if ((satzilla_feat.binary > 0.20000) &&
		(satzilla_feat.vcg_var_std < 0.30000) &&
		(satzilla_feat.vcg_var_max < 0.00000) &&
		(satzilla_feat.confl_size_max > 101.00000) &&
		(satzilla_feat.decisions_per_conflict < 2.60000))
	{
		total_plus += 0.923;
	}
	if ((satzilla_feat.vcg_var_max > 0.00000) &&
		(satzilla_feat.confl_size_max > 101.00000) &&
		(satzilla_feat.confl_glue_max < 34.00000))
	{
		total_plus += 0.889;
	}
	if ((satzilla_feat.confl_size_max < 101.00000))
	{
		total_neg += 0.923;
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


double get_score6(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((satzilla_feat.vcg_cls_std < 8.30000))
	{
		total_plus += 0.576;
	}
	if ((satzilla_feat.vcg_cls_std > 8.30000))
	{
		total_neg += 0.889;
	}
	if ((satzilla_feat.pnr_var_mean > 0.40000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.avg_confl_glue < 16.20000) &&
		(satzilla_feat.avg_branch_depth_delta > 1.30000) &&
		(satzilla_feat.decisions_per_conflict < 2.90000))
	{
		total_neg += 0.917;
	}
	if ((satzilla_feat.avg_confl_size < 17.60000) &&
		(satzilla_feat.decisions_per_conflict < 2.90000))
	{
		total_neg += 0.952;
	}
	if ((satzilla_feat.pnr_cls_max > 0.50000) &&
		(satzilla_feat.decisions_per_conflict < 2.90000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean > 5.80000) &&
		(satzilla_feat.red_cl_distrib.activity_distr_mean > 4804.10010))
	{
		total_neg += 0.950;
	}
	if ((satzilla_feat.binary > 0.20000) &&
		(satzilla_feat.pnr_var_max > 0.90000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.avg_confl_glue < 16.20000) &&
		(satzilla_feat.decisions_per_conflict < 2.90000))
	{
		total_neg += 0.944;
	}
	if ((satzilla_feat.decisions_per_conflict < 2.90000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 3.30000))
	{
		total_neg += 0.789;
	}
	if ((satzilla_feat.pnr_cls_mean > 0.50000) &&
		(satzilla_feat.decisions_per_conflict > 2.90000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_var > 4.90000))
	{
		total_neg += 0.875;
	}
	if ((satzilla_feat.avg_trail_depth_delta < 74.00000))
	{
		total_neg += 0.643;
	}
	if ((satzilla_feat.pnr_cls_mean < 0.50000) &&
		(satzilla_feat.avg_confl_size > 16.40000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.avg_confl_glue < 12.40000) &&
		(satzilla_feat.learnt_bins_per_confl < 0.00000) &&
		(satzilla_feat.decisions_per_conflict > 2.90000))
	{
		total_plus += 0.953;
	}
	if ((satzilla_feat.avg_branch_depth > 1243.20000))
	{
		total_neg += 0.800;
	}
	if ((satzilla_feat.vcg_var_spread < 0.00000) &&
		(satzilla_feat.confl_size_min > 1.00000) &&
		(satzilla_feat.branch_depth_max < 32.00000))
	{
		total_neg += 0.867;
	}
	if ((satzilla_feat.avg_branch_depth_delta < 1.00000))
	{
		total_neg += 0.857;
	}
	if ((satzilla_feat.numClauses > 17097.00000) &&
		(satzilla_feat.vcg_var_spread < 0.00000) &&
		(satzilla_feat.pnr_var_max < 0.90000) &&
		(satzilla_feat.pnr_cls_max > 0.50000))
	{
		total_neg += 0.857;
	}
	// num_rules: 14
	// rule_no: 14
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score7(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double default_val = 0.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((satzilla_feat.confl_glue_max > 41.00000))
	{
		total_neg += 0.755;
	}
	if ((satzilla_feat.confl_glue_max < 41.00000))
	{
		total_neg += 0.943;
	}
	if ((satzilla_feat.binary > 0.30000) &&
		(satzilla_feat.branch_depth_max < 316.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 6.90000) &&
		(satzilla_feat.red_cl_distrib.activity_distr_var > 4041287700.00000))
	{
		total_plus += 0.923;
	}
	if ((satzilla_feat.vcg_cls_std < 10.60000) &&
		(satzilla_feat.irred_cl_distrib.glue_distr_mean < 945.70001))
	{
		total_plus += 0.917;
	}
	if ((satzilla_feat.avg_confl_size > 49.50000) &&
		(satzilla_feat.branch_depth_min < 22.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 6.90000))
	{
		total_plus += 0.909;
	}
	if ((satzilla_feat.binary < 0.30000) &&
		(satzilla_feat.vcg_var_std < 1.20000) &&
		(satzilla_feat.confl_size_max > 943.00000) &&
		(satzilla_feat.branch_depth_min > 2.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.60000))
	{
		total_plus += 0.889;
	}
	if ((satzilla_feat.vcg_var_std < 1.20000) &&
		(satzilla_feat.confl_glue_max > 41.00000) &&
		(satzilla_feat.branch_depth_min < 22.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.60000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_var > 2.60000))
	{
		total_plus += 0.800;
	}
	if ((satzilla_feat.pnr_var_std > 0.50000) &&
		(satzilla_feat.confl_glue_max > 41.00000) &&
		(satzilla_feat.trail_depth_delta_min > 2.00000))
	{
		total_plus += 0.889;
	}
	if ((satzilla_feat.pnr_var_mean > 0.50000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 6.90000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_var > 13.70000))
	{
		total_plus += 0.857;
	}
	if ((satzilla_feat.vcg_var_std > 1.20000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.avg_branch_depth > 124.60000) &&
		(satzilla_feat.branch_depth_min < 22.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.60000))
	{
		total_plus += 0.800;
	}
	if ((satzilla_feat.avg_confl_size > 144.80000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 6.90000) &&
		(satzilla_feat.red_cl_distrib.activity_distr_var > 4041287700.00000))
	{
		total_plus += 0.857;
	}
	if ((satzilla_feat.branch_depth_min < 2.00000))
	{
		total_neg += 0.947;
	}
	// num_rules: 12
	// rule_no: 12
	// default is: -

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score12(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((satzilla_feat.vcg_var_spread < 0.00000) &&
		(satzilla_feat.pnr_var_std < 1.40000) &&
		(satzilla_feat.avg_confl_size < 60.00000) &&
		(satzilla_feat.branch_depth_max < 628.00000) &&
		(satzilla_feat.trail_depth_delta_max < 6774.00000) &&
		(satzilla_feat.confl_per_restart < 266.20001))
	{
		total_neg += 0.923;
	}
	if ((satzilla_feat.confl_per_restart > 194.00000))
	{
		total_plus += 0.490;
	}
	if ((satzilla_feat.binary > 0.10000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.confl_size_max < 6371.00000) &&
		(satzilla_feat.trail_depth_delta_max > 6774.00000))
	{
		total_plus += 0.964;
	}
	if ((satzilla_feat.pnr_var_std > 0.30000) &&
		(satzilla_feat.confl_size_max > 6371.00000) &&
		(satzilla_feat.avg_trail_depth_delta < 4679.60010) &&
		(satzilla_feat.irred_cl_distrib.glue_distr_var < 5139.60010))
	{
		total_neg += 0.938;
	}
	if ((satzilla_feat.confl_size_min > 1.00000) &&
		(satzilla_feat.avg_confl_glue > 10.80000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 16.80000))
	{
		total_neg += 0.778;
	}
	if ((satzilla_feat.binary > 0.20000) &&
		(satzilla_feat.trail_depth_delta_max < 6774.00000))
	{
		total_neg += 0.783;
	}
	if ((satzilla_feat.branch_depth_max > 42.00000) &&
		(satzilla_feat.trail_depth_delta_max < 6774.00000) &&
		(satzilla_feat.confl_per_restart < 194.00000))
	{
		total_neg += 0.915;
	}
	if ((satzilla_feat.vcg_var_spread > 0.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_var > 2.30000))
	{
		total_plus += 0.800;
	}
	if ((satzilla_feat.binary < 0.10000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 3.60000))
	{
		total_neg += 0.846;
	}
	if ((satzilla_feat.numClauses > 54199.00000) &&
		(satzilla_feat.avg_confl_size > 26.80000) &&
		(satzilla_feat.trail_depth_delta_max < 6774.00000))
	{
		total_neg += 0.909;
	}
	if ((satzilla_feat.pnr_var_std > 1.40000) &&
		(satzilla_feat.branch_depth_max < 42.00000))
	{
		total_plus += 0.833;
	}
	if ((satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.confl_size_max < 6371.00000) &&
		(satzilla_feat.trail_depth_delta_max > 6774.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean > 3.60000))
	{
		total_plus += 0.843;
	}
	if ((satzilla_feat.pnr_var_std > 0.30000) &&
		(satzilla_feat.avg_trail_depth_delta > 4679.60010))
	{
		total_plus += 0.846;
	}
	// num_rules: 13
	// rule_no: 13
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score16(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double default_val = 0.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((satzilla_feat.avg_branch_depth > 18.60000))
	{
		total_neg += 0.625;
	}
	if ((satzilla_feat.binary < 0.40000))
	{
		total_plus += 0.498;
	}
	if ((satzilla_feat.vcg_var_std < 2.10000) &&
		(satzilla_feat.vcg_cls_std > 5.50000) &&
		(satzilla_feat.pnr_cls_std < 11.90000))
	{
		total_plus += 0.939;
	}
	if ((satzilla_feat.horn > 0.10000) &&
		(satzilla_feat.vcg_var_std < 2.20000) &&
		(satzilla_feat.confl_size_min < 1.00000) &&
		(satzilla_feat.confl_size_max > 149.00000) &&
		(satzilla_feat.avg_branch_depth < 178.30000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.50000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_var < 3.60000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var < 0.40000) &&
		(satzilla_feat.red_cl_distrib.size_distr_mean > 5.20000))
	{
		total_plus += 0.893;
	}
	if ((satzilla_feat.vcg_var_std > 2.20000) &&
		(satzilla_feat.vcg_var_std < 3.30000) &&
		(satzilla_feat.avg_branch_depth > 18.60000) &&
		(satzilla_feat.avg_branch_depth < 181.89999) &&
		(satzilla_feat.confl_per_restart < 262.10001) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var < 0.40000))
	{
		total_plus += 0.941;
	}
	if ((satzilla_feat.numClauses < 7548140.00000) &&
		(satzilla_feat.trail_depth_delta_max > 167286.00000))
	{
		total_plus += 0.826;
	}
	if ((satzilla_feat.vcg_var_std < 2.20000) &&
		(satzilla_feat.confl_size_max > 149.00000) &&
		(satzilla_feat.avg_branch_depth > 218.20000) &&
		(satzilla_feat.branch_depth_min < 101.00000) &&
		(satzilla_feat.red_cl_distrib.glue_distr_var < 0.40000))
	{
		total_plus += 0.889;
	}
	if ((satzilla_feat.numClauses < 108335.00000) &&
		(satzilla_feat.vcg_var_max < 0.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean > 5.90000))
	{
		total_plus += 0.950;
	}
	// num_rules: 8
	// rule_no: 8
	// default is: -

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score17(const SatZillaFeatures& satzilla_feat, const int verb)
{
	double default_val = 0.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((satzilla_feat.confl_size_max > 115.00000))
	{
		total_neg += 0.579;
	}
	if ((satzilla_feat.confl_size_max < 115.00000))
	{
		total_plus += 0.896;
	}
	if ((satzilla_feat.vcg_cls_std < 5.50000) &&
		(satzilla_feat.branch_depth_min < 18.00000) &&
		(satzilla_feat.confl_per_restart > 169.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.50000) &&
		(satzilla_feat.red_cl_distrib.size_distr_var < 14.50000) &&
		(satzilla_feat.red_cl_distrib.activity_distr_var < 131731750000.00000))
	{
		total_plus += 0.811;
	}
	if ((satzilla_feat.vcg_cls_std > 5.50000) &&
		(satzilla_feat.avg_confl_size > 45.90000))
	{
		total_plus += 0.909;
	}
	if ((satzilla_feat.avg_branch_depth < 15.80000))
	{
		total_plus += 0.900;
	}
	if ((satzilla_feat.avg_trail_depth_delta > 5287.70020) &&
		(satzilla_feat.irred_cl_distrib.size_distr_var < 2.10000))
	{
		total_plus += 0.889;
	}
	if ((satzilla_feat.numClauses > 84464.00000) &&
		(satzilla_feat.pnr_var_mean > 0.50000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean > 4.50000))
	{
		total_plus += 0.833;
	}
	if ((satzilla_feat.red_cl_distrib.activity_distr_var > 131731750000.00000))
	{
		total_plus += 0.722;
	}
	if ((satzilla_feat.horn > 0.60000) &&
		(satzilla_feat.pnr_var_max < 0.50000) &&
		(satzilla_feat.avg_branch_depth_delta < 2.00000))
	{
		total_plus += 0.909;
	}
	if ((satzilla_feat.pnr_var_std > 0.50000) &&
		(satzilla_feat.branch_depth_min < 18.00000) &&
		(satzilla_feat.confl_per_restart > 169.00000) &&
		(satzilla_feat.confl_per_restart < 296.29999) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.50000) &&
		(satzilla_feat.red_cl_distrib.activity_distr_var < 131731750000.00000))
	{
		total_plus += 0.944;
	}
	if ((satzilla_feat.pnr_var_std < 0.30000) &&
		(satzilla_feat.pnr_var_max > 0.50000) &&
		(satzilla_feat.branch_depth_min < 18.00000) &&
		(satzilla_feat.avg_trail_depth_delta < 5287.70020) &&
		(satzilla_feat.trail_depth_delta_min < 2.00000) &&
		(satzilla_feat.confl_per_restart < 296.29999) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.50000))
	{
		total_plus += 0.857;
	}
	if ((satzilla_feat.trail_depth_delta_min > 2.00000) &&
		(satzilla_feat.irred_cl_distrib.size_distr_mean < 4.50000))
	{
		total_plus += 0.769;
	}
	// num_rules: 12
	// rule_no: 12
	// default is: -

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


} //end namespace
