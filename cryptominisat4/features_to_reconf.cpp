
#ifndef _FEATURES_TO_RECONF_H_
#define _FEATURES_TO_RECONF_H_

#include "features.h"
#include <iostream>
using std::cout;
using std::endl;

namespace CMSat {

double get_score1(const Features& feat, const int verb);
double get_score2(const Features& feat, const int verb);
double get_score3(const Features& feat, const int verb);
double get_score4(const Features& feat, const int verb);
double get_score6(const Features& feat, const int verb);
double get_score7(const Features& feat, const int verb);
double get_score12(const Features& feat, const int verb);
double get_score13(const Features& feat, const int verb);

int get_reconf_from_features(const Features& feat, const int verb)
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


	score = get_score2(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 2 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 2;
	}


	score = get_score3(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 3 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 3;
	}


	score = get_score4(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 4 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 4;
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


	if (verb >= 2)
		cout << "c Winning reconf is " << best_val << endl;
	return best_val;
}



double get_score1(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.decisions > 194960.00000))
	{
		total_plus += 0.807;
	}
	if ((feat.decisions < 194960.00000))
	{
		total_neg += 0.840;
	}
	if ((feat.num_restarts > 1425.00000) &&
		(feat.blocked_restart > 376.00000))
	{
		total_neg += 0.938;
	}
	if ((feat.vcg_var_min > 0.00000) &&
		(feat.vcg_cls_std > 0.43415) &&
		(feat.unary < 0.94206) &&
		(feat.trinary > 0.57041) &&
		(feat.trail_depth_delta_hist_max > 523.00000))
	{
		total_neg += 0.929;
	}
	if ((feat.vcg_var_min > 0.00008) &&
		(feat.vcg_cls_min < 0.00237) &&
		(feat.trinary > 0.57041) &&
		(feat.trail_depth_delta_hist_max > 523.00000))
	{
		total_neg += 0.889;
	}
	if ((feat.vcg_var_min > 0.00000) &&
		(feat.unary < 0.94206) &&
		(feat.trinary > 0.57041) &&
		(feat.lt_num_resolutions_max > 3496.00000))
	{
		total_neg += 0.889;
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


double get_score2(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.decisions < 5389699.00000))
	{
		total_plus += 0.659;
	}
	if ((feat.vcg_cls_std < 0.84905) &&
		(feat.pnr_var_min < 0.01852) &&
		(feat.pnr_cls_std > 0.47154) &&
		(feat.pnr_cls_std < 0.61331) &&
		(feat.unary < 0.50517) &&
		(feat.branch_depth_hist > 15.74000) &&
		(feat.lt_confl_glue_min < 1.00000) &&
		(feat.branch_depth_hist_max < 142.00000))
	{
		total_neg += 0.960;
	}
	if ((feat.numClauses < 234042.00000) &&
		(feat.trinary > 0.06502) &&
		(feat.props_per_confl > 0.00795))
	{
		total_neg += 0.957;
	}
	if ((feat.trail_depth_delta_hist_min < 10.00000) &&
		(feat.decisions > 5389699.00000))
	{
		total_neg += 0.944;
	}
	if ((feat.pnr_var_max < 0.98592) &&
		(feat.pnr_cls_std > 0.43951) &&
		(feat.unary < 0.50517) &&
		(feat.branch_depth_hist > 15.74000))
	{
		total_neg += 0.852;
	}
	if ((feat.vcg_cls_max < 0.00336) &&
		(feat.pnr_var_std < 0.24435) &&
		(feat.pnr_cls_std > 0.43951) &&
		(feat.unary > 0.50517) &&
		(feat.lt_confl_size < 53.09195))
	{
		total_neg += 0.857;
	}
	if ((feat.vcg_cls_std < 0.84905) &&
		(feat.pnr_cls_std > 0.47154) &&
		(feat.pnr_cls_std < 0.61331) &&
		(feat.unary < 0.50517) &&
		(feat.branch_depth_hist_max > 276.00000))
	{
		total_neg += 0.944;
	}
	if ((feat.pnr_cls_std > 0.47154) &&
		(feat.pnr_cls_std < 0.61331) &&
		(feat.unary < 0.50517) &&
		(feat.binary < 0.27233) &&
		(feat.branch_depth_hist > 15.74000))
	{
		total_neg += 0.956;
	}
	if ((feat.pnr_cls_std > 3.11628))
	{
		total_neg += 0.833;
	}
	if ((feat.vcg_cls_max > 0.00070) &&
		(feat.vcg_cls_max < 0.00144) &&
		(feat.pnr_cls_std > 0.61331) &&
		(feat.unary < 0.50517))
	{
		total_neg += 0.867;
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


double get_score3(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.vcg_var_mean < 0.00115))
	{
		total_plus += 0.834;
	}
	if ((feat.lt_num_resolutions_max < 8789.00000) &&
		(feat.decisions < 194960.00000))
	{
		total_neg += 0.952;
	}
	if ((feat.var_cl_ratio < 0.21592) &&
		(feat.pnr_var_std < 0.24435) &&
		(feat.pnr_cls_std > 0.43684) &&
		(feat.horn_min < 0.00009) &&
		(feat.branch_depth_hist_min < 89.00000) &&
		(feat.num_restarts > 455.00000) &&
		(feat.blocked_restart < 736.00000))
	{
		total_neg += 0.909;
	}
	if ((feat.vcg_var_mean > 0.00115) &&
		(feat.binary < 0.02434))
	{
		total_neg += 0.950;
	}
	if ((feat.pnr_var_std > 0.24435) &&
		(feat.binary > 0.02434) &&
		(feat.branch_depth_hist_min < 89.00000) &&
		(feat.props_per_confl > 0.00012) &&
		(feat.decisions > 194960.00000))
	{
		total_plus += 0.970;
	}
	if ((feat.var_cl_ratio > 0.21592) &&
		(feat.pnr_var_std < 0.24435) &&
		(feat.branch_depth_hist_min < 89.00000))
	{
		total_plus += 0.947;
	}
	if ((feat.trail_depth_delta_hist > 5886.59670) &&
		(feat.branch_depth_hist_min > 89.00000))
	{
		total_neg += 0.875;
	}
	if ((feat.vcg_var_spread < 0.00658) &&
		(feat.pnr_var_mean > 0.49915) &&
		(feat.binary > 0.02434) &&
		(feat.branch_depth_hist_min > 89.00000))
	{
		total_neg += 0.875;
	}
	if ((feat.var_cl_ratio < 0.21592) &&
		(feat.binary > 0.02434) &&
		(feat.blocked_restart > 736.00000))
	{
		total_plus += 0.914;
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


double get_score4(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.lt_num_resolutions > 11.15448))
	{
		total_plus += 0.771;
	}
	if ((feat.pnr_var_max > 0.85714) &&
		(feat.lt_confl_size > 23.31419) &&
		(feat.decisions < 194960.00000))
	{
		total_neg += 0.947;
	}
	if ((feat.lt_num_resolutions < 11.15448) &&
		(feat.blocked_restart < 736.00000))
	{
		total_neg += 0.889;
	}
	if ((feat.pnr_cls_std > 0.43951) &&
		(feat.pnr_cls_std < 0.46166) &&
		(feat.decisions > 194960.00000) &&
		(feat.decisions < 1576090.00000))
	{
		total_neg += 0.842;
	}
	if ((feat.pnr_var_max < 0.77778) &&
		(feat.lt_confl_glue_max > 298.00000))
	{
		total_neg += 0.909;
	}
	if ((feat.vcg_cls_std > 0.28952) &&
		(feat.vcg_cls_min < 0.00003) &&
		(feat.pnr_cls_std > 0.43951) &&
		(feat.horn_spread < 0.00351) &&
		(feat.branch_depth_hist > 170.00000) &&
		(feat.blocked_restart > 14.00000))
	{
		total_neg += 0.889;
	}
	if ((feat.vcg_cls_std < 0.28952) &&
		(feat.pnr_cls_std > 1.88265))
	{
		total_neg += 0.833;
	}
	if ((feat.vcg_cls_min < 0.00003) &&
		(feat.pnr_var_std < 0.22591) &&
		(feat.pnr_cls_std > 0.43951))
	{
		total_neg += 0.833;
	}
	if ((feat.var_cl_ratio < 0.17252) &&
		(feat.pnr_var_max > 0.77778) &&
		(feat.pnr_cls_std < 0.46166))
	{
		total_plus += 0.918;
	}
	if ((feat.pnr_cls_std > 0.43951) &&
		(feat.pnr_cls_std < 0.46166) &&
		(feat.lt_confl_glue > 8.45036) &&
		(feat.decisions > 194960.00000) &&
		(feat.decisions < 1576090.00000))
	{
		total_neg += 0.933;
	}
	if ((feat.var_cl_ratio < 0.11220) &&
		(feat.decisions < 194960.00000))
	{
		total_neg += 0.900;
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


double get_score6(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.numClauses > 129501.00000))
	{
		total_plus += 0.822;
	}
	if ((feat.numClauses < 101344.00000) &&
		(feat.binary > 0.02434) &&
		(feat.decisions > 190776.00000))
	{
		total_plus += 0.819;
	}
	if ((feat.vcg_var_min > 0.00000))
	{
		total_neg += 0.333;
	}
	if ((feat.vcg_var_min < 0.00001) &&
		(feat.binary > 0.02434) &&
		(feat.branch_depth_hist < 239.80193) &&
		(feat.props_per_confl > 0.00086))
	{
		total_plus += 0.984;
	}
	if ((feat.vcg_var_min < 0.00000) &&
		(feat.pnr_var_std > 0.34581) &&
		(feat.horn_max < 0.00030))
	{
		total_neg += 0.875;
	}
	if ((feat.numClauses > 49746.00000) &&
		(feat.vcg_var_min > 0.00001) &&
		(feat.vcg_var_spread < 0.00650) &&
		(feat.vcg_cls_min > 0.00012) &&
		(feat.lt_confl_glue_min < 1.00000))
	{
		total_neg += 0.850;
	}
	if ((feat.vcg_var_mean < 0.00015) &&
		(feat.vcg_var_min > 0.00000) &&
		(feat.vcg_var_min < 0.00001) &&
		(feat.binary < 0.58868) &&
		(feat.props_per_confl < 0.00086))
	{
		total_neg += 0.889;
	}
	if ((feat.var_cl_ratio < 0.21135) &&
		(feat.vcg_var_spread < 0.00650) &&
		(feat.pnr_var_mean > 0.50197) &&
		(feat.horn_max > 0.00030))
	{
		total_neg += 0.846;
	}
	if ((feat.numClauses > 49746.00000) &&
		(feat.numClauses < 129501.00000) &&
		(feat.vcg_var_spread < 0.00650) &&
		(feat.branch_depth_hist < 15.05010))
	{
		total_neg += 0.900;
	}
	if ((feat.vcg_cls_std > 0.27945) &&
		(feat.binary < 0.02434))
	{
		total_neg += 0.938;
	}
	if ((feat.vcg_var_spread > 0.00650) &&
		(feat.pnr_var_max < 0.95238) &&
		(feat.trail_depth_delta_hist_min < 4.00000))
	{
		total_neg += 0.667;
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


double get_score7(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.vcg_cls_min > 0.00001))
	{
		total_plus += 0.562;
	}
	if ((feat.pnr_var_std > 0.88650) &&
		(feat.horn_mean < 0.00007))
	{
		total_neg += 0.941;
	}
	if ((feat.vcg_var_spread > 0.00133) &&
		(feat.vcg_cls_spread < 0.01812) &&
		(feat.blocked_restart > 711.00000) &&
		(feat.learntBins > 1153.00000))
	{
		total_neg += 0.964;
	}
	if ((feat.lt_confl_glue_min > 1.00000) &&
		(feat.blocked_restart > 402.00000))
	{
		total_neg += 0.795;
	}
	if ((feat.pnr_var_mean < 0.48994) &&
		(feat.learntBins > 4681.00000))
	{
		total_neg += 0.941;
	}
	if ((feat.trail_depth_delta_hist < 12806.59300) &&
		(feat.lt_confl_size_min > 2989.00000))
	{
		total_neg += 0.875;
	}
	if ((feat.horn > 0.99868))
	{
		total_neg += 0.833;
	}
	if ((feat.vcg_cls_spread < 0.01812) &&
		(feat.pnr_cls_std > 0.58502) &&
		(feat.branch_depth_hist_min > 10.00000) &&
		(feat.blocked_restart > 711.00000) &&
		(feat.learntBins < 1153.00000))
	{
		total_neg += 0.929;
	}
	if ((feat.horn_min < 0.00030) &&
		(feat.horn < 0.99868) &&
		(feat.lt_confl_glue < 17.72951) &&
		(feat.blocked_restart < 402.00000) &&
		(feat.learntBins < 4681.00000))
	{
		total_plus += 0.892;
	}
	if ((feat.var_cl_ratio < 0.16898) &&
		(feat.vcg_cls_spread < 0.01812) &&
		(feat.branch_depth_hist_min > 10.00000) &&
		(feat.blocked_restart > 711.00000))
	{
		total_neg += 0.958;
	}
	if ((feat.vcg_cls_spread > 0.01812) &&
		(feat.blocked_restart > 711.00000))
	{
		total_plus += 0.889;
	}
	if ((feat.lt_confl_glue > 17.72951) &&
		(feat.lt_confl_glue_min > 1.00000) &&
		(feat.lt_confl_glue_min < 4.00000))
	{
		total_neg += 0.857;
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


double get_score12(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.lt_confl_glue < 13.03022))
	{
		total_plus += 0.810;
	}
	if ((feat.pnr_var_spread > 0.97845) &&
		(feat.binary > 0.38519) &&
		(feat.lt_confl_glue_max < 11403.00000) &&
		(feat.num_restarts < 1442.00000) &&
		(feat.decisions > 188642.00000))
	{
		total_plus += 0.976;
	}
	if ((feat.var_cl_ratio > 0.16744) &&
		(feat.branch_depth_hist_max < 314.00000) &&
		(feat.decisions > 188642.00000))
	{
		total_plus += 0.860;
	}
	if ((feat.lt_confl_glue > 13.03022) &&
		(feat.decisions < 188642.00000))
	{
		total_neg += 0.947;
	}
	if ((feat.branch_depth_hist_max > 314.00000) &&
		(feat.num_restarts > 1442.00000))
	{
		total_neg += 0.938;
	}
	if ((feat.vcg_var_std < 0.83612) &&
		(feat.pnr_var_min < 0.06667) &&
		(feat.pnr_var_spread < 0.97845) &&
		(feat.lt_num_resolutions < 75.10673) &&
		(feat.props_per_confl < 0.00569) &&
		(feat.num_restarts < 745.00000))
	{
		total_neg += 0.824;
	}
	if ((feat.pnr_var_spread < 0.97845) &&
		(feat.lt_num_resolutions < 75.10673) &&
		(feat.branch_depth_hist_max > 36.00000) &&
		(feat.props_per_confl < 0.00569) &&
		(feat.num_restarts < 745.00000))
	{
		total_neg += 0.900;
	}
	if ((feat.pnr_var_max < 0.72222))
	{
		total_neg += 0.889;
	}
	if ((feat.pnr_var_mean > 0.53670) &&
		(feat.pnr_cls_std < 0.46166))
	{
		total_neg += 0.875;
	}
	if ((feat.vcg_cls_min > 0.00001) &&
		(feat.pnr_var_spread > 0.97845) &&
		(feat.pnr_cls_std > 0.46166) &&
		(feat.num_restarts < 1442.00000) &&
		(feat.decisions > 188642.00000))
	{
		total_plus += 0.966;
	}
	if ((feat.lt_confl_glue_max > 11403.00000))
	{
		total_neg += 0.714;
	}
	if ((feat.pnr_var_spread < 0.97845) &&
		(feat.lt_num_resolutions > 296.38834) &&
		(feat.decisions > 188642.00000))
	{
		total_neg += 0.875;
	}
	if ((feat.var_cl_ratio < 0.16744) &&
		(feat.num_restarts > 1442.00000))
	{
		total_neg += 0.933;
	}
	if ((feat.vcg_cls_min < 0.00001) &&
		(feat.pnr_cls_std > 0.46166) &&
		(feat.binary < 0.38519))
	{
		total_neg += 0.857;
	}
	if ((feat.pnr_var_max > 0.72222) &&
		(feat.props_per_confl > 0.00569) &&
		(feat.num_restarts < 1442.00000) &&
		(feat.decisions > 188642.00000))
	{
		total_plus += 0.963;
	}
	if ((feat.pnr_var_spread < 0.97845) &&
		(feat.branch_depth_delta_hist > 12.33333) &&
		(feat.props_per_confl < 0.00569) &&
		(feat.num_restarts < 1442.00000))
	{
		total_neg += 0.875;
	}
	// num_rules: 16
	// rule_no: 16
	// default is: +

	if (total_plus == 0.0 && total_neg == 0.0) {
		return default_val;
	}
	if (verb >= 2) {
		//cout << "c plus: " << total_plus << " , neg: " << total_neg << endl;
	}
	return total_plus - total_neg;
}


