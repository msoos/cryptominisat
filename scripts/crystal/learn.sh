#!/bin/bash

# Copyright (C) 2020  Mate Soos
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# This file wraps CMake invocation for TravisCI
# so we can set different configurations via environment variables.

#set -x
set -e

function concat() {
    dirname="${basename}-${cut1}-cut2-${cut2}-limit-${limit}-est${est}-w${w}-xbmin${xgboostminchild}-xbmd${xboostmaxdepth}"

    rm -f ../../src/predict/*.json
    git rev-parse HEAD > ${dirname}/out_git
    cat learn.sh >> ${dirname}/out_git
    md5sum *.dat >> ${dirname}/out_git

    bestf="../../scripts/crystal/best_features-rdb0-only.txt"
    mkdir -p ${dirname}

    mypids=()
    tiers=("short" "long" "forever")
    for tier in "${tiers[@]}"
    do
        /usr/bin/time --verbose -o "${dirname}/out_${tier}.timeout" \
        ../cldata_predict.py \
        ${tier}-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat \
        --tier ${tier} --final --xgboost \
        --xgboostest ${est} --weight ${w} \
        --xgboostminchild $xgboostminchild --xboostmaxdepth=${xboostmaxdepth} \
        --basedir "${dirname}" \
        --bestfeatfile ${bestf} | tee "${dirname}/out_${tier}" &
        pid=$!
        echo "PID here is $pid"
        mypids+=("$pid")
    done

    echo "PIDS to wait for are: ${mypids[*]}"
    for pid2 in "${mypids[@]}"
    do
        echo "Waiting for $pid2 ..."
        wait $pid2
    done

    tar czvf classifiers-${dirname}.tar.gz ${dirname}
}

# best was: backto-cut1-5.0-cut2-30.0-limit-2000-est20-w0-xbmin50-xbmd6
# next best:backto-cut1-5.0-cut2-30.0-limit-2000-est10-w0-xbmin50-xbmd6
# next      backto-cut1-10.0-cut2-40.0-limit-8000-est60-w0-xbmin500-xbmd4
# next      backto-cut1-5.0-cut2-30.0-limit-2000-est10-w0-xbmin300-xbmd6
# next      backto-cut1-5.0-cut2-30.0-limit-2000-est10-w0-xbmin1-xbmd4

#xgboostminchild=1
#xboostmaxdepth=6
basename="8march-2020-3acd81dc55df3"
limit=2000
cut1="5.0"
cut2="30.0"
est=20
for xboostmaxdepth in 4 6
do
    limit=2000
    cut1="5.0"
    cut2="30.0"
    for xgboostminchild in 50 300
    do
        for est in 10 20
        do
            concat
        done
    done
done


exit 0
