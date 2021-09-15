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

function generate() {
    dirname="${basename}-cut1-${cut1}-cut2-${cut2}-limit-${limit}-est${est}-w${w}-xbmin${xgboostminchild}-xbmd${xboostmaxdepth}-reg${regressor}"


    mkdir -p ${dirname}
    rm -f ${dirname}/out_*
    rm -f ${dirname}/predictor*.json
    git rev-parse HEAD > ${dirname}/out_git
    cat learn.sh >> ${dirname}/out_git
    md5sum *.dat >> ${dirname}/out_git

    mypids=()
    tiers=("short" "long" "forever")
    for tier in "${tiers[@]}"
    do
        # check if DAT file exists
        INFILE="${tier}-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat"
        if test -f "$INFILE"; then
            echo "$INFILE exists, OK"
        else
            echo "ERROR: $INFILE does not exist!!"
            exit -1
        fi

        /usr/bin/time --verbose -o "${dirname}/out_${tier}.timeout" \
        ../cldata_predict.py \
        $INFILE \
        --tier ${tier} --regressor $regressor \
        --xgboostest ${est} --weight ${w} \
        --xgboostminchild $xgboostminchild --xboostmaxdepth=${xboostmaxdepth} \
        --basedir "${dirname}" \
        --features "best_only" \
        --xgboostsubsample "$xgboostsubsample" \
        --bestfeatfile ${bestf} 2>&1 | tee "${dirname}/out_${tier}" &
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

    tar czvf ${dirname}.tar.gz ${dirname}
}


# best was: 8march-2020-3acd81dc55df3-cut1-5.0-cut2-30.0-limit-2000-est10-w0-xbmin300-xbmd4

#xboostmaxdepth=4
#xboostminchild=300
#est=10

bestf="../../scripts/crystal/best_features-rdb0-only.txt"
w=0
xgboostsubsample="1.0"
basename="14-april-2021-69bad529f962c"
#basename="8march-2020-3acd81dc55df3-36feats"
#basename="aes-30-march-2020-a1e0e19be0c1"
#basename="orig"
limit=1000
cut1="3.0"
cut2="25.0"
xboostmaxdepth=4
xgboostminchild=300
est=10

for regressor in "xgboost" "lgbm"
do
    for xboostmaxdepth in 4 6
    do
        for xgboostminchild in 50 300
        do
            for est in 10
            do
                generate
            done
        done
    done
done

exit 0
