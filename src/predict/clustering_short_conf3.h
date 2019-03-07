/******************************************
Copyright (c) 2018, Mate Soos

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


#ifndef CLUSTERING_short_conf3_H
#define CLUSTERING_short_conf3_H

#include "satzilla_features.h"
#include "clustering.h"
#include <cmath>

namespace CMSat {
class Clustering_short_conf3: public Clustering {

public:
    Clustering_short_conf3() {
        set_up_centers();
    }

    virtual ~Clustering_short_conf3() {
    }

    SatZillaFeatures center[1];
    std::vector<int> used_clusters;

    virtual void set_up_centers() {

        // Doing cluster center 0

        used_clusters.push_back(0);
        center[0].var_cl_ratio = 5.051874367824474e-15L;
        center[0].numClauses = -9.418628414510927e-14L;
        center[0].avg_num_resolutions = -1.3755316668760671e-14L;
        center[0].irred_cl_distrib.size_distr_mean = -2.077452408583675e-14L;
    }

    double sq(double x) const {
        return x*x;
    }

    virtual double norm_dist(const SatZillaFeatures& a, const SatZillaFeatures& b) const {
        double dist = 0;
        double tmp;
        tmp = (a.var_cl_ratio-0.080889491L)/0.04933107L;
        dist+=sq(tmp-b.var_cl_ratio);

        tmp = (a.numClauses-637242.697125982L)/1179761.85679346L;
        dist+=sq(tmp-b.numClauses);

        tmp = (a.avg_num_resolutions-93.950312776L)/362.63682426L;
        dist+=sq(tmp-b.avg_num_resolutions);

        tmp = (a.irred_cl_distrib.size_distr_mean-7.505814145L)/8.68140833L;
        dist+=sq(tmp-b.irred_cl_distrib.size_distr_mean);


        return dist;
    }

    virtual int which_is_closest(const SatZillaFeatures& p) const {
        double closest_dist = std::numeric_limits<double>::max();
        int closest = -1;
        for (int i: used_clusters) {
            double dist = norm_dist(p, center[i]);
            if (dist < closest_dist) {
                closest_dist = dist;
                closest = i;
            }
        }
        return closest;
    }

};


} //end namespace

#endif //header guard
