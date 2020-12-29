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
    name="cut1-${cut1}-cut2-${cut2}-limit-${limit}-est${est}-w${w}"

    tiers=("short" "long" "forever")
    for tier in "${tiers[@]}"
    do
        /usr/bin/time --verbose -o "classifiers/out_${tier}-${name}.timeout" \
        ../cldata_predict.py \
        ${tier}-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat \
        --tier ${tier} --final --xgboost \
        --xgboostest ${est} --weight ${w} \
        --basedir ../../src/predict/ --bestfeatfile ${bestf} | tee classifiers/out_${tier}
    done

    cp ../../src/predict/*.json classifiers/
    tar czvf classifiers-${name}.tar.gz classifiers
}

limit=2000
w=0
# est=20
# cut1="50.0"
# cut2="80.0"
# concat

# w=3
# est=20
# cut1="10.0"
# cut2="40.0"
# concat
#
# w=5
# est=20
# cut1="10.0"
# cut2="40.0"
# concat

w=0
est=40
cut1="10.0"
cut2="40.0"
concat

w=0
est=60
cut1="10.0"
cut2="40.0"
concat

# est=40
# cut1="40.0"
# cut2="70.0"
# concat

# est=20
# cut1="40.0"
# cut2="70.0"
# concat

exit 0
