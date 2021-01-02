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
    rm -f out_*
    rm -f ../../src/predict/*.json
    rm -f classifiers/*
    git rev-parse HEAD > out_git
    cat learn.sh >> out_git
    md5sum *.dat >> out_git

    bestf="../../scripts/crystal/best_features-rdb0-only.txt"
    dirname="backto-cut1-${cut1}-cut2-${cut2}-limit-${limit}-est${est}-w${w}-xbmin${xgboostminchild}-xbmd${xboostmaxdepth}"
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



w="0"

xgboostminchild=1
xboostmaxdepth=6
limit=2000
cut1="5.0"
cut2="30.0"
est=20
for xboostmaxdepth in 4 6
do
    limit=2000
    cut1="5.0"
    cut2="30.0"
    for xgboostminchild in 1 50 300
    do
        for est in 10 20
        do
            concat
        done
    done
done


exit 0