double get_score13(const Features& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.vcg_var_std < 2.32482))
	{
		total_plus += 0.690;
	}
	if ((feat.branch_depth_hist_max > 29.00000))
	{
		total_neg += 0.405;
	}
	if ((feat.numClauses < 1290172.00000) &&
		(feat.vcg_var_std < 4.44515) &&
		(feat.pnr_var_std > 0.32006) &&
		(feat.num_restarts < 1335.00000))
	{
		total_plus += 0.945;
	}
	if ((feat.pnr_var_std > 0.32006) &&
		(feat.num_restarts > 1335.00000))
	{
		total_neg += 0.958;
	}
	if ((feat.vcg_var_std > 0.97794) &&
		(feat.vcg_var_std < 2.18500) &&
		(feat.pnr_var_std < 0.32006) &&
		(feat.lt_num_resolutions > 18.28283) &&
		(feat.branch_depth_hist_max > 29.00000) &&
		(feat.trail_depth_delta_hist_min < 9.00000) &&
		(feat.trail_depth_delta_hist_max < 4298.00000))
	{
		total_neg += 0.929;
	}
	if ((feat.vcg_var_std < 4.44515) &&
		(feat.pnr_var_std < 0.32006) &&
		(feat.pnr_var_max > 0.84615) &&
		(feat.lt_confl_size_min > 716.00000) &&
		(feat.lt_confl_glue_min < 1.00000) &&
		(feat.lt_confl_glue_max < 390.00000) &&
		(feat.branch_depth_hist_max > 29.00000) &&
		(feat.num_restarts < 1335.00000))
	{
		total_neg += 0.875;
	}
	if ((feat.pnr_var_min < 0.00231) &&
		(feat.lt_num_resolutions < 44.52870) &&
		(feat.num_restarts > 1335.00000))
	{
		total_neg += 0.958;
	}
	if ((feat.pnr_var_std < 0.32006) &&
		(feat.trinary > 0.38456) &&
		(feat.horn_min < 0.00004) &&
		(feat.lt_confl_glue_min > 1.00000) &&
		(feat.branch_depth_hist_min < 83.00000))
	{
		total_neg += 0.900;
	}
	if ((feat.numClauses > 1290172.00000) &&
		(feat.vcg_var_max > 0.00349))
	{
		total_neg += 0.952;
	}
	if ((feat.numClauses > 1290172.00000) &&
		(feat.var_cl_ratio < 0.13915) &&
		(feat.pnr_var_mean < 0.15540))
	{
		total_neg += 0.889;
	}
	if ((feat.numClauses > 1290172.00000) &&
		(feat.vcg_var_std > 0.43290))
	{
		total_neg += 0.944;
	}
	if ((feat.var_cl_ratio < 0.19929) &&
		(feat.vcg_var_std > 4.44515))
	{
		total_neg += 0.938;
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


