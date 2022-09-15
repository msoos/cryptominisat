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
    dirname="${basename}-cut1-${cut1}-cut2-${cut2}-limit-${limit}-est${est}-xbmin${xgboostminchild}-xbmd${xboostmaxdepth}-reg${regressor}-subs${xgboostsubsample}"

    mkdir -p ${dirname}
    rm -f ${dirname}/out_*
    rm -f ${dirname}/predictor*.json
    rm -f dirname.tar.gz
    git rev-parse HEAD > ${dirname}/out_git
    cat learn.sh >> ${dirname}/out_git
    md5sum *.dat >> ${dirname}/out_git

    tiers=("short" "long" "forever")
    tables=("used_later" "used_later_anc")
    for tier in "${tiers[@]}"; do
        mypids=()
        for table in "${tables[@]}"; do
            # check if DAT file exists
            INFILE="comb-${table}-${tier}-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat"
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
            --xgboostest ${est} \
            --xgboostminchild $xgboostminchild --xboostmaxdepth=${xboostmaxdepth} \
            --basedir "${dirname}" \
            --features "best_only" \
            --table ${table} \
            --xgboostsubsample "$xgboostsubsample" \
            --bestfeatfile ${bestf} 2>&1 | tee "${dirname}/out-${table}-${tier}" &
            pid=$!
            echo "PID here is $pid"
            mypids+=("$pid")
        done

        # wait for PIDs now
        echo "PIDS to wait for are: ${mypids[*]}"
        for pid2 in "${mypids[@]}"
        do
            echo "Waiting for $pid2 ..."
            wait $pid2
        done
    done

    tar czvf ${dirname}.tar.gz ${dirname}
}


# best was: 8march-2020-3acd81dc55df3-cut1-5.0-cut2-30.0-limit-2000-est10-w0-xbmin300-xbmd4

#xboostmaxdepth=4
#xboostminchild=300
#est=10

# bestf="../../scripts/crystal/best_features-correlation2.txt"
bestf="../../scripts/crystal/best_features-kissat.txt"

w=0
xgboostsubsample="1.0"
#basename="15-dec-b1bd8f74bc2b42"
basename="15-dec-b1bd8f74bc2b42-kissat-2"
#basename="14-april-2021-69bad529f962c"
#basename="8march-2020-3acd81dc55df3-36feats"
#basename="aes-30-march-2020-a1e0e19be0c1"
#basename="orig"
limit=1000
cut1="3.0"
cut2="25.0"
xboostmaxdepth=4
xgboostminchild=300
est=10

for xgboostsubsample in 1.0
do
for limit in 10000 #1000
do
    for regressor in "xgb" #"lgbm"
    do
        for xboostmaxdepth in 4 #6 #8 10 12
        do
            for xgboostminchild in 10 #300
            do
                for est in 10
                do
                    generate
                done
            done
        done
    done
done
done

exit 0


# simple run:

# ./cryptominisat5 goldb-heqc-i10mul.cnf --simdrat 1 --printsol 0 --predloc ./data/15-dec-b1bd8f74bc2b42-cut1-3.0-cut2-25.0-limit-10000-est10-xbmin10-xbmd4-regxgb-subs1.0/ --predtype py --predbestfeats ../scripts/crystal/best_features-correlation2.txt --predtables 000
