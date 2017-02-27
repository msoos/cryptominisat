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

#ifndef _FEATURES_TO_RECONF_H_
#define _FEATURES_TO_RECONF_H_

#include "solvefeatures.h"
#include <iostream>
using std::cout;
using std::endl;

namespace CMSat {

double get_score0(const SolveFeatures& feat, const int verb);
double get_score7(const SolveFeatures& feat, const int verb);

int get_reconf_from_features(const SolveFeatures& feat, const int verb)
{
	double best_score = 0.0;
	int best_val = 0;
	double score;


	score = get_score0(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 0 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 0;
	}


	score = get_score7(feat, verb);
	if (verb >= 2)
		cout << "c Score for reconf 7 is " << score << endl;
	if (best_score < score) {
		best_score = score;
		best_val = 7;
	}


	if (verb >= 2)
		cout << "c Winning reconf is " << best_val << endl;
	return best_val;
}



double get_score0(const SolveFeatures& feat, const int verb)
{
	double default_val = 1.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.horn < 0.96500))
	{
		total_plus += 0.850;
	}
	if ((feat.vcg_var_mean > 0.00090) &&
		(feat.vcg_var_min < 0.00050) &&
		(feat.branch_depth_min > 18.00000) &&
		(feat.irred_cl_distrib.size_distr_var > 0.94500))
	{
		total_neg += 0.909;
	}
	if ((feat.horn > 0.96500) &&
		(feat.avg_confl_glue > 27.23350))
	{
		total_neg += 0.875;
	}
	if ((feat.vcg_var_mean > 0.00090) &&
		(feat.vcg_var_min < 0.00050) &&
		(feat.vcg_cls_spread > 0.00150) &&
		(feat.branch_depth_min > 18.00000))
	{
		total_neg += 0.875;
	}
	if ((feat.avg_confl_glue < 27.23350) &&
		(feat.branch_depth_min < 18.00000))
	{
		total_plus += 0.970;
	}
	if ((feat.vcg_var_mean < 0.00090) &&
		(feat.avg_confl_glue < 27.23350))
	{
		total_plus += 0.903;
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
	double default_val = 0.00;

	double total_plus = 0.0;
	double total_neg = 0.0;
	if ((feat.avg_confl_glue < 27.23350))
	{
		total_neg += 0.601;
	}
	if ((feat.vcg_var_min < 0.00010) &&
		(feat.pnr_var_std < 0.22300) &&
		(feat.red_cl_distrib.glue_distr_var > 0.43030))
	{
		total_plus += 0.955;
	}
	if ((feat.pnr_var_mean > 0.49400) &&
		(feat.pnr_var_std > 0.22470) &&
		(feat.pnr_var_std < 0.23520))
	{
		total_plus += 0.875;
	}
	if ((feat.horn_mean < 0.00110) &&
		(feat.vcg_var_mean > 0.00060) &&
		(feat.vcg_cls_std > 0.11230) &&
		(feat.pnr_var_std > 0.23520) &&
		(feat.avg_confl_glue < 16.31490) &&
		(feat.branch_depth_min > 11.00000))
	{
		total_plus += 0.929;
	}
	if ((feat.avg_confl_glue > 27.23350))
	{
		total_plus += 0.909;
	}
	if ((feat.vcg_var_mean < 0.00010) &&
		(feat.confl_size_min < 1.00000) &&
		(feat.avg_confl_glue < 19.17660) &&
		(feat.num_xors_found_last < 1654.00000))
	{
		total_plus += 0.810;
	}
	if ((feat.horn_mean < 0.00110) &&
		(feat.vcg_var_mean > 0.00060) &&
		(feat.vcg_cls_std > 0.11230) &&
		(feat.pnr_var_std > 0.29280) &&
		(feat.avg_confl_glue < 16.31490))
	{
		total_plus += 0.917;
	}
	// num_rules: 7
	// rule_no: 7
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

#endif //_FEATURES_TO_RECONF_H_


