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
    rm -f classifiers/*
    git rev-parse HEAD > out_git
    cat learn.sh >> out_git
    md5sum *.dat >> out_git

    bestf_short="../../scripts/crystal/best_features-rdb0-only-short.txt"
    bestf="../../scripts/crystal/best_features-rdb0-only.txt"
    myforever="forever"
    ../cldata_predict.py \
        short-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat \
        --tier short --final --xgboost \
        --basedir ../../src/predict/ --bestfeatfile ${bestf_short} | tee out_short

    ../cldata_predict.py \
        long-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat  \
        --tier long  --final --xgboost\
        --basedir ../../src/predict/ --bestfeatfile $bestf | tee out_long

    ../cldata_predict.py \
        ${myforever}-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat \
        --tier ${myforever} --final --xgboost \
        --basedir ../../src/predict/ --bestfeatfile $bestf --topperc | tee out_${myforever}

    cp ../../src/predict/*.json classifiers/
    cp out_* classifiers/
    tar czvf classifiers-cut1-${cut1}-cut2-${cut2}-limit-${limit}.tar.gz classifiers
}

limit=700
cut1="10.0"
cut2="40.0"
concat

cut1="20.0"
cut2="50.0"
concat

cut1="40.0"
cut2="70.0"
concat

exit 0
