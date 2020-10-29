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

    only=0.99
    bestf="../../scripts/crystal/best_features.txt"
    bestf="../../scripts/crystal/best_features-ext.txt"
    bestf="../../scripts/crystal/best_features-rdb0-only.txt"
    ../cldata_predict.py \
        short-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat \
        --tier short --xgboost --final --only $only \
        --basedir ../../src/predict/ --name short --bestfeatfile $bestf | tee out_short

    ../cldata_predict.py \
        long-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat  \
        --tier long --xgboost --final --only $only \
        --basedir ../../src/predict/ --name long --bestfeatfile $bestf | tee out_long

    ../cldata_predict.py \
        forever_div-comb-cut1-${cut1}-cut2-${cut2}-limit-${limit}.dat \
        --tier forever --xgboost --final --only $only \
        --basedir ../../src/predict/ --name forever --topperc --bestfeatfile $bestf | tee out_forever

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
