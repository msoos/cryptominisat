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

set -e
set -x

numconfs=1

function wait_threads {
    FAIL=0
    for job in $(jobs -p)
    do
        echo "waiting for job '$job' ..."
        wait "${job}" || FAIL=$((FAIL+=1))
    done
}

function check_fails {
    if [ "$FAIL" == "0" ];
    then
        echo "All went well!"
    else
        echo "FAIL of one of the threads! ($FAIL)"
        exit 255
    fi
}

function check_error {
    # exit in case of errors
    if [[ -s error ]]; then
        echo "ERROR: Issues occurred!"
        exit 255
    else
        echo "OK, no errors"
    fi
}

function populate_error {
    rm -f error
    touch error
    for (( CONF = 0; CONF < numconfs; CONF++))
    do
        grep -E --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "${1}_${CONF}" 2>&1 | tee -a error
        grep -E --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "${2}_${CONF}" 2>&1 | tee -a error
    done
}

function combine {
    rm -f out_combine_short_*
    rm -f comb-short-conf-*.dat
    rm -f out_combine_long_*
    rm -f comb-long-conf-*.dat
    for (( CONF = 0; CONF < numconfs; CONF++))
    do
        rm -f "comb-short-conf-${CONF}.dat"
        rm -f "comb-long-conf-${CONF}.dat"

        ./combine_dats.py -o "comb-short-conf-${CONF}.dat" $1/*-short-conf-${CONF}.dat > "out${2}_combine_short_${CONF}" 2>&1 &
        ./combine_dats.py -o "comb-long-conf-${CONF}.dat"  $1/*-long-conf-${CONF}.dat  > "out${2}_combine_long_${CONF}" 2>&1 &

        wait_threads
        check_fails
    done

    populate_error "out_combine_short" "out_combine_long"
    check_error
}

function predict {
    mkdir -p ../src/predict
    rm -f ../src/predict/*.h
    rm -f out_pred_short_*
    rm -f out_pred_long_*
    for (( CONF = 0; CONF < numconfs; CONF++))
    do
        ./predict.py "comb-long-conf-${CONF}.dat" --scale --name long  --basedir "../src/predict/" --final --forest --clusters 1 --split 0.005 --conf "${CONF}" --clustmin 0.10 --mindump 19  --prefok 1 > "out_pred_long_${CONF}" 2>&1 &
        ./predict.py "comb-short-conf-${CONF}.dat" --scale --name short --basedir "../src/predict/" --final --forest --clusters 1 --split 0.005 --conf "${CONF}" --clustmin 0.10 --mindump 19 --prefok 1 > "out_pred_short_${CONF}" 2>&1 &
        wait_threads
        check_fails
    done

    populate_error "out_pred_long" "out_pred_short"
    check_error
}

################
# running
################

combine out-dats-8145576.wlm01 noclean
combine out-dats-8160767.wlm01 normal
predict
